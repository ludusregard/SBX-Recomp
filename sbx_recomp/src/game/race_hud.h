// The race HUD's pieces worked out so far.

#pragma once

#include "game/guest_types.h"
#include "game/race_states.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

// A rival's logo on the HUD.
struct RivalLogo {
  enum Flags : uint32_t { kShown = 8, kFinished = 16 };

  i32_be unknown_0;
  u32_be flags;
  u32_be movie_key;  // Its UI movie, in the movie table.
  u32_be unknown_12;
  u32_be unknown_16;
  uint8_t unknown_20[4];  // A member built by sub_8220F490.
};

// A node of a UI movie (UiMovie::Node gives it), as far as the HUD sets it.
struct UiNode {
  uint8_t unknown_0[36];
  u32_be value_36;  // The boost meter's.
  uint8_t unknown_40[40];
  u32_be value_80;  // A gauge's.
  uint8_t unknown_84[24];
  i32_be frame;  // The gear's.
};
GUEST_OFFSET(UiNode, value_36, 36);
GUEST_OFFSET(UiNode, value_80, 80);
GUEST_OFFSET(UiNode, frame, 108);

// The race HUD object.
struct RaceHud {
  uint8_t unknown_0[12];
  u32_be shown;
  u32_be player;
  uint8_t unknown_20[112];
  uint8_t boost_meter[100];  // Updated by sub_8220FEA0.
  u32_be boost;
  uint8_t unknown_236[200];
  u32_be counting_down;
  u32_be vehicle;  // The player's.
  uint8_t unknown_444[16];
  u32_be item;     // The item shown.
  uint8_t unknown_464[4];
  u32_be warning;  // Whether the warning is drawn.
  u32_be gauges;   // Whether the gauges are drawn.
  uint8_t unknown_476[112];
  u32_be nodes;  // Its node list in the movie.
  uint8_t unknown_592[16];
  u32_be movie;
  u32_be countdown_movie;
  u32_be warning_movie;
  uint8_t unknown_620[8];
  u32_be movie_628;
};
GUEST_OFFSET(RaceHud, boost_meter, 132);
GUEST_OFFSET(RaceHud, boost, 232);
GUEST_OFFSET(RaceHud, counting_down, 436);
GUEST_OFFSET(RaceHud, vehicle, 440);
GUEST_OFFSET(RaceHud, item, 460);
GUEST_OFFSET(RaceHud, warning, 468);
GUEST_OFFSET(RaceHud, nodes, 588);
GUEST_OFFSET(RaceHud, movie, 608);
GUEST_OFFSET(RaceHud, warning_movie, 616);
GUEST_OFFSET(RaceHud, movie_628, 628);

// The race's screens, as far as their menus' draws go.
struct RaceScreens {
  uint8_t unknown_0[1012];
  u32_be pause_menu;
  uint8_t unknown_1016[4];
  u32_be after_race_menu;
};
GUEST_OFFSET(RaceScreens, pause_menu, 1012);
GUEST_OFFSET(RaceScreens, after_race_menu, 1020);

}  // namespace game
