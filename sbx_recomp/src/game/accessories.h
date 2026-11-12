// The interior accessories: the set at [0x826B51F0], its three entries,
// each entry's accessory (a part model with two files and a skeleton), and
// the 36-byte files they're loaded through.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"
#include "game/model.h"

namespace game {

using guest::i32_be;
using guest::ptr;
using guest::u32_be;

// A file loaded through the archive.
struct AccessoryFile {
  static constexpr uint32_t kVtable = 0x82006710;
  enum State : uint32_t { kNone = 0, kLoading = 1, kLoaded = 2 };

  u32_be vtable;
  i32_be asset;
  i32_be serial;  // The archive's, while loading.
  u32_be state;
  u32_be data;
  u32_be size;
  u32_be owns_data;  // The data is the file's own pages (VirtualAlloc'd).
  u32_be callback;
  u32_be context;
};
GUEST_SIZE(AccessoryFile, 36);

struct AccessorySkeleton {
  static constexpr uint32_t kVtable = 0x820066D8;
  static constexpr uint32_t kVtable176 = 0x820066D4;

  u32_be vtable;
  uint8_t unknown_4[172];
  u32_be vtable_176;
  AccessoryFile file;
  u32_be member_216;  // Members with their own vtables.
  u32_be member_220;
  uint8_t unknown_224[16];
  u32_be member_240;
  uint8_t unknown_244[204];
};
GUEST_OFFSET(AccessorySkeleton, vtable_176, 176);
GUEST_OFFSET(AccessorySkeleton, file, 180);
GUEST_OFFSET(AccessorySkeleton, member_240, 240);
GUEST_SIZE(AccessorySkeleton, 448);

struct Accessory {
  static constexpr uint32_t kVtable = 0x820066C0;

  PartModel model;
  AccessoryFile files[2];
  uint8_t unknown_536[8];
  AccessorySkeleton skeleton;
  Vector position;
  Vector rotation;
  Vector scale;
  u32_be unknown_1040;
  u32_be unknown_1044;
  u32_be unknown_1048;
};
GUEST_OFFSET(Accessory, files, 464);
GUEST_OFFSET(Accessory, skeleton, 544);
GUEST_OFFSET(Accessory, position, 992);
GUEST_OFFSET(Accessory, scale, 1024);
GUEST_OFFSET(Accessory, unknown_1048, 1048);

// One of the set's entries: how many hold it, its file, its accessory.
struct AccessoryEntry {
  u32_be count;
  AccessoryFile file;
  uint8_t unknown_40[8];
  Accessory accessory;
};
GUEST_OFFSET(AccessoryEntry, file, 4);
GUEST_OFFSET(AccessoryEntry, accessory, 48);

// The set: three entries (by index; 3 is none) and the scale they're
// given.
struct Accessories {
  static constexpr uint32_t kBaseVtable = 0x82006704;
  static constexpr uint32_t kVtable = 0x82006718;
  static constexpr int kEntries = 3;
  static constexpr uint32_t kNone = 3;

  u32_be vtable;
  ptr<AccessoryEntry> entries[kEntries];
  Vector scale;
};
GUEST_SIZE(Accessories, 32);
GUEST_OFFSET(Accessories, scale, 16);

}  // namespace game
