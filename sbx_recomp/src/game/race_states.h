// The race states' machine and the race progress it resets.

#pragma once

#include "game/game_modes.h"
#include "game/guest_types.h"
#include "game/race_setup.h"

namespace game {

using guest::f32_be;
using guest::i16_be;
using guest::ptr;
using guest::u32_be;

struct ProgressEntry {
  u32_be unknown_0[3];
  uint8_t unknown_12[512];
  u32_be unknown_524;
  u32_be unknown_528;  // 2 once reset.
  i16_be unknown_532[2];
};
GUEST_SIZE(ProgressEntry, 536);
GUEST_OFFSET(ProgressEntry, unknown_524, 524);

// 256 signed halfwords, -1 once reset, and a word after them.
struct ProgressHalfwords {
  i16_be values[256];
  u32_be unknown_512;
};
GUEST_SIZE(ProgressHalfwords, 516);

// The race's progress at [0x826C7D90] (writable).
struct RaceProgress {
  uint8_t unknown_0[40];
  ProgressEntry entries[4];
  RaceEntry entry;  // Its parts are the racers' times.
  ProgressHalfwords unknown_6120[2];
  uint8_t unknown_7152[4];
  f32_be unknown_7156;
  u32_be unknown_7160[15];
  u32_be unknown_7220[3];
  f32_be unknown_7232[2];
  u32_be item;  // The item the HUD shows.
};
GUEST_OFFSET(RaceProgress, entries, 40);
GUEST_OFFSET(RaceProgress, entry, 2184);
GUEST_OFFSET(RaceProgress, unknown_6120, 6120);
GUEST_OFFSET(RaceProgress, unknown_7156, 7156);
GUEST_OFFSET(RaceProgress, unknown_7232, 7232);
GUEST_OFFSET(RaceProgress, item, 7240);
constexpr uint32_t kRaceProgress = 0x826C7D90;  // Holds the progress' address.

// The race states: a state machine, then its own words.
struct RaceStates {
  StateMachine machine;
  u32_be step;  // Cleared by ClearStep.
  u32_be unknown_9612;
  u32_be unknown_9616[2];
  ptr<RaceProgress> progress;
  u32_be unknown_9628[4];
};
GUEST_OFFSET(RaceStates, step, 9608);
GUEST_OFFSET(RaceStates, progress, 9624);
GUEST_OFFSET(RaceStates, unknown_9628, 9628);

}  // namespace game
