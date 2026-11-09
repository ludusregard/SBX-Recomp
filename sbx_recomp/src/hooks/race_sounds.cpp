// The race sounds' handlers as our own C++ (--native_race_sounds, on by
// default): the listener's set-up through the sound manager, and the
// handlers the race's messages call (the countdowns, the rolling start,
// the next stage, an abort, leaving after the race): the music's volume
// put back, the effects' volume, the state's bits, and the music picked.
//
// Written from SBX's generated code. The race sounds: +9644 their state's
// bits (0x10, 0x4000, 0x8000 and 0x4000 | 0x8000 the music's, 0x20000 the
// rolling start's), +9648 whether the cars are set up. The volumes are the
// options' (0x82606B74 the music's, 0x82606BBC its working copy,
// 0x82606BC0 and 0x82606BC4 the effects'). The sound manager's slots, the
// music's stop and pick, the cars' sounds and the ticks stay recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_sounds, true, "SBX",
                    "Our own C++ for the race sounds' handlers (the listener, the countdowns, "
                    "the rolling start, the next stage, an abort, leaving after the race) "
                    "instead of the recompiled functions; the sounds' ticks stay recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// RaceSounds::OnEffectsBankLoaded (sub_82173D90).
// 1 stores
constexpr Fill kRaceSounds_OnEffectsBankLoaded0[] = {
    {0x826C7E70, kAbsWord, 0x1},
};

bool On() { return REXCVAR_GET(native_race_sounds); }

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kSoundManager = 0x826B5960;  // [it] the AudioManager
constexpr uint32_t kMusic = 0x826B591C;         // [it] the music player
constexpr uint32_t kMusicVolume = 0x82606B74;
constexpr uint32_t kMusicVolumeNow = 0x82606BBC;
constexpr uint32_t kEffectsVolume = 0x82606BC0;
constexpr uint32_t kEffectsVolume2 = 0x82606BC4;
constexpr uint32_t kMusicFlag = 0x826B5964;  // a byte
constexpr uint32_t kState = 9644;
constexpr uint32_t kCarsSetUp = 9648;

void SetState(uint8_t* base, uint32_t sounds, uint32_t clear, uint32_t set) {
  StoreU32(base, sounds + kState, (LoadU32(base, sounds + kState) & ~clear) | set);
}

// The music's volume back to the option's, its flag cleared, the state's
// 0x10 cleared and `bits` set.
void RestoreMusic(uint8_t* base, uint32_t sounds, uint32_t bits) {
  StoreU32(base, kMusicVolumeNow, LoadU32(base, kMusicVolume));
  guest::StoreU8(base, kMusicFlag, 0);
  SetState(base, sounds, 0x10, bits);
}

// The effects' working volume back to the option's.
void RestoreEffects(uint8_t* base) {
  StoreU32(base, kEffectsVolume, LoadU32(base, kEffectsVolume2));
}

void Call(PPCContext& ctx, uint8_t* base, uint32_t sounds, void (*fn)(PPCContext&, uint8_t*)) {
  ctx.r3.u64 = sounds;
  fn(ctx, base);
}

}  // namespace

// RaceSounds::ResumeCars.
extern "C" REX_FUNC(sub_82173B18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173B18(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, 0x826B591C);
  sub_8210E440(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826B591C);
  sub_8210E440(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826B591C);
  sub_8210E440(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826B591C);
  sub_8210E440(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826B591C);
  sub_8210E440(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826B591C);
  sub_8210E440(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826B591C);
  sub_8210E440(ctx, base);
}

// RaceSounds::OnEffectsBankLoaded.
extern "C" REX_FUNC(sub_82173D90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173D90(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceSounds_OnEffectsBankLoaded0, args);
  ctx.r3.u64 = object;
}


// RaceSounds::SetListener: the sound manager's slots 26 (+104, r4 and r5
// as given), 25 (+100, r4 = r3) and 27 (+108, r4 = r6).
extern "C" REX_FUNC(sub_82173760) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173760(ctx, base);
    return;
  }
  const uint32_t a = ctx.r3.u32, d = ctx.r6.u32;
  const uint32_t slots[3] = {104, 100, 108};
  for (uint32_t i = 0; i < 3; i++) {
    const uint32_t manager = LoadU32(base, kSoundManager);
    ctx.r3.u64 = manager;
    if (i == 1) ctx.r4.u64 = a;
    if (i == 2) ctx.r4.u64 = d;
    REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, manager) + slots[i]));
  }
}

// RaceSounds::OnStageCountdown: the effects' volume back, the cars set
// up, 0x4000 cleared.
extern "C" REX_FUNC(sub_82173BE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173BE0(ctx, base);
    return;
  }
  RestoreEffects(base);
  StoreU32(base, ctx.r3.u32 + kCarsSetUp, 1);
  SetState(base, ctx.r3.u32, 0x4000, 0);
}

// RaceSounds::OnAbortDone: the same, the volume last.
extern "C" REX_FUNC(sub_82173BB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173BB8(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + kCarsSetUp, 1);
  SetState(base, ctx.r3.u32, 0x4000, 0);
  RestoreEffects(base);
}

// RaceSounds::OnRollingStartDone: during the rolling start (0x20000) and
// before its music (0x4000), the music stopped (sub_8210FEC0) and 0x4000
// set, 0x10000 cleared; then the cars set up and 0x4000 cleared.
extern "C" REX_FUNC(sub_82173C08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173C08(ctx, base);
    return;
  }
  const uint32_t sounds = ctx.r3.u32;
  const uint32_t state = LoadU32(base, sounds + kState);
  if ((state & 0x20000) && !(state & 0x4000)) {
    Call(ctx, base, sounds, sub_8210FEC0);
    SetState(base, sounds, 0x10000, 0x4000);
  }
  const uint32_t now = LoadU32(base, sounds + kState) & ~0x4000u;
  StoreU32(base, sounds + kCarsSetUp, 1);
  StoreU32(base, sounds + kState, now);
}

// RaceSounds::OnCountdown: the music player's sub_8210FFA0, the music
// back (0x8000), the rival music picked (sub_821743B0).
extern "C" REX_FUNC(sub_82174828) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82174828(ctx, base);
    return;
  }
  const uint32_t sounds = ctx.r3.u32;
  Call(ctx, base, LoadU32(base, kMusic), sub_8210FFA0);
  RestoreMusic(base, sounds, 0x8000);
  Call(ctx, base, sounds, sub_821743B0);
}

// RaceSounds::OnLeaveAfterRace: the cars' sounds updated (sub_82173DE0),
// the music back (0x8000), the effects' volume back.
extern "C" REX_FUNC(sub_82174D20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82174D20(ctx, base);
    return;
  }
  const uint32_t sounds = ctx.r3.u32;
  Call(ctx, base, sounds, sub_82173DE0);
  RestoreMusic(base, sounds, 0x8000);
  RestoreEffects(base);
}

// RaceSounds::OnNextStage: the music stopped, back (0xC000), the rival
// music picked.
extern "C" REX_FUNC(sub_82174D88) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82174D88(ctx, base);
    return;
  }
  const uint32_t sounds = ctx.r3.u32;
  Call(ctx, base, sounds, sub_8210FEC0);
  RestoreMusic(base, sounds, 0xC000);
  Call(ctx, base, sounds, sub_821743B0);
}

// RaceSounds::OnAbort: the music stopped and back (0xC000).
extern "C" REX_FUNC(sub_82173B60) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173B60(ctx, base);
    return;
  }
  const uint32_t sounds = ctx.r3.u32;
  Call(ctx, base, sounds, sub_8210FEC0);
  RestoreMusic(base, sounds, 0xC000);
}

// RaceSounds::OnRollingStart: the cars' sounds set up (sub_82174048) when
// +9648 is 1, the music back (0x8000), the rival music picked.
extern "C" REX_FUNC(sub_82174F80) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82174F80(ctx, base);
    return;
  }
  const uint32_t sounds = ctx.r3.u32;
  if (LoadU32(base, sounds + kCarsSetUp) == 1) {
    Call(ctx, base, sounds, sub_82174048);
  }
  RestoreMusic(base, sounds, 0x8000);
  Call(ctx, base, sounds, sub_821743B0);
}
