// Autosave (--autosave, off by default): saves the game when the player is
// back at the garage's top menu after a day of racing, or after closing the
// tuning shop, car setup, car shop, garage car or number plate menu. The
// game itself only writes the save when the player saves from the System
// menu or the save / load menu; shops, tuning and races change the save in
// memory only (docs/menus.md, "Saving").
//
// How it knows (docs/menus.md, "How the garage menus open and close"): every
// garage menu is a handler object with a state at +9604, driven by game
// messages through StateTable::Dispatch (sub_82106410), whose override in
// src/hooks/title_screen.cpp hands each one here (Autosave_OnDispatch):
// - A menu opens on its open message in state 9 (dormant). The last one that
//   opened is kept: 12 tuning shop, 18 or 19 car setup, 10 car shop, 23 garage
//   car, 28 number plate (the System menu, 14, saves on its own; the rival
//   list, 21, changes nothing).
// - The garage top is the object its load step (sub_8218C240, the repo's
//   PostRace::Check, src/hooks/post_race.cpp) runs for; it runs on every entry
//   to the garage. When the top becomes active again (state 5) or takes
//   message 6 ("back to the garage top"), the menu that just closed decides
//   whether to save.
// - A race drawn (Race::DrawStep) after the garage was first entered marks a
//   day of racing; the garage top's next activation saves. The title screen's
//   attract race comes before any garage, so it doesn't count; a course-in
//   cancelled before racing draws no race.
//
// The save goes through the game's own SaveGame::Write (sub_8228CEE8), so
// the file is the one a manual save writes, and is followed as the game's
// save / load menu follows its own (see Step). It's skipped when the device
// check (sub_8228CDD0) answers 1 (no storage device), and waits while the
// device isn't idle (another save or load running). A short notice shows on
// the overlay instead of the game's saving window.
//
// 2026-11-09: the first version followed SaveGame::Status, which stays 0 for
// these saves (it's the System menu's), so it took every save as failed after
// 20 s while calling the save pump (sub_8228CE40) all that time, and then
// SaveGame::Finish; a manual save made then waited forever. The saves
// themselves were written. --log_autosave traces every save call, the game's
// and ours, with its caller and result.
//
// Still to confirm with --log_autosave (it logs every state change and every
// message other than the tick and draw that a handler object takes): the open
// ids, that the top's activation is seen, and SaveGame::Status's values.

#include "hooks/autosave.h"

#include "guest.h"
#include "hooks/native_calls.h"

#ifdef SBX_VULKAN_RENDERER
#include "native/overlay.h"
#endif

#include <rex/cvar.h>
#include <rex/logging.h>

#include <chrono>
#include <cstdio>

REXCVAR_DEFINE_BOOL(autosave, false, "SBX",
                    "Save the game when back at the garage's top menu after racing, or after "
                    "the tuning shop, car setup, car shop, garage car or number plate menu");
REXCVAR_DEFINE_BOOL(log_autosave, false, "SBX",
                    "Diagnostic: log the garage menus' state changes and messages, and the "
                    "autosave's save calls (to confirm the ids autosave uses)");

namespace {

using Clock = std::chrono::steady_clock;

// A handler object's state (StateTable).
constexpr uint32_t kStateOffset = 9604;
constexpr uint32_t kActive = 5;
constexpr uint32_t kDormant = 9;
// StateTable's "no entry" (sub_82106380 returns it when the table has none).
constexpr uint32_t kNoEntry = 0x82695848;
// Message 6: back to the garage top.
constexpr uint32_t kBackToTop = 6;

// The save device's state (sub_8228CDD0) when nothing runs; a write leaves
// it and comes back.
constexpr uint32_t kDeviceIdle = 3;
constexpr auto kSaveTimeout = std::chrono::seconds(20);
// How long a pending autosave waits for a running save to finish.
constexpr auto kBusyTimeout = std::chrono::seconds(10);
// Status polls: about once a tick.
constexpr auto kPollInterval = std::chrono::milliseconds(30);

const char* MenuName(uint32_t open_message) {
  switch (open_message) {
    case 10: return "car shop";
    case 12: return "tuning shop";
    case 18:
    case 19: return "car setup";
    case 23: return "garage car";
    case 28: return "number plate";
    default: return nullptr;
  }
}

enum class Phase { kIdle, kPending, kSaving };

uint32_t g_garage_top = 0;      // the garage top's handler object
bool g_garage_seen = false;     // a garage has been entered this session
bool g_raced = false;           // a race was drawn since the top was last active
uint32_t g_last_menu = 0;       // the open message of the last garage menu opened
Phase g_phase = Phase::kIdle;
const char* g_reason = "";
Clock::time_point g_phase_start{};
Clock::time_point g_last_poll{};
uint32_t g_last_device = UINT32_MAX;
bool g_in_autosave = false;  // the save calls below are ours
bool g_left_idle = false;    // the device has left idle since the write

void Notify(const char* text) {
#ifdef SBX_VULKAN_RENDERER
  native::overlay::Notify(text);
#else
  (void)text;
#endif
}

// A save call taking r3 = 0 (the save slot), returning r3.
#define SAVE_CALL(function) \
  (g_in_autosave = true, ctx.r3.u64 = 0, function(ctx, base), g_in_autosave = false, ctx.r3.u32)

void MarkPending(const char* reason) {
  if (!REXCVAR_GET(autosave)) {
    return;
  }
  if (g_phase == Phase::kSaving) {
    REXLOG_INFO("autosave: {}: a save is already running; not saving again", reason);
    return;
  }
  g_phase = Phase::kPending;
  g_reason = reason;
  g_phase_start = Clock::now();
  REXLOG_INFO("autosave: back at the garage top {}; saving", reason);
}

// The garage top is active: starts the pending save, or follows the running
// one. At most every kPollInterval.
//
// It follows the save the way the game's own save / load menu does
// (--log_autosave's trace, 2026-11-09: that menu calls SaveGame::Write with
// the device state at 3 and then only reads the device state): 3 is idle,
// a write runs through 7, 5 and 6 and is done when it's back at 3. Nothing
// else is called: no SaveGame::Status, no pump (sub_8228CE40) and no
// SaveGame::Finish, which the menu doesn't make around a write either.
// (Calling the pump for 20 s after a save that had already finished left a
// manual save right after it waiting forever.)
void Step(PPCContext& ctx, uint8_t* base) {
  const auto now = Clock::now();
  if (now - g_last_poll < kPollInterval) {
    return;
  }
  g_last_poll = now;
  const bool log = REXCVAR_GET(log_autosave);
  const auto elapsed = [&] {
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - g_phase_start).count();
  };

  if (g_phase == Phase::kPending) {
    const uint32_t device = SAVE_CALL(sub_8228CDD0);
    if (device == 1) {
      REXLOG_WARN("autosave: not saved: no save device (sub_8228CDD0 answered 1)");
      Notify("Autosave skipped: no save device");
      g_phase = Phase::kIdle;
      return;
    }
    if (device != kDeviceIdle) {
      if (now - g_phase_start > kBusyTimeout) {
        REXLOG_WARN("autosave: not saved: the save device stayed busy (state {})", device);
        g_phase = Phase::kIdle;
      } else if (log) {
        REXLOG_INFO("autosave: waiting: device state {}", device);
      }
      return;
    }
    REXLOG_INFO("autosave: saving ({})", g_reason);
    g_in_autosave = true;
    sub_8228CEE8(ctx, base);  // SaveGame::Write
    g_in_autosave = false;
    g_phase = Phase::kSaving;
    g_phase_start = now;
    g_last_device = UINT32_MAX;
    g_left_idle = false;
    return;
  }

  if (g_phase != Phase::kSaving) {
    return;
  }
  const uint32_t device = SAVE_CALL(sub_8228CDD0);
  if (device != g_last_device) {
    if (log) {
      REXLOG_INFO("autosave: device state {} after {} ms", device, elapsed());
    }
    g_last_device = device;
  }
  if (device != kDeviceIdle) {
    g_left_idle = true;
  } else if (g_left_idle) {
    g_phase = Phase::kIdle;
    REXLOG_INFO("autosave: saved ({}, {} ms)", g_reason, elapsed());
    Notify("Saved");
    return;
  }
  if (device == 1 || now - g_phase_start > kSaveTimeout) {
    g_phase = Phase::kIdle;
    REXLOG_WARN("autosave: the save didn't finish (device state {}); the game may not be "
                "saved",
                device);
    Notify("Autosave failed");
  }
}

// The garage top is back: the menu that just closed, or a day of racing,
// decides whether to save.
void OnBackAtTop() {
  if (const char* menu = MenuName(g_last_menu)) {
    static thread_local char reason[48];
    std::snprintf(reason, sizeof(reason), "after the %s", menu);
    MarkPending(reason);
  } else if (g_raced) {
    MarkPending("after racing");
  }
  g_last_menu = 0;
  g_raced = false;
}

}  // namespace

namespace hooks {

void Autosave_OnGarageTopLoadStep(uint32_t garage_top) {
  if (garage_top != g_garage_top && REXCVAR_GET(log_autosave)) {
    REXLOG_INFO("autosave: the garage top is {:08X}", garage_top);
  }
  g_garage_top = garage_top;
  g_garage_seen = true;
}

void Autosave_OnRaceDrawn() {
  if (g_garage_seen) {
    g_raced = true;
  }
  g_last_menu = 0;  // message ids mean other things in races
}

}  // namespace hooks

namespace hooks {

bool Autosave_Watching() { return REXCVAR_GET(autosave) || REXCVAR_GET(log_autosave); }

// After StateTable::Dispatch (sub_82106410) ran for `object` with message
// `id` in state `before`; ctx.r3 is its result (the table entry), kept.
void Autosave_OnDispatch(PPCContext& ctx, uint8_t* base, uint32_t object, uint32_t id,
                         uint32_t before) {
  const uint64_t entry = ctx.r3.u64;
  const bool log = REXCVAR_GET(log_autosave);
  const uint32_t after = guest::LoadU32(base, object + kStateOffset);
  const bool found = ctx.r3.u32 != kNoEntry;
  const bool top = object != 0 && object == g_garage_top;

  if (log && ((found && id > 1) || before != after)) {
    REXLOG_INFO("autosave: {:08X}{} message {} in state {} -> {}{}", object,
                top ? " (garage top)" : "", id, before, after, found ? "" : " (no entry)");
  }
  if (found && before == kDormant && MenuName(id)) {
    g_last_menu = id;
  }
  if (top && ((after == kActive && before != kActive) || (found && id == kBackToTop))) {
    OnBackAtTop();
  }
  if (top && after == kActive && g_phase != Phase::kIdle) {
    Step(ctx, base);
  }
  ctx.r3.u64 = entry;
}

}  // namespace hooks

// --- --log_autosave's trace of the save calls -------------------------------
//
// Each call to the save object's functions, the game's and ours, with its
// caller (the return address) and its answer; repeats of the same caller and
// answer aren't logged again. The game's own save from the System menu shows
// the sequence (and the device states) a save takes.

namespace {

struct TraceKey {
  uint32_t lr = 0, result = UINT32_MAX;
};

void Trace(TraceKey& last, const char* name, uint32_t lr, uint32_t result) {
  if (!REXCVAR_GET(log_autosave) || (last.lr == lr && last.result == result)) {
    return;
  }
  last = {lr, result};
  REXLOG_INFO("autosave trace: {} from {:08X} ({}) -> {}", name, lr,
              g_in_autosave ? "autosave" : "game", result);
}

}  // namespace

#define SBX_TRACE_SAVE_CALL(name, label)                \
  extern "C" REX_FUNC(name) {                          \
    REX_FUNC_PROLOGUE();                               \
    static TraceKey last;                              \
    const uint32_t lr = uint32_t(ctx.lr);              \
    __imp__##name(ctx, base);                          \
    Trace(last, label, lr, ctx.r3.u32);                \
  }

SBX_TRACE_SAVE_CALL(sub_8228CDD0, "device state (sub_8228CDD0)")
SBX_TRACE_SAVE_CALL(sub_8228CE08, "SaveGame::Status (sub_8228CE08)")
SBX_TRACE_SAVE_CALL(sub_8228CE40, "save pump (sub_8228CE40)")
SBX_TRACE_SAVE_CALL(sub_8228CD98, "SaveGame::Finish (sub_8228CD98)")
SBX_TRACE_SAVE_CALL(sub_8228CFA8, "SaveGame::Read (sub_8228CFA8)")

// SaveGame::Write: the device state just before it, too.
extern "C" REX_FUNC(sub_8228CEE8) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(log_autosave)) {
    const uint32_t lr = uint32_t(ctx.lr);
    const uint64_t r3 = ctx.r3.u64;
    ctx.r3.u64 = 0;
    __imp__sub_8228CDD0(ctx, base);
    REXLOG_INFO("autosave trace: SaveGame::Write from {:08X} ({}), device state {}", lr,
                g_in_autosave ? "autosave" : "game", ctx.r3.u32);
    ctx.r3.u64 = r3;
    ctx.lr = lr;
  }
  hooks::NativeSaveInterface_Write(ctx, base);
}
