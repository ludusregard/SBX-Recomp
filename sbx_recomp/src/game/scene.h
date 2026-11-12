// The scene's assets and the car scene that holds them, as far as their
// constructors, destructor and release show them.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"
#include "game/model.h"
#include "game/scene_loads.h"

namespace game {

using guest::i16_be;
using guest::u16_be;
using guest::u32_be;

// A load's slot, its XPR and the XPR's list.
struct AssetLoad {
  LoadSlot slot;
  XPR xpr;
  XPRList list;
};
GUEST_SIZE(AssetLoad, 68);

// A 20-byte record the assets clear when made.
struct AssetRecord {
  uint8_t unknown_0[3];
  uint8_t unknown_3;
  i16_be unknown_4[2];
  uint8_t unknown_8;
  uint8_t unknown_9;
  u16_be unknown_10[4];
  uint8_t unknown_18[2];
};
GUEST_SIZE(AssetRecord, 20);

struct SceneAssets {
  LoadSlot slot_0;
  XPR xpr_24;
  LoadSlot slot_48;
  LoadSlot slot_72;
  XPR xprs_96[2];
  XPRList lists_144[2];
  AssetLoad loads_184[3];
  LoadSlot slot_388;
  uint8_t unknown_412[4];
  PartModel part_416;
  LoadSlot slot_880;
  LoadSlot slot_904;
  AssetRecord record_928;
  u32_be pages_948;  // VirtualAlloc'd pages, freed by Release.
  AssetLoad loads_952[2];
  LoadSlot slots_1088[4];
  PartModel parts_1184[4];
  LoadSlot slots_3040[4];
  LoadSlot slots_3136[19];
  u32_be pages_3592[19];
  AssetRecord records_3668[19];
  LoadSlot slot_4048;
  LoadSlot slot_4072;
  PartModel part_4096;
  LoadSlot slots_4560[4];
  XPR xprs_4656[4];
  XPRList lists_4752[4];
};
GUEST_OFFSET(SceneAssets, loads_184, 184);
GUEST_OFFSET(SceneAssets, part_416, 416);
GUEST_OFFSET(SceneAssets, record_928, 928);
GUEST_OFFSET(SceneAssets, loads_952, 952);
GUEST_OFFSET(SceneAssets, parts_1184, 1184);
GUEST_OFFSET(SceneAssets, slots_3136, 3136);
GUEST_OFFSET(SceneAssets, pages_3592, 3592);
GUEST_OFFSET(SceneAssets, records_3668, 3668);
GUEST_OFFSET(SceneAssets, part_4096, 4096);
GUEST_OFFSET(SceneAssets, xprs_4656, 4656);
GUEST_OFFSET(SceneAssets, lists_4752, 4752);
GUEST_SIZE(SceneAssets, 4832);

struct CarSceneEntry {
  u32_be unknown_0;
  uint8_t unknown_4[4];
  u32_be unknown_8;
};
GUEST_SIZE(CarSceneEntry, 12);

struct CarScene {
  uint8_t unknown_0[40];
  u32_be unknown_40[2];
  uint8_t unknown_48[16];
  SceneAssets assets;
  CarSceneEntry entries[8];
  uint8_t unknown_4992[128];
  Vector unknown_5120[22];  // Made with w = 1.0.
};
GUEST_OFFSET(CarScene, assets, 64);
GUEST_OFFSET(CarScene, entries, 4896);
GUEST_OFFSET(CarScene, unknown_5120, 5120);

struct SceneSetUp {
  uint8_t unknown_0[36];
  u32_be unknown_36[2];
};
GUEST_OFFSET(SceneSetUp, unknown_36, 36);

}  // namespace game
