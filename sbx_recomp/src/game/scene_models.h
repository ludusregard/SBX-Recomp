// A scene model: a car (or traffic) model in the scene, its record, its
// deferred build and its interior accessory.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"
#include "game/scene_loads.h"
#include "game/scene_points.h"

namespace game {

using guest::i32_be;
using guest::ptr;
using guest::u32_be;

// An object that holds a car model at +24 (what SceneParts::Link and
// SceneInterior::Link are given).
struct CarModelOwner {
  uint8_t unknown_0[24];
  ptr<CarModel> model;
};
GUEST_OFFSET(CarModelOwner, model, 24);

// The object a scene interior's record is copied to: +28 the copy, its
// +112 the accessory (3 for none).
struct InteriorRecordCopy {
  uint8_t unknown_0[28];
  InteriorRecord record;
};
GUEST_OFFSET(InteriorRecordCopy, record, 28);

// The accessories' set at [0x826B51F0]: each entry's address, by the
// interior's accessory.
struct AccessorySet {
  uint8_t unknown_0[4];
  u32_be entries[3];
};

struct SceneModelInterior {
  uint8_t unknown_0[36];
  ptr<InteriorRecordCopy> interior;
};
GUEST_OFFSET(SceneModelInterior, interior, 36);

// The scene a model belongs to, as far as SceneModel::Update reads it.
struct SceneModelScene {
  uint8_t unknown_0[4];
  u32_be unknown_4;  // 0: the interior's readiness is the model's +4296.
  uint8_t unknown_8[4992];
  u32_be unknown_5000;  // Set: no accessory update.
};
GUEST_OFFSET(SceneModelScene, unknown_5000, 5000);

struct SceneModel {
  uint8_t unknown_0[32];
  SceneModelRecord record;
  uint8_t unknown_4276[20];
  u32_be unknown_4296;
  uint8_t unknown_4300[4];
  Matrix transform;  // The accessory's anchor is multiplied by it.
  uint8_t unknown_4368[664];
  u32_be second_wheels_added;
  u32_be reload_decals;
  u32_be build_pending;
  i32_be build_countdown;
  ptr<CarModelOwner> car_model;
  uint8_t unknown_5052[4];
  u32_be unknown_5056;
  uint8_t unknown_5060[8];
  ptr<SceneModelInterior> unknown_5068;
  u32_be unknown_5072;
  u32_be unknown_5076;
  uint8_t unknown_5080[28];
  u32_be second_wheels;
  u32_be unknown_5112;
  ptr<SceneModelScene> scene;
};
GUEST_OFFSET(SceneModel, record, 32);
GUEST_OFFSET(SceneModel, unknown_4296, 4296);
GUEST_OFFSET(SceneModel, transform, 4304);
GUEST_OFFSET(SceneModel, second_wheels_added, 5032);
GUEST_OFFSET(SceneModel, build_countdown, 5044);
GUEST_OFFSET(SceneModel, car_model, 5048);
GUEST_OFFSET(SceneModel, unknown_5056, 5056);
GUEST_OFFSET(SceneModel, unknown_5068, 5068);
GUEST_OFFSET(SceneModel, second_wheels, 5108);
GUEST_OFFSET(SceneModel, scene, 5116);

}  // namespace game
