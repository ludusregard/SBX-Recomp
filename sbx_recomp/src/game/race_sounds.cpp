// The race sounds' handlers as our own C++ (--native_race_sounds, on by
// default): the listener's set-up through the sound manager, and the
// handlers the race's messages call (the countdowns, the rolling start, the
// next stage, an abort, leaving after the race): the music's volume put
// back, the effects' volume, the state's bits, and the music picked. The
// volumes are the options' (writable, read each time). The sound manager's
// slots, the music's stop and pick, the cars' sounds and the ticks stay
// recompiled.

#include "game/race_sounds.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_sounds, true, "SBX",
                    "Our own C++ for the race sounds' handlers (the listener, the countdowns, "
                    "the rolling start, the next stage, an abort, leaving after the race) "
                    "instead of the recompiled functions; the sounds' ticks stay recompiled");

namespace {

using game::RaceSounds;
using game::SoundManager;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_race_sounds); }

// Writable globals.
constexpr uint32_t kSoundManager = 0x826B5960;     // [it] the AudioManager
constexpr uint32_t kMusicPlayer = 0x826B591C;      // [it] the music player
constexpr uint32_t kMusicVolume = 0x82606B74;      // the option's
constexpr uint32_t kMusicVolumeNow = 0x82606BBC;   // its working copy
constexpr uint32_t kEffectsVolumeNow = 0x82606BC0;
constexpr uint32_t kEffectsVolume = 0x82606BC4;    // the option's
constexpr uint32_t kMusicFlag = 0x826B5964;        // a byte
constexpr uint32_t kEffectsBankLoaded = 0x826C7E70;

u32_be& Global(uint32_t address) { return *At<u32_be>(address); }

void SetState(RaceSounds* sounds, uint32_t clear, uint32_t set) {
  sounds->state = (sounds->state & ~clear) | set;
}

// The music's volume back to the option's, its flag cleared, the state's
// music-paused bit cleared and `bits` set.
void RestoreMusic(RaceSounds* sounds, uint32_t bits) {
  Global(kMusicVolumeNow) = Global(kMusicVolume);
  *At<uint8_t>(kMusicFlag) = 0;
  SetState(sounds, RaceSounds::kMusicPaused, bits);
}

// The effects' working volume back to the option's.
void RestoreEffects() { Global(kEffectsVolumeNow) = Global(kEffectsVolume); }

// The music stopped (sub_8210FEC0) and the rival music picked
// (sub_821743B0), on the race sounds.
void StopMusic(Guest& g, RaceSounds* sounds) { g.Call(sub_8210FEC0, sounds); }
void PickMusic(Guest& g, RaceSounds* sounds) { g.Call(sub_821743B0, sounds); }

}  // namespace

#define SBX_RACE_SOUNDS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                 \
  if (!On()) {                         \
    __imp__##name(ctx, base);          \
    return;                            \
  }                                    \
  Guest g(ctx, base);

// RaceSounds::ResumeCars: the music player's sub_8210E440, once for each of
// the seven cars (the player read each time).
extern "C" REX_FUNC(sub_82173B18) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82173B18)
  for (int i = 0; i < 7; i++) {
    g.Call(sub_8210E440, Global(kMusicPlayer));
  }
}

// RaceSounds::OnEffectsBankLoaded: noted. r3 left.
extern "C" REX_FUNC(sub_82173D90) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82173D90)
  Global(kEffectsBankLoaded) = 1;
}

// RaceSounds::SetListener: the sound manager's slots 26 (r4 and r5 as
// given), 25 (r4 = r3) and 27 (r4 = r6), the manager read each time.
extern "C" REX_FUNC(sub_82173760) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82173760)
  const uint32_t a = ctx.r3.u32, d = ctx.r6.u32;
  auto manager = [] { return At<SoundManager>(Global(kSoundManager)); };
  g.CallVirtual(manager(), 26);
  g.CallVirtual(manager(), 25, a);
  g.CallVirtual(manager(), 27, d);
}

// RaceSounds::OnStageCountdown: the effects' volume back, the cars set
// up, the rolling start's music bit cleared.
extern "C" REX_FUNC(sub_82173BE0) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82173BE0)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  RestoreEffects();
  sounds->cars_set_up = 1;
  SetState(sounds, RaceSounds::kRollingStartMusic, 0);
}

// RaceSounds::OnAbortDone: the same, the volume last.
extern "C" REX_FUNC(sub_82173BB8) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82173BB8)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  sounds->cars_set_up = 1;
  SetState(sounds, RaceSounds::kRollingStartMusic, 0);
  RestoreEffects();
}

// RaceSounds::OnRollingStartDone: during the rolling start and before its
// music, the music stopped, its bit set and the stopped bit cleared; then
// the cars set up and the rolling start's music bit cleared.
extern "C" REX_FUNC(sub_82173C08) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82173C08)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  const uint32_t state = sounds->state;
  if ((state & RaceSounds::kRollingStart) && !(state & RaceSounds::kRollingStartMusic)) {
    StopMusic(g, sounds);
    SetState(sounds, RaceSounds::kMusicStopped, RaceSounds::kRollingStartMusic);
  }
  const uint32_t now = sounds->state & ~RaceSounds::kRollingStartMusic;
  sounds->cars_set_up = 1;
  sounds->state = now;
}

// RaceSounds::OnCountdown: the music player's sub_8210FFA0, the music
// back, the rival music picked.
extern "C" REX_FUNC(sub_82174828) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82174828)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  g.Call(sub_8210FFA0, Global(kMusicPlayer));
  RestoreMusic(sounds, RaceSounds::kMusicBack);
  PickMusic(g, sounds);
}

// RaceSounds::OnLeaveAfterRace: the cars' sounds updated (sub_82173DE0),
// the music back, the effects' volume back.
extern "C" REX_FUNC(sub_82174D20) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82174D20)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  g.Call(sub_82173DE0, sounds);
  RestoreMusic(sounds, RaceSounds::kMusicBack);
  RestoreEffects();
}

// RaceSounds::OnNextStage: the music stopped, back (with the rolling
// start's bit), the rival music picked.
extern "C" REX_FUNC(sub_82174D88) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82174D88)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  StopMusic(g, sounds);
  RestoreMusic(sounds, RaceSounds::kMusicBack | RaceSounds::kRollingStartMusic);
  PickMusic(g, sounds);
}

// RaceSounds::OnAbort: the music stopped and back (with the rolling start's
// bit).
extern "C" REX_FUNC(sub_82173B60) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82173B60)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  StopMusic(g, sounds);
  RestoreMusic(sounds, RaceSounds::kMusicBack | RaceSounds::kRollingStartMusic);
}

// RaceSounds::OnRollingStart: the cars' sounds set up (sub_82174048) when
// they're marked so, the music back, the rival music picked.
extern "C" REX_FUNC(sub_82174F80) {
  SBX_RACE_SOUNDS_FALLBACK(sub_82174F80)
  RaceSounds* sounds = At<RaceSounds>(ctx.r3.u32);
  if (sounds->cars_set_up == 1) {
    g.Call(sub_82174048, sounds);
  }
  RestoreMusic(sounds, RaceSounds::kMusicBack);
  PickMusic(g, sounds);
}
