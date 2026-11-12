// The race's scene object (at [0x826C7D7C], 31,168 bytes): the seven cars'
// draw records and the scene's light set.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"

namespace game {

using guest::u32_be;

// What a car's draw keeps: arrays of matrices and points (their elements'
// destructor is an empty one). Car::DrawUpdate's r3 points at one.
struct SceneCar {
  uint8_t unknown_0[32];
  Vector points_32[3];
  Vector points_80[3];
  Vector points_128[3];
  uint8_t unknown_176[16];
  Matrix matrices_192[3];
  Matrix matrices_384[3];
  Matrix matrices_576[6];
  Matrix matrices_960[6];
  Matrix matrices_1344[6];
  Matrix matrices_1728[6];
  Matrix matrices_2112[6];
  Matrix matrices_2496[6];
  Matrix matrices_2880[6];
  Matrix matrices_3264[6];
  uint8_t unknown_3648[640];
};
GUEST_SIZE(SceneCar, 4288);
GUEST_OFFSET(SceneCar, matrices_192, 192);
GUEST_OFFSET(SceneCar, matrices_3264, 3264);

// One of the scene's lights: a point light (kind 1) or a spot (kind 2).
struct RaceLight {
  static constexpr uint32_t kPoint = 1;
  static constexpr uint32_t kSpot = 2;

  u32_be kind;
  uint8_t unknown_4[12];
  Vector color;
  Vector level;
  Vector white;
  Vector spot;   // A spot's.
  Vector cone;   // A spot's: the cosines of its outer and inner angles.
};
GUEST_SIZE(RaceLight, 96);
GUEST_OFFSET(RaceLight, color, 16);
GUEST_OFFSET(RaceLight, cone, 80);

struct RaceLights {
  static constexpr int kLights = 11;

  u32_be count;
  u32_be points;
  u32_be spots;
  uint8_t unknown_12[4];
  RaceLight lights[kLights];
  u32_be unknown_1072;
};
GUEST_OFFSET(RaceLights, lights, 16);
GUEST_OFFSET(RaceLights, unknown_1072, 1072);

// The scene's cars (RaceScene::DestroyCars is given +16).
struct SceneCars {
  uint8_t unknown_0[32];
  SceneCar cars[7];
};
GUEST_SIZE(SceneCars, 30048);

struct RaceScene {
  uint8_t unknown_0[16];
  SceneCars cars;
  RaceLights lights;
  uint8_t unknown_31140[28];
};
GUEST_SIZE(RaceScene, 31168);
GUEST_OFFSET(RaceScene, lights, 30064);

// The race scene machine: a state machine, then its pause toggle.
struct RaceSceneMachine {
  uint8_t machine[9608];  // A StateMachine.
  u32_be paused;
};
GUEST_OFFSET(RaceSceneMachine, paused, 9608);

// A post effect's object, as the race scene sets it through its vtable.
struct PostEffectObject {
  u32_be vtable;
};

}  // namespace game

namespace game {

// The post effects' state at [0x826B29C8].
struct PostEffectsState {
  uint8_t unknown_0[8];
  guest::u32_be split_screen;
};

}  // namespace game
