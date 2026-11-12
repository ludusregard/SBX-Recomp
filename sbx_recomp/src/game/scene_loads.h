// The scene's loads: the slots a file is loaded into, and the car model's,
// cockpit's, traffic's, wheels' and car parts' loads built on them.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"
#include "game/model.h"

namespace game {

using guest::f32_be;
using guest::i32_be;
using guest::u32_be;

// A file being loaded: its block (32 KB, freed by LoadSlot::Free unless the
// slot keeps it) and its state (2 while loading).
struct LoadSlot {
  static constexpr uint32_t kLoading = 2;

  u32_be unknown_0;
  i32_be unknown_4;
  u32_be unknown_8;
  u32_be keeps_block;
  u32_be block;
  u32_be state;
};
GUEST_SIZE(LoadSlot, 24);
GUEST_OFFSET(LoadSlot, state, 20);

// A part model takes 464 bytes in the loads' arrays.
static_assert(sizeof(PartModel) == 464);

struct CarModelLoads {
  uint8_t unknown_0[32];
  uint8_t skeleton[192];  // A Skeleton.
  uint8_t unknown_224[48];
  XPR xprs[2];
  XPRList lists[2];
};
GUEST_OFFSET(CarModelLoads, skeleton, 32);
GUEST_OFFSET(CarModelLoads, xprs, 272);
GUEST_OFFSET(CarModelLoads, lists, 320);

struct CockpitLoads {
  CarModelLoads model;
  uint8_t unknown_360[152];
  PartModel parts[6];
};
GUEST_OFFSET(CockpitLoads, parts, 512);

struct TrafficLoads {
  uint8_t unknown_0[48];
  uint8_t skeleton[192];  // A Skeleton.
  uint8_t unknown_240[48];
  XPR xprs[2];
  XPRList lists[2];
  uint8_t unknown_376[152];
  PartModel parts[6];
};
GUEST_OFFSET(TrafficLoads, skeleton, 48);
GUEST_OFFSET(TrafficLoads, xprs, 288);
GUEST_OFFSET(TrafficLoads, lists, 336);
GUEST_OFFSET(TrafficLoads, parts, 528);

struct WheelsLoads {
  LoadSlot slots_0[2];
  PartModel parts_48[2];
  LoadSlot slots_976[2];
  PartModel parts_1024[2];
  LoadSlot slots_1952[2];
  XPR xprs[2];
  XPRList lists[2];
  LoadSlot slots_2088[2];
  uint8_t unknown_2136[8];
  PartModel parts_2144[2];
};
GUEST_OFFSET(WheelsLoads, parts_48, 48);
GUEST_OFFSET(WheelsLoads, slots_976, 976);
GUEST_OFFSET(WheelsLoads, parts_1024, 1024);
GUEST_OFFSET(WheelsLoads, slots_1952, 1952);
GUEST_OFFSET(WheelsLoads, xprs, 2000);
GUEST_OFFSET(WheelsLoads, lists, 2048);
GUEST_OFFSET(WheelsLoads, slots_2088, 2088);
GUEST_OFFSET(WheelsLoads, parts_2144, 2144);
GUEST_SIZE(WheelsLoads, 3072);

// Three words a record keeps: an index (-1 for none) and two floats
// (Record12::Reset: both -1.0).
struct Record12 {
  i32_be unknown_0;
  f32_be unknown_4;
  f32_be unknown_8;
};
GUEST_SIZE(Record12, 12);

// The wheels' record (156 bytes; WheelsRecord::Reset).
struct WheelsRecord {
  u32_be unknown_0;
  u32_be unknown_4[3];
  Record12 unknown_16;  // -1, 20.0, 0.5 once reset.
  u32_be unknown_28;
  Record12 unknown_32;  // The same.
  Record12 unknown_44;
  Record12 unknown_56;
  u32_be unknown_68[5];
  u32_be caliper_texture;
  u32_be unknown_92[2];
  f32_be unknown_100[2];
  u32_be unknown_108[2];  // 11 once reset.
  u32_be unknown_116[2];
  i32_be unknown_124;
  f32_be unknown_128[7];
};
GUEST_SIZE(WheelsRecord, 156);
GUEST_OFFSET(WheelsRecord, unknown_44, 44);
GUEST_OFFSET(WheelsRecord, caliper_texture, 88);
GUEST_OFFSET(WheelsRecord, unknown_128, 128);

struct Wheels {
  uint8_t unknown_0[24];
  u32_be unknown_24;
  WheelsRecord record;
  uint8_t unknown_184[8];
  WheelsLoads loads;
  u32_be unknown_3264;
  u32_be unknown_3268;  // A copy of [0x826B2B20].
};
GUEST_OFFSET(Wheels, record, 28);
GUEST_OFFSET(Wheels, loads, 192);
GUEST_OFFSET(Wheels, unknown_3268, 3268);

// The car parts' loads: a slot and a model for each of 216 parts.
struct CarPartsLoads {
  static constexpr int kParts = 216;

  LoadSlot slots[kParts];
  PartModel parts[kParts];
  uint8_t unknown_105408[16];
  Vector points[30];
};
GUEST_OFFSET(CarPartsLoads, parts, 5184);
GUEST_OFFSET(CarPartsLoads, points, 105424);

struct CarParts {
  uint8_t unknown_0[96];
  CarPartsLoads loads;
};
GUEST_OFFSET(CarParts, loads, 96);

// The record a pool 4 object is loaded from (52 bytes; Pool4Record::Reset).
struct Pool4Record {
  struct Color {
    u32_be unknown_0;
    f32_be rgb[3];  // 1.0 once reset.
  };

  u32_be texture;
  Color colors[3];
};
GUEST_SIZE(Pool4Record, 52);

struct Pool4Object {
  uint8_t unknown_0[24];
  u32_be unknown_24;
  Pool4Record record;
  LoadSlot slot;
  uint8_t unknown_104[8];
  PartModel model;
  u32_be unknown_576;  // A copy of [0x826B2B20].
};
GUEST_OFFSET(Pool4Object, record, 28);
GUEST_OFFSET(Pool4Object, slot, 80);
GUEST_OFFSET(Pool4Object, model, 112);
GUEST_OFFSET(Pool4Object, unknown_576, 576);

// A car record's vinyl layer or sticker (64 bytes; Layer::Reset).
struct RecordLayer {
  i32_be unknown_0;
  u32_be unknown_4;
  u32_be kind;  // 8 once reset.
  uint8_t unknown_12[4];
  Vector unknown_16;
  f32_be unknown_32;
  uint8_t unknown_36[12];
  Vector unknown_48;
};
GUEST_SIZE(RecordLayer, 64);
GUEST_OFFSET(RecordLayer, unknown_48, 48);

// A car's record (3,696 bytes; CarRecord::Reset): nine vectors, 54
// layers, the part order.
struct SceneCarRecord {
  static constexpr int kLayers = 54;

  i32_be unknown_0;
  uint8_t unknown_4[12];
  Vector unknown_16[9];
  u32_be unknown_160[2];
  uint8_t unknown_168[8];
  RecordLayer layers[kLayers];
  u32_be part_order[11];  // Copied with memcpy.
  u32_be unknown_3676[4];
  uint8_t unknown_3692[4];
};
GUEST_SIZE(SceneCarRecord, 3696);
GUEST_OFFSET(SceneCarRecord, layers, 176);
GUEST_OFFSET(SceneCarRecord, part_order, 3632);
GUEST_OFFSET(SceneCarRecord, unknown_3676, 3676);

struct PartsRecord {
  u32_be unknown_0[13];
  u32_be unknown_52;  // 3 once reset.
  u32_be unknown_56;
};
GUEST_SIZE(PartsRecord, 60);

struct InteriorRecord {
  u32_be unknown_0[25];  // See InteriorRecord::Reset for the defaults.
  i32_be unknown_100;    // SceneInterior::Apply's index.
  u32_be unknown_104[2];
  i32_be accessory;  // 3 for none.
  u32_be unknown_116;
};
GUEST_OFFSET(InteriorRecord, accessory, 112);
GUEST_SIZE(InteriorRecord, 120);

struct CockpitRecord {
  u32_be unknown_0[5];
  uint8_t unknown_20[4];
  InteriorRecord interior;
};
GUEST_SIZE(CockpitRecord, 144);
GUEST_OFFSET(CockpitRecord, interior, 24);

// A scene model's record (4,244 bytes): its kind, then each part's.
struct SceneModelRecord {
  u32_be kind;
  uint8_t unknown_4[12];
  SceneCarRecord car;
  PartsRecord parts;
  InteriorRecord interior;
  CockpitRecord cockpit;
  WheelsRecord wheels;
  Pool4Record pool4;
};
GUEST_OFFSET(SceneModelRecord, car, 16);
GUEST_OFFSET(SceneModelRecord, parts, 3712);
GUEST_OFFSET(SceneModelRecord, interior, 3772);
GUEST_OFFSET(SceneModelRecord, cockpit, 3892);
GUEST_OFFSET(SceneModelRecord, wheels, 4036);
GUEST_OFFSET(SceneModelRecord, pool4, 4192);
GUEST_SIZE(SceneModelRecord, 4244);

// A car model, as far as its loads go.
struct CarModel {
  uint8_t unknown_0[32];
  SceneCarRecord record;  // Set by CarModel::SetRecord.
  LoadSlot slot;
  uint8_t unknown_3752[8];
  uint8_t skeleton[192];  // What the parts and interior link to.
  LoadSlot slots[2];
  uint8_t unknown_4000[48];
  uint8_t node_block[3168];  // What the interior links to.
  Vector accessory_node;     // The interior accessory's anchor row.
  uint8_t unknown_7232[24];
  u32_be record_address;  // A record: +8 its kind.
};
GUEST_OFFSET(CarModel, record, 32);
GUEST_OFFSET(CarModel, skeleton, 3760);
GUEST_OFFSET(CarModel, node_block, 4048);
GUEST_OFFSET(CarModel, accessory_node, 7216);
GUEST_OFFSET(CarModel, slot, 3728);
GUEST_OFFSET(CarModel, slots, 3952);
GUEST_OFFSET(CarModel, record_address, 7256);

// A car model's record, as far as its kind goes.
struct CarModelRecord {
  uint8_t unknown_0[8];
  u32_be kind;
};

}  // namespace game
