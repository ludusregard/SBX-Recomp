// The data tables: twelve assets loaded once at boot into one block, their
// entries byte-swapped in place once loaded, then read by index all through
// the game (the item records, asset 94, are what the shop, the parts lists
// and the battle music read). The table object is at [0x826B5980].

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

// An item record (asset 94; 400 of them).
struct ItemRecord {
  uint8_t unknown_0[3004];
};
GUEST_SIZE(ItemRecord, 3004);

// Asset 95's entries: a list ending at an entry whose first word is -1.
struct DataEntry20 {
  i32_be key_0;
  u32_be unknown_4;
  i32_be key_8;
  u32_be unknown_12[2];
};
GUEST_SIZE(DataEntry20, 20);

// Asset 96's entries: a list ending at an entry whose second word is -1.
struct DataEntry40 {
  u32_be unknown_0;
  i32_be end;
  u32_be unknown_8[8];
};
GUEST_SIZE(DataEntry40, 40);

// The eight 56-byte tables' entries: each list ends at an entry whose first
// word is -1.
struct DataEntry56 {
  i32_be first;
  u32_be unknown_4[13];
};
GUEST_SIZE(DataEntry56, 56);

// The table object (64 bytes).
struct DataTables {
  static constexpr uint32_t kItemCount = 400;

  u32_be serial;  // The last load's.
  u32_be loads_pending;
  u32_be item_count;
  u32_be block;  // All twelve tables, one after another.
  u32_be items;
  u32_be tables_56[8];  // Their asset ids at 0x82606BEC.
  u32_be table_20;      // Asset 95.
  u32_be table_40;      // Asset 96.
  u32_be table_84;      // Asset 84.
};
GUEST_SIZE(DataTables, 64);
GUEST_OFFSET(DataTables, items, 16);
GUEST_OFFSET(DataTables, table_20, 52);

}  // namespace game
