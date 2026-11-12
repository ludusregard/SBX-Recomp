// A save (129,200 bytes, from operator new): its header, stats, progress,
// course records and options, as far as their resets show them.

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::f32_be;
using guest::i16_be;
using guest::i32_be;
using guest::u16_be;
using guest::u32_be;

// One of the 15 records the stats keep (48 bytes).
struct StatRecord {
  u32_be unknown_0[3];
  f32_be unknown_12;
  u32_be unknown_16[2];
  i32_be unknown_24[3];  // -1 once reset.
  u32_be unknown_36[2];
  uint8_t unknown_44[4];
};
GUEST_SIZE(StatRecord, 48);

struct StatRecords {
  StatRecord records[15];
  u32_be unknown_720;
};
GUEST_SIZE(StatRecords, 724);

struct StatPair {
  f32_be unknown_0;
  u32_be unknown_4;
};

// The save's stats (6,044 bytes at +48).
struct SaveStats {
  uint8_t unknown_0[4888];
  f32_be unknown_4888[2];
  u32_be unknown_4896;
  uint8_t unknown_4900[48];
  StatRecords records;
  uint8_t unknown_5672[208];
  StatPair pairs_5880[9];
  uint8_t unknown_5952[4];
  u32_be unknown_5956;  // 1 once reset.
  uint8_t unknown_5960[48];
  u32_be unknown_6008;  // 3 once reset.
  uint8_t unknown_6012[28];
  u32_be unknown_6040;  // Save +6088: set for a new game from [0x826B5870].
};
GUEST_OFFSET(SaveStats, unknown_4896, 4896);
GUEST_OFFSET(SaveStats, records, 4948);
GUEST_OFFSET(SaveStats, pairs_5880, 5880);
GUEST_OFFSET(SaveStats, unknown_5956, 5956);
GUEST_OFFSET(SaveStats, unknown_6008, 6008);
GUEST_SIZE(SaveStats, 6044);

struct SaveEntry {
  u32_be unknown_0;  // 6 once cleared.
  u16_be unknown_4;
  uint8_t unknown_6;  // 1 once cleared.
  uint8_t unknown_7;
};
GUEST_SIZE(SaveEntry, 8);

// A fixed list's 50 nodes and its heads (as FixedList, game/fixed_list.h).
struct SaveListIndex {
  struct Node {
    i16_be next;
    i16_be slot;
  };

  Node nodes[50];
  i16_be free_head;
  i16_be unknown_free;
  i16_be head;
  i16_be unknown_head;
  i16_be count;
};

// A player's control map: each of the 15 actions' button.
struct ControlMap {
  u32_be buttons[15];
};
GUEST_SIZE(ControlMap, 60);

struct SaveOptions {
  u32_be unknown_0[6];  // 1 by default.
  u32_be unknown_24;    // 2 by default.
  uint8_t unknown_28[2];  // 15 by default.
  uint8_t unknown_30[2];
  u32_be unknown_32;
  uint8_t unknown_36[6];
  uint8_t unknown_42[2];
  ControlMap maps[4];
};
GUEST_OFFSET(SaveOptions, maps, 44);
GUEST_SIZE(SaveOptions, 284);

// The course records: a header, then 22 records of 408 bytes.
struct CourseRecords {
  u32_be unknown_0[4];
  u16_be unknown_16[2];
  uint8_t records[22][408];  // Each reset by sub_82105E20.
};
GUEST_SIZE(CourseRecords, 8996);

struct SaveData {
  static constexpr uint32_t kBytes = 129200;

  u32_be unknown_0;  // 1 for a new game.
  u32_be unknown_4;
  uint8_t unknown_8;
  uint8_t unknown_9[3];
  SaveEntry entries_12[2];
  uint8_t unknown_28;
  uint8_t unknown_29[3];
  SaveEntry entries_32[2];
  SaveStats stats;
  uint8_t unknown_6092[16];
  u32_be unknown_6108;
  u32_be unknown_6112;
  u32_be unknown_6116;
  SaveListIndex list_6120;
  uint8_t unknown_6330[98202];
  u32_be unknown_104532;
  uint8_t unknown_104536[4];
  u32_be car_unlocks;  // The car unlock bits' first word.
  uint8_t unknown_104544[8];
  uint8_t unknown_104552[60];
  u32_be unknown_104612[15];
  uint8_t unknown_104672[28];
  uint8_t unknown_104700[28];
  u32_be unknown_104728;
  uint8_t unknown_104732[4];
  u32_be unknown_104736;
  u32_be unknown_104740;
  uint8_t unknown_104744[4852];
  u32_be bits_109596[13];  // BitSet400.
  uint8_t unknown_109648[4];
  u32_be unknown_109652;
  uint8_t unknown_109656[10244];
  i32_be unknown_119900;
  uint8_t unknown_119904[4];
  CourseRecords course_records;
  uint8_t unknown_128904[12];  // Set up by sub_82105B48.
  SaveOptions options;
};
GUEST_OFFSET(SaveData, stats, 48);
GUEST_OFFSET(SaveData, unknown_6108, 6108);
GUEST_OFFSET(SaveData, list_6120, 6120);
GUEST_OFFSET(SaveData, unknown_104532, 104532);
GUEST_OFFSET(SaveData, car_unlocks, 104540);
GUEST_OFFSET(SaveData, unknown_104552, 104552);
GUEST_OFFSET(SaveData, unknown_104612, 104612);
GUEST_OFFSET(SaveData, unknown_104672, 104672);
GUEST_OFFSET(SaveData, unknown_104700, 104700);
GUEST_OFFSET(SaveData, unknown_104728, 104728);
GUEST_OFFSET(SaveData, bits_109596, 109596);
GUEST_OFFSET(SaveData, unknown_109652, 109652);
GUEST_OFFSET(SaveData, unknown_119900, 119900);
GUEST_OFFSET(SaveData, course_records, 119908);
GUEST_OFFSET(SaveData, options, 128916);
GUEST_SIZE(SaveData, SaveData::kBytes);

}  // namespace game
