// The race sounds: the object the race's sound messages are handled on.

#pragma once

#include "game/guest_types.h"

namespace game {

struct RaceSounds {
  // The state's bits.
  static constexpr uint32_t kMusicPaused = 0x10;
  static constexpr uint32_t kRollingStartMusic = 0x4000;
  static constexpr uint32_t kMusicBack = 0x8000;
  static constexpr uint32_t kMusicStopped = 0x10000;
  static constexpr uint32_t kRollingStart = 0x20000;

  uint8_t machine[9644];  // A StateMachine and more.
  guest::u32_be state;
  guest::u32_be cars_set_up;
};
GUEST_OFFSET(RaceSounds, state, 9644);
GUEST_OFFSET(RaceSounds, cars_set_up, 9648);

// The sound manager (AudioManager) at [0x826B5960], called through its
// vtable.
struct SoundManager {
  guest::u32_be vtable;
};

}  // namespace game
