// The save data's defaults as our own C++ (--native_save_data, on by
// default): the stats' and progress' resets, the options' and control
// maps' defaults, the course records, a save slot's create and delete,
// the players' controllers, the menu lists' set-up and the title screen's
// player reset.
//
// The originals clear with memset and then store each field's default, 0s
// included; here the memset's clear is done in place and only the fields
// that aren't 0 are set after it, which leaves the same memory.
// SaveData::NewGame (sub_82113FB0) keeps unlock_cut_cars' override, which
// calls ours (at the end of this file) in place of the recompiled
// function; the course record's own reset (sub_82105E20), the heap and
// VirtualAlloc stay recompiled.

#include "game/race_screens.h"
#include "game/save_data.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(native_save_data, true, "SBX",
                    "Our own C++ for the save data's defaults (stats, progress, options, "
                    "course records, a slot's create and delete, the menu lists' set-up) "
                    "instead of the recompiled functions; the heap and the progress defaults "
                    "stay recompiled");

namespace {

using game::ControlMap;
using game::CourseRecords;
using game::MenuList;
using game::SaveData;
using game::SaveEntry;
using game::SaveListIndex;
using game::SaveOptions;
using game::SaveStats;
using game::StatPair;
using game::StatRecord;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_save_data); }

// The save slots (a word each), the players' controllers and the flag set
// once they're reset, and the title screen's menu list and state (all
// writable).
constexpr uint32_t kSaves = 0x826B5838;
constexpr uint32_t kControllers = 0x826AA884;
constexpr uint32_t kControllersSet = 0x826B73F8;
constexpr uint32_t kTitleMenuList = 0x8280DE40;
constexpr uint32_t kTitleState = 0x8280DE44;
constexpr uint32_t kPlayerFlags = 0x826AA820;
constexpr uint32_t kNewGameWord = 0x826B5870;

// The defaults' tables the getters return (writable).
constexpr uint32_t kNewGameTable60 = 0x826B4D88;
constexpr uint32_t kUnlockedTable = 0x826B2D24;
constexpr uint32_t kLeftOutTable = 0x826B3B14;
constexpr uint32_t kNewGameTableA = 0x826B3904;
constexpr uint32_t kNewGameTableB = 0x826B4DC4;

constexpr uint32_t kMemRelease = 0x8000;
constexpr uint32_t kMemCommit = 0x1000;
constexpr uint32_t kPageReadWrite = 4;

constexpr uint32_t kDefaultButtons[15] = {18, 14, 13, 9, 8, 10, 16, 15, 24, 7, 17, 0, 25, 12, 11};

// The title screen's state, as far as ResetPlayer clears it.
struct TitleState {
  uint8_t unknown_0[36];
  u32_be unknown_36;
};

// What a memset to 0 leaves.
template <typename T>
void Clear(T& object) {
  std::memset(&object, 0, sizeof(T));
}

u32_be& SaveSlot(uint32_t slot) { return *guest::Element<u32_be>(kSaves, slot); }

// StatRecords::Reset (sub_82113828): each record's words 0, its three
// indices -1.
void ResetStatRecords(game::StatRecords& records) {
  for (StatRecord& record : records.records) {
    for (u32_be& word : record.unknown_0) {
      word = 0;
    }
    record.unknown_12 = 0.0f;
    record.unknown_16[0] = 0;
    record.unknown_16[1] = 0;
    for (auto& index : record.unknown_24) {
      index = -1;
    }
    record.unknown_36[0] = 0;
    record.unknown_36[1] = 0;
  }
  records.unknown_720 = 0;
}

// Stats::Reset (sub_82113880): cleared, then its records' indices -1, +5956
// = 1 and +6008 = 3; returns its records' address, as the original leaves
// r3.
uint32_t ResetStats(SaveStats& stats) {
  Clear(stats);
  ResetStatRecords(stats.records);
  stats.unknown_5956 = 1;
  stats.unknown_6008 = 3;
  return guest::AddressOf(&stats.records);
}

// Stats::Stats (sub_82105830): its floats and the words after them 0.
void ConstructStats(SaveStats& stats) {
  stats.unknown_4888[1] = 0.0f;
  stats.unknown_4896 = 0;
  for (StatRecord& record : stats.records.records) {
    record.unknown_12 = 0.0f;
    record.unknown_16[0] = 0;
  }
  for (StatPair& pair : stats.pairs_5880) {
    pair.unknown_0 = 0.0f;
    pair.unknown_4 = 0;
  }
}

// The list index's nodes all free, as FixedList::Reset leaves them.
void ResetListIndex(SaveListIndex& list) {
  for (int i = 0; i < 50; i++) {
    list.nodes[i].slot = int16_t(i);
    list.nodes[i].next = int16_t(i + 1);
  }
  list.nodes[49].next = -1;
  list.free_head = 0;
  list.unknown_free = -1;
  list.head = -1;
  list.unknown_head = -1;
  list.count = 0;
}

// Progress::Clear (sub_82113D28): the save up to its course records
// cleared, its header's entries, its stats reset, then its other
// defaults. Returns what Stats::Reset leaves.
uint32_t ClearProgress(SaveData& save) {
  std::memset(&save, 0, offsetof(SaveData, course_records));
  for (SaveEntry* entries : {save.entries_12, save.entries_32}) {
    for (int i = 0; i < 2; i++) {
      entries[i].unknown_0 = 6;
      entries[i].unknown_6 = 1;
    }
  }
  const uint32_t r3 = ResetStats(save.stats);
  save.unknown_6108 = 14;
  save.unknown_6112 = 48;
  save.unknown_6116 = 3000;
  ResetListIndex(save.list_6120);
  save.unknown_104532 = 50;
  save.unknown_109652 = 4;
  save.unknown_119900 = -1;
  return r3;
}

// Options::SetDefaults (sub_82105AB8): returns its last map, as the
// original leaves r3.
uint32_t SetDefaultOptions(SaveOptions& options) {
  for (u32_be& word : options.unknown_0) {
    word = 1;
  }
  options.unknown_24 = 2;
  options.unknown_28[0] = 15;
  options.unknown_28[1] = 15;
  options.unknown_32 = 0;
  for (uint8_t& byte : options.unknown_36) {
    byte = 0;
  }
  for (ControlMap& map : options.maps) {
    for (int i = 0; i < 15; i++) {
      map.buttons[i] = kDefaultButtons[i];
    }
  }
  return guest::AddressOf(&options.maps[3]);
}

// MenuList::Init (sub_82209868): the count, the most shown and the words
// it's given; the cursor's words cleared; its flags (a word an entry, 1
// each) VirtualAlloc'd anew, the old ones VirtualFree'd.
void InitMenuList(Guest& g, MenuList* list, uint32_t count, uint32_t shown, uint32_t r6,
                  const uint32_t (&words)[6]) {
  list->count = count;
  list->unknown_4[0] = 0;
  list->unknown_4[1] = 0;
  list->selection = 0;
  list->previous_selection = 0;
  list->shown = int32_t(count) < int32_t(shown) ? count : shown;
  const uint32_t old = list->enabled;
  for (int i = 0; i < 6; i++) {
    list->unknown_36[i] = words[i];
  }
  list->unknown_64 = r6;
  list->unknown_24[0] = 0;
  list->unknown_24[1] = 0;
  if (old != 0) {
    g.Call(sub_82292D70, old, 0u, kMemRelease);  // VirtualFree
  }
  list->enabled = g.Call(sub_82292DD8, 0u, uint32_t(list->count) << 2, kMemCommit,
                         kPageReadWrite);  // VirtualAlloc
  for (int32_t i = 0; i < int32_t(uint32_t(list->count)); i++) {
    *guest::Element<u32_be>(list->enabled, uint32_t(i)) = 1;
  }
}

// SaveData::Delete (sub_82105748): the slot's save freed (operator delete)
// and its word cleared.
void DeleteSave(Guest& g, uint32_t slot) {
  g.Call(sub_8258C868, SaveSlot(slot));
  SaveSlot(slot) = 0;
}

// SaveData::Create (sub_82105938): a save from operator new, its stats
// made, stored to the slot (0 when the allocation failed, as the original
// goes on with it), then its course records reset, Progress::SetDefaults,
// the options' defaults and sub_82105B48 on +128904.
void CreateSave(Guest& g, uint32_t slot) {
  const uint32_t address = g.Call(sub_8258C000, SaveData::kBytes);  // operator new
  if (address != 0) {
    ConstructStats(At<SaveData>(address)->stats);
  }
  SaveSlot(slot) = address;
  g.Call(sub_82105E98, address + uint32_t(offsetof(SaveData, course_records)));
  g.Call(sub_82113FB0, SaveSlot(slot));
  g.Call(sub_82105AB8, SaveSlot(slot) + uint32_t(offsetof(SaveData, options)));
  g.Call(sub_82105B48, SaveSlot(slot) + uint32_t(offsetof(SaveData, unknown_128904)));
}

// Players::ResetControllers (sub_8209F718): controller i = i, then the
// flag set.
void ResetControllers() {
  for (uint32_t i = 0; i < 4; i++) {
    *guest::Element<u32_be>(kControllers, i) = i;
  }
  *At<u32_be>(kControllersSet) = 1;
}

}  // namespace

#define SBX_SAVE_DATA_FALLBACK(name) \
  REX_FUNC_PROLOGUE();               \
  if (!On()) {                       \
    __imp__##name(ctx, base);        \
    return;                          \
  }                                  \
  Guest g(ctx, base);

// Stats::Stats: r3 left.
extern "C" REX_FUNC(sub_82105830) {
  SBX_SAVE_DATA_FALLBACK(sub_82105830)
  ConstructStats(*At<SaveStats>(ctx.r3.u32));
}

// StatRecords::Reset: r3 left.
extern "C" REX_FUNC(sub_82113828) {
  SBX_SAVE_DATA_FALLBACK(sub_82113828)
  ResetStatRecords(*At<game::StatRecords>(ctx.r3.u32));
}

// Stats::Reset.
extern "C" REX_FUNC(sub_82113880) {
  SBX_SAVE_DATA_FALLBACK(sub_82113880)
  ctx.r3.u64 = ResetStats(*At<SaveStats>(ctx.r3.u32));
}

// Progress::Clear.
extern "C" REX_FUNC(sub_82113D28) {
  SBX_SAVE_DATA_FALLBACK(sub_82113D28)
  ctx.r3.u64 = ClearProgress(*At<SaveData>(ctx.r3.u32));
}

// BitSet400::SetAll: bits 0 to 399 set (12 full words and 0xFFFF).
extern "C" REX_FUNC(sub_821134F8) {
  SBX_SAVE_DATA_FALLBACK(sub_821134F8)
  u32_be* words = At<u32_be>(ctx.r3.u32);
  for (int i = 0; i < 12; i++) {
    words[i] = 0xFFFFFFFFu;
  }
  words[12] = 0x0000FFFFu;
}

// ControlMap::SetDefault.
extern "C" REX_FUNC(sub_82105A18) {
  SBX_SAVE_DATA_FALLBACK(sub_82105A18)
  ControlMap* map = At<ControlMap>(ctx.r3.u32);
  for (int i = 0; i < 15; i++) {
    map->buttons[i] = kDefaultButtons[i];
  }
}

// Options::SetDefaults: the options and their four control maps.
extern "C" REX_FUNC(sub_82105AB8) {
  SBX_SAVE_DATA_FALLBACK(sub_82105AB8)
  ctx.r3.u64 = SetDefaultOptions(*At<SaveOptions>(ctx.r3.u32));
}

// CourseRecords::Reset: the header cleared, then each record's own reset
// (sub_82105E20).
extern "C" REX_FUNC(sub_82105E98) {
  SBX_SAVE_DATA_FALLBACK(sub_82105E98)
  CourseRecords* records = At<CourseRecords>(ctx.r3.u32);
  for (u32_be& word : records->unknown_0) {
    word = 0;
  }
  records->unknown_16[1] = 0;
  records->unknown_16[0] = 0;
  for (auto& record : records->records) {
    g.Call(sub_82105E20, record);
  }
}

// SaveData::Delete: r3 the slot.
extern "C" REX_FUNC(sub_82105748) {
  SBX_SAVE_DATA_FALLBACK(sub_82105748)
  DeleteSave(g, ctx.r3.u32);
}

// SaveData::Create: r3 the slot.
extern "C" REX_FUNC(sub_82105938) {
  SBX_SAVE_DATA_FALLBACK(sub_82105938)
  CreateSave(g, ctx.r3.u32);
}

// Players::ResetControllers.
extern "C" REX_FUNC(sub_8209F718) {
  SBX_SAVE_DATA_FALLBACK(sub_8209F718)
  ResetControllers();
}

// MenuList::Init: r3 the list, r4 its count, r5 the most shown, r6 into
// +64, r7 to r10 and the two words at the caller's r1 + 84 and + 92 into
// +36 to +56.
extern "C" REX_FUNC(sub_82209868) {
  SBX_SAVE_DATA_FALLBACK(sub_82209868)
  const uint32_t sp = ctx.r1.u32;
  const uint32_t words[6] = {ctx.r7.u32, ctx.r8.u32, ctx.r9.u32, ctx.r10.u32,
                             *At<u32_be>(sp + 84), *At<u32_be>(sp + 92)};
  InitMenuList(g, At<MenuList>(ctx.r3.u32), ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, words);
}

// TitleScreen::ResetPlayer: the title's menu list set up (one entry), the
// title state's +36 cleared, the four player flags set, save slot 0
// deleted and made again, the controllers reset.
extern "C" REX_FUNC(sub_8221F7A8) {
  SBX_SAVE_DATA_FALLBACK(sub_8221F7A8)
  constexpr uint32_t kTitleWords[6] = {154, 153, 155, 156, 159, 158};
  InitMenuList(g, At<MenuList>(*At<u32_be>(kTitleMenuList)), 1, 1, 0, kTitleWords);
  At<TitleState>(*At<u32_be>(kTitleState))->unknown_36 = 0;
  for (uint32_t i = 0; i < 4; i++) {
    *guest::Element<u32_be>(kPlayerFlags, i) = 1;
  }
  DeleteSave(g, 0);
  CreateSave(g, 0);
  ResetControllers();
}

// The defaults' tables' getters.
extern "C" REX_FUNC(sub_820CCA60) {
  SBX_SAVE_DATA_FALLBACK(sub_820CCA60)
  ctx.r3.u64 = kNewGameTable60;
}

extern "C" REX_FUNC(sub_820CCA70) {
  SBX_SAVE_DATA_FALLBACK(sub_820CCA70)
  ctx.r3.u64 = kUnlockedTable;
}

extern "C" REX_FUNC(sub_820CCA80) {
  SBX_SAVE_DATA_FALLBACK(sub_820CCA80)
  ctx.r3.u64 = kLeftOutTable;
}

extern "C" REX_FUNC(sub_820CCA90) {
  SBX_SAVE_DATA_FALLBACK(sub_820CCA90)
  ctx.r3.u64 = kNewGameTableA;
}

extern "C" REX_FUNC(sub_820CCAA0) {
  SBX_SAVE_DATA_FALLBACK(sub_820CCAA0)
  ctx.r3.u64 = kNewGameTableB;
}

// SaveData::NewGame: r3 the save. The save cleared (Progress::Clear) and a
// new game's state set, in the original's order: the tables from the
// getters copied in (memcpy), the car unlocks less the left-out ones
// (worked out in the original's 176-byte frame at +80), the first unlock
// word's bits 0x6BFDF3 set, the 400 bits set but 310 to 375, and +6088
// from [0x826B5870]. Returns what BitSet400::SetAll leaves. Called from
// unlock_cut_cars' override (unlock_cut_cars.cpp), which sets the cut
// cars' bits after it.
void hooks::NativeSaveData_NewGame(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_82113FB0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  SaveData* save = At<SaveData>(ctx.r3.u32);
  guest::StackFrame frame(ctx, 176);
  u32_be* unlocked = At<u32_be>(frame.Local(80));

  g.Call(sub_82113D28, save);
  save->unknown_0 = 1;
  g.Call(sub_8258CB90, save->unknown_104552, g.Call(sub_820CCA60), 60u);  // memcpy
  g.Call(sub_8258CB90, save->unknown_104672, g.Call(sub_820CCA90), 28u);
  g.Call(sub_8258CB90, save->unknown_104700, g.Call(sub_820CCAA0), 28u);
  save->unknown_104728 = 0x703;
  save->unknown_104740 = 0;
  save->unknown_104736 = 0xCF0F;

  g.Call(sub_8258CB90, unlocked, g.Call(sub_820CCA70), 60u);
  const u32_be* left_out = At<u32_be>(g.Call(sub_820CCA80));
  for (int i = 0; i < 15; i++) {
    unlocked[i] = unlocked[i] & ~uint32_t(left_out[i]);
  }
  g.Call(sub_8258CB90, save->unknown_104612, unlocked, 60u);
  save->car_unlocks = save->car_unlocks | 0x6BFDF3u;

  g.Call(sub_821134F8, save->bits_109596);
  for (uint32_t bit = 310; bit <= 375; bit++) {
    save->bits_109596[bit >> 5] &= ~(1u << (bit & 31));
  }
  save->stats.unknown_6040 = *At<u32_be>(kNewGameWord);
}
