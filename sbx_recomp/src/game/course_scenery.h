// The course scenery's pool objects, pools, course set and texture
// animations.

#pragma once

#include "game/fixed_list.h"
#include "game/guest_types.h"
#include "game/model.h"

namespace game {

using guest::i32_be;
using guest::ptr;
using guest::u32_be;

// A pool object's data: where it starts, its size, and how much is used
// (PoolObject::AllocData moves `used`).
struct PoolData {
  u32_be start;
  u32_be size;
  u32_be used;
};
GUEST_SIZE(PoolData, 12);

// An object in one of the scenery's pools.
struct PoolObject {
  enum Flags : uint32_t {
    kLoading = 0x2,     // Cleared by MarkLoaded.
    kDataLoaded = 0x8,  // Set by MarkLoaded.
    kLoaded = 0x9,      // Both of 0x1 and 0x8.
    kVisible = 0x10,
    kExpiring = 0x20,
    kLoadFlags = 0x3C,  // Cleared by ClearLoadFlags.
  };

  u32_be vtable;  // Slot 0: Key; slot 1: Load.
  PoolData data;
  u32_be pool;
  uint8_t unknown_20[4];
  u32_be data_end;
  u32_be unknown_28;
  u32_be flags;
  i32_be key;  // -1 for none.
};
GUEST_SIZE(PoolObject, 40);
GUEST_OFFSET(PoolObject, flags, 32);

// The two pool objects whose keys are their own words.
struct AreaLights {
  PoolObject object;
  uint8_t unknown_40[40];
  u32_be light_set;  // The light file's entry 0 (a DataFile of light groups).
  u32_be key;
};
GUEST_OFFSET(AreaLights, light_set, 80);
GUEST_OFFSET(AreaLights, key, 84);

struct AreaModel {
  PoolObject object;
  uint8_t unknown_40[24];
  u32_be key;
};
GUEST_OFFSET(AreaModel, key, 64);

// One pool: its objects are `unknown_4 + unknown_8` bytes each from
// `objects`.
struct PoolEntry {
  uint8_t unknown_0[4];
  u32_be unknown_4;
  u32_be unknown_8;
  u32_be objects;
  uint8_t unknown_16[8];
};
GUEST_SIZE(PoolEntry, 24);

struct ObjectPools {
  uint8_t unknown_0[4];
  u32_be pools;  // The address of its PoolEntry array.

  PoolEntry* Pool(uint32_t index) { return guest::Element<PoolEntry>(pools, index); }
};

// A model's assets in a scenery config: the first and last asset.
struct ModelAssets {
  u32_be first;
  u32_be last;
  uint8_t unknown_8[4];
};
GUEST_SIZE(ModelAssets, 12);

struct SceneryConfig {
  uint8_t unknown_0[16];
  u32_be model_count;
  uint8_t unknown_20[4];
  ModelAssets models;  // The first of `model_count`.

  ModelAssets* Model(uint32_t index) {
    return guest::Element<ModelAssets>(guest::AddressOf(&models), index);
  }
};
GUEST_OFFSET(SceneryConfig, models, 24);

struct Scenery {
  uint8_t unknown_0[28];
  ptr<SceneryConfig> config;
};
GUEST_OFFSET(Scenery, config, 28);

struct SceneryList {
  uint8_t unknown_0[4];
  u32_be unknown_4[2];
};

// A course set's list: a word, then a fixed list of 64 words.
struct CourseModelList {
  u32_be unknown_0;
  FixedList<64, u32_be> list;
};
GUEST_SIZE(CourseModelList, 528);

// A texture animation: the key it's on and the frame, with each key's
// first frame.
struct Animation {
  i32_be key_count;
  i32_be key;
  i32_be frame;
  i32_be last_frame;
  i32_be key_frames;  // The first of `key_count`.

  i32_be* KeyFrame(int32_t key) {
    return guest::Element<i32_be>(guest::AddressOf(&key_frames), uint32_t(key));
  }
};
GUEST_OFFSET(Animation, key_frames, 16);

// The animations: a count, then each one's offset from the end of the
// offsets.
struct AnimationSet {
  u32_be count;
  u32_be offsets;  // The first of `count`.

  Animation* Entry(uint32_t index) {
    const uint32_t table_end = guest::AddressOf(this) + ((uint32_t(count) + 1) << 2);
    return guest::At<Animation>(table_end + *guest::Element<u32_be>(guest::AddressOf(&offsets),
                                                                    index));
  }
};

// The course set's parts (CourseSet::InitParts makes them).
struct CourseParts {
  XPR xpr;
  uint8_t unknown_24[8];
  Model model;
  uint8_t unknown_456[8];
  CourseModelList list_464;
  uint8_t unknown_992[4];
  ptr<AnimationSet> animations;
  CourseModelList list_1000;
  uint8_t unknown_1528[4];
  CourseModelList lists_1532[4];
  uint8_t unknown_3644[80];
  u32_be unknown_3724[4];
};
GUEST_OFFSET(CourseParts, model, 32);
GUEST_OFFSET(CourseParts, list_464, 464);
GUEST_OFFSET(CourseParts, animations, 996);
GUEST_OFFSET(CourseParts, lists_1532, 1532);
GUEST_OFFSET(CourseParts, unknown_3724, 3724);

// The course set: a pool object, then its parts.
struct CourseSet {
  PoolObject object;
  uint8_t unknown_40[24];
  CourseParts parts;
};
GUEST_OFFSET(CourseSet, parts, 64);

}  // namespace game
