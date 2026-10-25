#include "debug/frame_state_diff.h"

#include "guest.h"
#include "subtick/game_messages.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fstream>
#include <sstream>
#endif

REXCVAR_DEFINE_BOOL(find_per_frame_state, false, "SBX",
                    "Debug: find game state that advances on render-only passes (--subtick), "
                    "and the code that advances it");
REXCVAR_DEFINE_INT32(find_per_frame_state_interval_ms, 2000, "SBX",
                     "How often --find_per_frame_state samples a render-only pass (0 = every "
                     "one; slow)");
REXCVAR_DEFINE_STRING(watch_addresses, "", "SBX",
                      "With --find_per_frame_state, guest addresses (hex, comma-separated) to "
                      "watch from the start, logging every change with the code making it");
REXCVAR_DEFINE_BOOL(find_any_steps, false, "SBX",
                    "With --find_per_frame_state, look for any steady step (a float changing "
                    "by less than 1, or an integer by up to 4, by the same amount in two "
                    "samples) instead of timer steps only");
REXCVAR_DEFINE_BOOL(find_countdowns, false, "SBX",
                    "With --find_per_frame_state, look for values counting down (a float in "
                    "0..10 going down, or a small integer going down by one) instead of any "
                    "timer step");

namespace debug {

namespace {

// Guest addresses that can hold game state: below 0x90000000 (0x9xxxxxxx
// mirrors the image at 0x8xxxxxxx, so its words would show twice).
constexpr uint64_t kGuestScanEnd = 0x90000000;
// Words watched around each handler. Small values (under kSmallValue, likely
// timers and countdowns) are picked first, then by address.
constexpr size_t kMaxWatched = 2000;
constexpr uint32_t kSmallValue = 1000;

struct Range {
  uint32_t start;
  uint32_t size;
};

// The guest's committed, writable memory.
std::vector<Range> ReadableGuestRanges(uint8_t* base) {
  std::vector<Range> ranges;
  uint64_t guest_start = reinterpret_cast<uint64_t>(base);
  uint64_t guest_end = guest_start + kGuestScanEnd;
  auto add = [&](uint64_t start, uint64_t end) {
    start = std::max(start, guest_start);
    end = std::min(end, guest_end);
    if (end <= start) {
      return;
    }
    // Merge with the last range when they touch.
    uint32_t s = uint32_t(start - guest_start);
    uint32_t n = uint32_t(end - start);
    if (!ranges.empty() && uint64_t(ranges.back().start) + ranges.back().size == s) {
      ranges.back().size += n;
    } else {
      ranges.push_back({s, n});
    }
  };
#ifdef _WIN32
  constexpr DWORD kWritable =
      PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
  uint64_t address = guest_start;
  while (address < guest_end) {
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) == 0) {
      break;
    }
    uint64_t region_start = reinterpret_cast<uint64_t>(info.BaseAddress);
    uint64_t region_end = region_start + info.RegionSize;
    if (info.State == MEM_COMMIT && (info.Protect & kWritable) &&
        !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
      add(region_start, region_end);
    }
    if (region_end <= address) {
      break;
    }
    address = region_end;
  }
#else
  std::ifstream maps("/proc/self/maps");
  std::string line;
  while (std::getline(maps, line)) {
    std::istringstream fields(line);
    std::string span, perms;
    fields >> span >> perms;
    if (perms.size() < 2 || perms[0] != 'r' || perms[1] != 'w') {
      continue;
    }
    uint64_t start = std::stoull(span.substr(0, span.find('-')), nullptr, 16);
    uint64_t end = std::stoull(span.substr(span.find('-') + 1), nullptr, 16);
    add(start, end);
  }
#endif
  return ranges;
}

float AsFloat(uint32_t bits) {
  float f;
  std::memcpy(&f, &bits, 4);
  return f;
}

// Whether a word went down like a countdown: a float in 0..10 going down by
// a small step, or a small integer going down by one.
const char* CountdownStep(uint32_t before, uint32_t after) {
  float a = AsFloat(before), b = AsFloat(after);
  if (std::isfinite(a) && std::isfinite(b) && a > 0 && a <= 10 && b < a && a - b < 0.5f &&
      a - b > 1e-4f) {
    return "float";
  }
  if (int64_t(before) - int64_t(after) == 1 && before <= 300) {
    return "int";
  }
  return nullptr;
}

// Whether a word changed by a small amount: a normal float by less than 1,
// or a small integer by up to 4. Sets `step` to the change.
const char* AnyStep(uint32_t before, uint32_t after, double& step) {
  float a = AsFloat(before), b = AsFloat(after);
  auto plain = [](float f) { return f == 0 || (std::abs(f) >= 1e-6f && std::abs(f) < 1e5f); };
  if (plain(a) && plain(b) && a != b && std::abs(b - a) < 1) {
    step = double(b) - double(a);
    return "float";
  }
  int64_t di = int64_t(after) - int64_t(before);
  if (di != 0 && di >= -4 && di <= 4 && before < 100000 && after < 100000) {
    step = double(di);
    return "int";
  }
  return nullptr;
}

// Whether a word stepped like a frame timer: a float by +-1/30 or +-1/60 s,
// or a small integer by +-1.
const char* TimerStep(uint32_t before, uint32_t after) {
  if (REXCVAR_GET(find_countdowns)) {
    return CountdownStep(before, after);
  }
  float a = AsFloat(before), b = AsFloat(after);
  if (std::isfinite(a) && std::isfinite(b) && a != b) {
    float d = std::abs(b - a);
    if (std::abs(d - 1.0f / 30) < 1e-5f || std::abs(d - 1.0f / 60) < 1e-5f) {
      return "float";
    }
  }
  int64_t di = int64_t(after) - int64_t(before);
  if ((di == 1 || di == -1) && before < 100000 && after < 100000) {
    return "int";
  }
  return nullptr;
}

class FrameStateDiff {
 public:
  void OnPassStart(uint8_t* base, bool render_only, bool in_race) {
    if (!REXCVAR_GET(find_per_frame_state)) {
      return;
    }
    if (snapshot_taken_) {
      Compare(base);
      snapshot_taken_ = false;
    }
    LogForcedValues(base);
    render_only_ = render_only;
    before_.clear();
    auto now = std::chrono::steady_clock::now();
    bool sample = render_only &&
                  now - last_snapshot_ >=
                      std::chrono::milliseconds(REXCVAR_GET(find_per_frame_state_interval_ms));
    if (sample) {
      last_snapshot_ = now;
      ranges_ = ReadableGuestRanges(base);
      snapshot_.clear();
      for (const Range& r : ranges_) {
        snapshot_.insert(snapshot_.end(), base + r.start, base + r.start + r.size);
      }
      snapshot_taken_ = true;
    }
    LogChanges(false);
  }

  void Before(uint8_t* base) {
    if (!REXCVAR_GET(find_per_frame_state) || !render_only_) {
      return;
    }
    AddForcedWatches();
    if (watched_.empty()) {
      return;
    }
    std::vector<uint32_t> values(watched_.size());
    for (size_t i = 0; i < watched_.size(); i++) {
      values[i] = guest::LoadU32(base, watched_[i]);
    }
    before_.push_back(std::move(values));
  }

  void After(uint8_t* base, const char* kind, uint32_t id) {
    if (!REXCVAR_GET(find_per_frame_state) || !render_only_ || before_.empty()) {
      return;
    }
    std::vector<uint32_t> values = std::move(before_.back());
    before_.pop_back();
    for (size_t i = 0; i < values.size(); i++) {
      uint32_t now = guest::LoadU32(base, watched_[i]);
      if (now != values[i]) {
        changes_[{std::string(kind), id, watched_[i]}]++;
        if (forced_.count(watched_[i]) && logged_values_ < kMaxLoggedValues) {
          logged_values_++;
          REXLOG_INFO("per-frame state: pass {} {} {:08X} wrote {:08X}: {} -> {}",
                      subtick::GameMessages_PassNumber(), kind, id, watched_[i], values[i], now);
        }
      }
    }
  }

 private:
  void Compare(uint8_t* base) {
    // Words are compared as stored (big endian), converted to values for the
    // timer test.
    size_t offset = 0;
    int found = 0;
    std::string sample;
    std::vector<uint32_t> stepped_now;
    for (const Range& r : ranges_) {
      const uint8_t* now = base + r.start;
      const uint8_t* then = snapshot_.data() + offset;
      for (uint32_t i = 0; i + 4 <= r.size; i += 4) {
        if (std::memcmp(now + i, then + i, 4) == 0) {
          continue;
        }
        uint32_t address = r.start + i;
        uint32_t stored;
        std::memcpy(&stored, then + i, 4);
        uint32_t before = __builtin_bswap32(stored);
        uint32_t after = guest::LoadU32(base, address);
        const char* step;
        if (REXCVAR_GET(find_any_steps)) {
          // Only steps equal to the word's step in an earlier sample: a
          // steady rate, not a buffer being rewritten.
          double change = 0;
          step = AnyStep(before, after, change);
          if (!step) {
            continue;
          }
          auto last = last_step_.find(address);
          bool steady = last != last_step_.end() &&
                        std::abs(last->second - change) <= 0.01 * std::abs(change);
          last_step_[address] = change;
          if (!steady) {
            continue;
          }
        } else {
          step = TimerStep(before, after);
          if (!step) {
            continue;
          }
        }
        found++;
        if (sample.size() < 3000) {
          sample += std::string(step) == "float"
                        ? std::format(" {:08X}:{}->{}", address, AsFloat(before), AsFloat(after))
                        : std::format(" {:08X}:{}->{}", address, before, after);
        }
        stepped_now.push_back(address);
      }
      offset += r.size;
    }
    REXLOG_INFO("per-frame state: pass {}: {} words stepped like timers over a render-only "
                "pass:{}",
                subtick::GameMessages_PassNumber(), found, sample);
    // Watch words that stepped in more than one sampled pass: steady
    // per-frame counters, not one-off changes (a load finishing).
    for (uint32_t address : stepped_now) {
      hits_[address]++;
    }
    watched_.clear();
    forced_added_ = false;
    for (int pick = 0; pick < 2; pick++) {
      for (const auto& [address, hits] : hits_) {
        bool small = guest::LoadU32(base, address) < kSmallValue;
        if (hits >= 2 && small == (pick == 0) && watched_.size() < kMaxWatched) {
          watched_.push_back(address);
        }
      }
    }
    if (hits_.size() > 200000) {
      hits_.clear();
    }
    if (last_step_.size() > 2000000) {
      last_step_.clear();
    }
  }

  // --watch_addresses words, once a pass: each change since the last pass
  // start, with the type of the pass that made it (so a word stepped outside
  // every wrapped handler still shows, and its rate: per pass or per tick).
  void LogForcedValues(uint8_t* base) {
    AddForcedWatches();
    for (uint32_t address : forced_) {
      uint32_t value = guest::LoadU32(base, address);
      auto last = forced_last_.find(address);
      if (last != forced_last_.end() && last->second != value) {
        REXLOG_INFO("per-frame state: pass {} ({}): {:08X} {} -> {} (float {} -> {})",
                    subtick::GameMessages_PassNumber(), render_only_ ? "render-only" : "logic",
                    address, last->second, value, AsFloat(last->second), AsFloat(value));
      }
      forced_last_[address] = value;
    }
  }

  // Every 2 s (or now), the code that changed watched words on render-only
  // passes since the last time.
  void LogChanges(bool now_please) {
    auto now = std::chrono::steady_clock::now();
    if ((!now_please && now - last_log_ < std::chrono::seconds(2)) || changes_.empty()) {
      return;
    }
    last_log_ = now;
    logged_values_ = 0;
    for (const auto& [key, n] : changes_) {
      REXLOG_INFO("per-frame state: {} {:08X} changed {:08X} ({} times)", std::get<0>(key),
                  std::get<1>(key), std::get<2>(key), n);
    }
    changes_.clear();
  }

  void AddForcedWatches() {
    if (forced_added_) {
      return;
    }
    forced_added_ = true;
    if (!forced_parsed_) {
      forced_parsed_ = true;
      std::string list = REXCVAR_GET(watch_addresses);
      size_t start = 0;
      while (start < list.size()) {
        size_t end = list.find(',', start);
        std::string item =
            list.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!item.empty()) {
          forced_.insert(uint32_t(std::stoul(item, nullptr, 16)));
        }
        if (end == std::string::npos) {
          break;
        }
        start = end + 1;
      }
    }
    for (uint32_t address : forced_) {
      if (std::find(watched_.begin(), watched_.end(), address) == watched_.end()) {
        watched_.insert(watched_.begin(), address);
      }
    }
  }

  // Every change to a --watch_addresses word is logged with its values, up
  // to this many per 2 s.
  static constexpr int kMaxLoggedValues = 200;
  int logged_values_ = 0;
  std::set<uint32_t> forced_;
  std::map<uint32_t, uint32_t> forced_last_;  // each forced word at the last pass start
  bool forced_added_ = false;
  bool forced_parsed_ = false;
  bool render_only_ = false;
  bool snapshot_taken_ = false;
  std::vector<Range> ranges_;
  std::vector<uint8_t> snapshot_;
  std::chrono::steady_clock::time_point last_snapshot_{}, last_log_{};
  std::vector<uint32_t> watched_;
  std::map<uint32_t, int> hits_;                    // sampled passes each word stepped in
  std::unordered_map<uint32_t, double> last_step_;  // --find_any_steps
  std::vector<std::vector<uint32_t>> before_;       // nested: messages within handlers
  std::map<std::tuple<std::string, uint32_t, uint32_t>, int> changes_;
};

FrameStateDiff& Get() {
  static FrameStateDiff diff;
  return diff;
}

}  // namespace

void FrameStateDiff_OnPassStart(uint8_t* base, bool render_only, bool in_race) {
  Get().OnPassStart(base, render_only, in_race);
}

void FrameStateDiff_Before(uint8_t* base) {
  Get().Before(base);
}

void FrameStateDiff_After(uint8_t* base, const char* kind, uint32_t id) {
  Get().After(base, kind, id);
}

}  // namespace debug

// TickHandler::Invoke: r3 = the app, r4 = the handler object (its vtable slot
// 1 is its tick; the target is at +8, as delegates keep it). Wrapped to name
// the tick handlers that advance watched state on render-only passes.
extern "C" REX_FUNC(sub_8209A1C0) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(find_per_frame_state)) {
    __imp__sub_8209A1C0(ctx, base);
    return;
  }
  uint32_t target = guest::LoadU32(base, ctx.r4.u32 + 8);
  debug::FrameStateDiff_Before(base);
  __imp__sub_8209A1C0(ctx, base);
  debug::FrameStateDiff_After(base, "tick handler", target);
}

// Race::DrawStep's parts (sub_82159540 calls these), wrapped only to name
// which one advances watched state (logged as "draw step part"), as ITC's
// scan wraps its draw step's. With --find_per_frame_state off they just run
// the original.
#define SBX_FRAME_STATE_WRAP(address)                                  \
  extern "C" REX_FUNC(sub_##address) {                                 \
    REX_FUNC_PROLOGUE();                                               \
    if (!REXCVAR_GET(find_per_frame_state)) {                          \
      __imp__sub_##address(ctx, base);                                 \
      return;                                                          \
    }                                                                  \
    debug::FrameStateDiff_Before(base);                                \
    __imp__sub_##address(ctx, base);                                   \
    debug::FrameStateDiff_After(base, "draw step part", 0x##address);  \
  }
SBX_FRAME_STATE_WRAP(820A0B40)  // per camera pass, in a loop over the passes
SBX_FRAME_STATE_WRAP(820AA160)
SBX_FRAME_STATE_WRAP(8209F998)
SBX_FRAME_STATE_WRAP(8209F9F8)
SBX_FRAME_STATE_WRAP(820C93A8)
SBX_FRAME_STATE_WRAP(820C5BA0)  // draws the cars
SBX_FRAME_STATE_WRAP(820F5F48)  // the draw step's own camera updates
SBX_FRAME_STATE_WRAP(82132438)
SBX_FRAME_STATE_WRAP(821325C0)
SBX_FRAME_STATE_WRAP(8212F5D8)
#undef SBX_FRAME_STATE_WRAP
