// Input sampled between ticks (--subtick, --subtick_input).
//
// Each tick a race mode has the player's control record built by
// sub_8215DBF8 (r3 = the record, r5 = the pad) from the action mappers, and
// Vehicle::Tick applies it to the car. With --subtick the passes between
// ticks are render-only: Update isn't delivered, Input::Update (sub_82099700)
// is skipped and the action mappers' update (sub_8210B140) runs on logic
// passes only, so presses, menu repeats and the input listeners still count
// ticks.
//
// At the start of each render-only pass in a race, this reads the pads again
// (the physical pads, sub_82577498, and the logical pads, sub_82577150; not
// the processors and listeners) and runs the mappers' update with their
// digital state (held, pressed and fired words, the per-action hysteresis
// and the repeat objects) put back afterwards, so only the analog values
// move. From those it works out the record's five axes as sub_8215DBF8
// does (see Sample below) and keeps them as a sample for the player.
//
// When the builder makes the tick's record, the samples since the last tick
// (the builder's own result last) are weighted into one input
// (input_weights.cpp) and written into the record right there, before the
// game copies it anywhere. So the replay recorder (sub_82102B30, right after
// the builder) stores exactly what the physics uses, and a replay plays back
// 1:1. Vehicle::Tick is the game's own. When a replay is playing, the
// builder's record is replaced from the replay (sub_82102BB0) after this, so
// live input never reaches it.


#include "subtick/input.h"

#include "subtick/game_messages.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <chrono>
#include <string>

REXCVAR_DECLARE(bool, subtick);

REXCVAR_DEFINE_BOOL(subtick_input, true, "SBX",
                    "With --subtick: sample the player's steering, throttle and brake on every "
                    "frame and feed them to the car's two physics steps a tick");

namespace subtick {

namespace {

// Input manager (r3 of Input::Update, sub_82099700).
constexpr uint32_t kPhysicalPadsOffset = 988;  // 4 x 212
constexpr uint32_t kPhysicalPadBytes = 212;
constexpr uint32_t kPhysicalPadCount = 4;
constexpr uint32_t kLogicalPadsOffset = 8;  // 5 x 196
constexpr uint32_t kLogicalPadBytes = 196;
constexpr uint32_t kLogicalPadCount = 5;
// Physical pad: flags (connected, connected or disconnected on this read)
// and the two rumble motors (whose timers count reads).
constexpr uint32_t kPadFlagsOffset = 4;
constexpr uint32_t kPadMotorsOffset = 168;
constexpr uint32_t kPadMotorsWords = 6;

// Action mapper (r3 of sub_8210C4A8): its digital state.
constexpr uint32_t kMapperWordsOffset = 448;  // held, held before, pressed, released, fired, +20
constexpr uint32_t kMapperWordsCount = 6;
constexpr uint32_t kMapperHysteresisOffset = 1852;  // 30 x 20 bytes (sub_8210B850)
constexpr uint32_t kMapperHysteresisWords = 150;
constexpr uint32_t kMapperRepeatsOffset = 472;  // 30 pointers to repeat objects
constexpr uint32_t kActionCount = 30;
constexpr uint32_t kRepeatWords = 4;  // vtable, count +4, delay +8, period +12 (sub_8210B6A0)

// Return addresses of the game's own calls, set for consistency.
constexpr uint32_t kPhysicalPadReturn = 0x82099728;
constexpr uint32_t kLogicalPadReturn = 0x8209974C;
constexpr uint32_t kMapperUpdateReturn = 0x82107B4C;
constexpr uint32_t kQueryReturn = 0x8215DD1C;

// sub_8215DBF8's table of the abstract input ids it queries:
// [0x826B5838] + pad * 120 + 0x1F7C0.
constexpr uint32_t kInputTablesPointer = 0x826B5838;
constexpr uint32_t kPadIdsStride = 120;
constexpr uint32_t kPadIdsOffset = 0x1F7C0;
// Offsets of the ids in it, and the fixed id tried first for steering.
constexpr uint32_t kThrottleId = 4, kBrakeId = 8, kSteeringId = 0, kAxis10Id = 12,
                   kAxis12Id = 48;
constexpr uint32_t kSteeringFirstId = 1;
constexpr uint32_t kQueryAnalog = 3;  // sub_821024D8 kind 3: the u16 analog value

// Word 0 has one of these set when the builder took its normal path (the
// throttle's source bits).
constexpr uint32_t kBuilderNormalPath = 0x0C000000;

constexpr uint32_t kMaxPads = 4;
constexpr size_t kMaxSamples = 64;

struct Player {
  bool valid = false;
  uint8_t control_type = 0;  // record +17
  std::vector<ControlSample> samples;
};
Player g_players[kMaxPads];

uint32_t g_manager = 0;      // Input::Update's r3
uint32_t g_mapper_list = 0;  // sub_8210B140's r3
bool g_refreshing = false;

bool Active() {
  return REXCVAR_GET(subtick) && REXCVAR_GET(subtick_input);
}

int32_t Clamp(int64_t v) {
  return int32_t(std::clamp<int64_t>(v, -32767, 32767));
}

// Throttle, brake and +10: sub_8215DBF8 at 0x8215DD38 (and the two after).
int32_t Pedal(int32_t out) {
  int32_t v = int32_t(int64_t(out) * 110 / 100);
  if (v > 32767) {
    v = 32767;
  }
  v -= 5000;
  if (v < 0) {
    return 0;
  }
  return int32_t(std::min<int64_t>(int64_t(v) * 32767 / 27767, 32767));
}

// Steering (0x8215DEB0): scale, a dead zone, rescaled to full range.
int32_t Steering(int32_t out, int32_t scale, int32_t dead_zone) {
  int32_t v = int32_t(int64_t(scale) * out / 100);
  int32_t t;
  if (v >= 0) {
    t = v - dead_zone;
    if (t < 0) {
      return 0;
    }
  } else {
    t = v + dead_zone;
    if (t > 0) {
      return 0;
    }
  }
  return Clamp(int64_t(t) * 32767 / (32767 - dead_zone));
}

// +12 (0x8215E008): reversed, a dead zone of 7500, rescaled.
int32_t Axis12(int32_t out) {
  int32_t v = Clamp(int64_t(out) * -110 / 100);
  if (v >= 0) {
    v -= 7500;
    if (v < 0) {
      return 0;
    }
  } else {
    v += 7500;
    if (v > 0) {
      return 0;
    }
  }
  return Clamp(int64_t(v) * 32767 / 25267);
}

// The record's five axes as sub_8215DBF8 works them out from the mappers'
// current state, for pad `pad`. One word on the guest stack holds each
// query's result, as the builder's +80 does: a query that writes nothing
// leaves the last axis there.
ControlSample Sample(PPCContext& ctx, uint8_t* base, uint32_t pad, uint8_t control_type) {
  uint32_t ids = guest::LoadU32(base, kInputTablesPointer) + pad * kPadIdsStride + kPadIdsOffset;
  guest::StackFrame frame(ctx, 112);
  uint32_t out = frame.Local(80);
  auto query = [&](uint32_t id) {
    ctx.r3.u64 = pad;
    ctx.r4.u64 = id;
    ctx.r5.u64 = kQueryAnalog;
    ctx.r6.u64 = out;
    ctx.lr = kQueryReturn;
    sub_82102610(ctx, base);
    return int32_t(guest::LoadU32(base, out));
  };
  auto keep = [&](int32_t v) {
    guest::StoreU32(base, out, uint32_t(v));
    return v;
  };
  ControlSample s;
  guest::StoreU32(base, out, 0);
  s.axes[0] = int16_t(keep(Pedal(query(guest::LoadU32(base, ids + kThrottleId)))));
  s.axes[1] = int16_t(keep(Pedal(query(guest::LoadU32(base, ids + kBrakeId)))));
  bool wheel = control_type == 1;
  guest::StoreU32(base, out, 0);
  int32_t steer = query(kSteeringFirstId);
  if (steer == 0) {
    steer = query(guest::LoadU32(base, ids + kSteeringId));
  }
  s.axes[2] = int16_t(keep(Steering(steer, wheel ? -130 : -110, wheel ? 1000 : 5000)));
  s.axes[3] = int16_t(keep(Pedal(query(guest::LoadU32(base, ids + kAxis10Id)))));
  s.axes[4] = int16_t(keep(Axis12(query(guest::LoadU32(base, ids + kAxis12Id)))));
  s.presence = (s.axes[0] ? 0x4 : 0) | (s.axes[1] ? 0x8 : 0) | (s.axes[3] ? 0x8000 : 0);
  return s;
}

}  // namespace

ControlSample Input_ReadSample(uint8_t* base, uint32_t record) {
  ControlSample s;
  uint32_t w1 = guest::LoadU32(base, record + 4);
  uint32_t w2 = guest::LoadU32(base, record + 8);
  uint32_t w3 = guest::LoadU32(base, record + 12);
  s.axes[0] = int16_t(w1 >> 16);
  s.axes[1] = int16_t(w1 & 0xFFFF);
  s.axes[2] = int16_t(w2 >> 16);
  s.axes[3] = int16_t(w2 & 0xFFFF);
  s.axes[4] = int16_t(w3 >> 16);
  s.presence = guest::LoadU32(base, record) & kPresenceBits;
  return s;
}

void Input_WriteSample(uint8_t* base, uint32_t record, const ControlSample& s) {
  auto pair = [](int16_t high, int16_t low) {
    return (uint32_t(uint16_t(high)) << 16) | uint16_t(low);
  };
  uint32_t w0 = guest::LoadU32(base, record);
  guest::StoreU32(base, record, (w0 & ~kPresenceBits) | s.presence);
  guest::StoreU32(base, record + 4, pair(s.axes[0], s.axes[1]));
  guest::StoreU32(base, record + 8, pair(s.axes[2], s.axes[3]));
  uint32_t w3 = guest::LoadU32(base, record + 12);
  guest::StoreU32(base, record + 12, (uint32_t(uint16_t(s.axes[4])) << 16) | (w3 & 0xFFFF));
}

void Input_OnRenderOnlyPass(PPCContext& ctx, uint8_t* base, float progress) {
  if (!Active() || !GameMessages_InRace() || !g_manager || !g_mapper_list) {
    return;
  }
  bool any = false;
  for (const Player& p : g_players) {
    any |= p.valid;
  }
  if (!any) {
    return;
  }
  // The pads, with their flags and rumble timers put back.
  for (uint32_t i = 0; i < kPhysicalPadCount; i++) {
    uint32_t pad = g_manager + kPhysicalPadsOffset + i * kPhysicalPadBytes;
    uint32_t flags = guest::LoadU32(base, pad + kPadFlagsOffset);
    uint32_t motors[kPadMotorsWords];
    for (uint32_t k = 0; k < kPadMotorsWords; k++) {
      motors[k] = guest::LoadU32(base, pad + kPadMotorsOffset + 4 * k);
    }
    {
      guest::StackFrame frame(ctx, 96);
      uint32_t user = frame.Local(80);
      guest::StoreU32(base, user, i);
      ctx.r3.u64 = pad;
      ctx.r4.u64 = user;
      ctx.lr = kPhysicalPadReturn;
      sub_82577498(ctx, base);
    }
    guest::StoreU32(base, pad + kPadFlagsOffset, flags);
    for (uint32_t k = 0; k < kPadMotorsWords; k++) {
      guest::StoreU32(base, pad + kPadMotorsOffset + 4 * k, motors[k]);
    }
  }
  for (uint32_t j = 0; j < kLogicalPadCount; j++) {
    ctx.r3.u64 = g_manager + kLogicalPadsOffset + j * kLogicalPadBytes;
    ctx.lr = kLogicalPadReturn;
    sub_82577150(ctx, base);
  }
  // The mappers' analog values (sub_8210C4A8 below puts their digital
  // state back).
  g_refreshing = true;
  ctx.r3.u64 = g_mapper_list;
  ctx.lr = kMapperUpdateReturn;
  sub_8210B140(ctx, base);
  g_refreshing = false;
  for (uint32_t pad = 0; pad < kMaxPads; pad++) {
    Player& p = g_players[pad];
    if (!p.valid || p.samples.size() >= kMaxSamples) {
      continue;
    }
    ControlSample s = Sample(ctx, base, pad, p.control_type);
    s.at = progress;
    p.samples.push_back(s);
  }
}

void Input_OnLogicPassFinished() {
  for (Player& p : g_players) {
    if (p.samples.empty()) {
      continue;
    }
    ControlSample carry = p.samples.back();
    carry.at = 0;
    p.samples.assign(1, carry);
  }
}

}  // namespace subtick

using subtick::g_players;

// Input::Update: r3 = the input manager. Skipped on render-only passes, so
// the pads' processors and listeners count ticks; the pads are read again
// for samples by Input_OnRenderOnlyPass instead.
extern "C" REX_FUNC(sub_82099700) {
  REX_FUNC_PROLOGUE();
  subtick::g_manager = ctx.r3.u32;
  if (REXCVAR_GET(subtick) && subtick::GameMessages_IsRenderOnlyPass()) {
    return;
  }
  __imp__sub_82099700(ctx, base);
}

// The action mappers' update (from sub_82107A38, every pass): r3 = the
// mapper list. Once a tick with --subtick: it rebuilds the
// held, pressed and fired words and steps each action's repeat, so run on
// render-only passes it lost presses made between ticks and repeated menu
// moves at the frame rate.
extern "C" REX_FUNC(sub_8210B140) {
  REX_FUNC_PROLOGUE();
  if (!subtick::g_refreshing) {
    subtick::g_mapper_list = ctx.r3.u32;
    if (REXCVAR_GET(subtick) && subtick::GameMessages_IsRenderOnlyPass()) {
      return;
    }
  }
  __imp__sub_8210B140(ctx, base);
}

// One action mapper's update: r3 = the mapper. While refreshing between
// ticks, its digital state is put back afterwards.
extern "C" REX_FUNC(sub_8210C4A8) {
  REX_FUNC_PROLOGUE();
  if (!subtick::g_refreshing) {
    __imp__sub_8210C4A8(ctx, base);
    return;
  }
  using namespace subtick;
  uint32_t mapper = ctx.r3.u32;
  uint32_t words[kMapperWordsCount];
  uint32_t hysteresis[kMapperHysteresisWords];
  uint32_t repeat_objects[kActionCount];
  uint32_t repeats[kActionCount][kRepeatWords];
  guest::CopyU32s(base, mapper + kMapperWordsOffset, words, kMapperWordsCount);
  guest::CopyU32s(base, mapper + kMapperHysteresisOffset, hysteresis, kMapperHysteresisWords);
  for (uint32_t i = 0; i < kActionCount; i++) {
    repeat_objects[i] = guest::LoadU32(base, mapper + kMapperRepeatsOffset + 4 * i);
    if (repeat_objects[i]) {
      guest::CopyU32s(base, repeat_objects[i], repeats[i], kRepeatWords);
    }
  }
  __imp__sub_8210C4A8(ctx, base);
  guest::StoreU32s(base, mapper + kMapperWordsOffset, words, kMapperWordsCount);
  guest::StoreU32s(base, mapper + kMapperHysteresisOffset, hysteresis, kMapperHysteresisWords);
  for (uint32_t i = 0; i < kActionCount; i++) {
    if (repeat_objects[i]) {
      guest::StoreU32s(base, repeat_objects[i], repeats[i], kRepeatWords);
    }
  }
}

// The player's control record builder: r3 = the record, r4, r5 = the pad,
// r6 = a message dispatcher, r7. Returns r3. Its record becomes the tick's
// last sample, and the record gets the tick's weighted input.
extern "C" REX_FUNC(sub_8215DBF8) {
  REX_FUNC_PROLOGUE();
  uint32_t record = ctx.r3.u32;
  uint32_t pad = ctx.r5.u32;
  __imp__sub_8215DBF8(ctx, base);
  if (!subtick::Active() || pad >= subtick::kMaxPads) {
    return;
  }
  uint64_t result = ctx.r3.u64;
  subtick::Player& p = g_players[pad];
  uint32_t w0 = guest::LoadU32(base, record);
  if (!(w0 & subtick::kBuilderNormalPath)) {
    p.valid = false;
    p.samples.clear();
    ctx.r3.u64 = result;
    return;
  }
  p.control_type = guest::LoadU8(base, record + 17);
  p.valid = true;
  subtick::ControlSample tick = subtick::Input_ReadSample(base, record);
  tick.at = 1;
  if (p.samples.empty()) {
    subtick::ControlSample carry = tick;
    carry.at = 0;
    p.samples.push_back(carry);
  }
  p.samples.push_back(tick);
  subtick::Input_WriteSample(base, record, subtick::Input_Aggregate(p.samples));
  ctx.r3.u64 = result;
}
