// The race screens' small parts: a screen's pages, its menu lists, and the
// menus and dialog the race's screens keep (each a global, made with
// VirtualAlloc).

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

// A screen's pages: the page shown, the next one, and a pending pair (a
// page and the one after it; -1 none).
struct ScreenPages {
  i32_be page;
  i32_be next;
  i32_be unknown_8;
  i32_be pending;
  i32_be pending_after;
  i32_be unknown_20;
  u32_be unknown_24;
};
GUEST_SIZE(ScreenPages, 28);

struct MenuList {
  u32_be count;
  u32_be unknown_4[2];
  u32_be shown;  // The most shown at once.
  u32_be selection;
  u32_be previous_selection;
  u32_be unknown_24[2];
  uint8_t unknown_32[4];
  u32_be unknown_36[6];  // MenuList::Init's r7 to r10 and two stack words.
  uint8_t unknown_60[4];
  u32_be unknown_64;  // MenuList::Init's r6.
  u32_be enabled;     // The address of its entries' flags (a word each).
};
GUEST_OFFSET(MenuList, shown, 12);
GUEST_OFFSET(MenuList, unknown_36, 36);
GUEST_OFFSET(MenuList, selection, 16);
GUEST_OFFSET(MenuList, enabled, 68);

struct PauseMenu {
  uint8_t unknown_0[140];
  u32_be answer_page;
};
GUEST_OFFSET(PauseMenu, answer_page, 140);

struct AfterRaceMenu {
  uint8_t unknown_0[12];
  u32_be answer_page;
};
GUEST_OFFSET(AfterRaceMenu, answer_page, 12);

struct RaceDialog {
  i32_be choice;
  uint8_t unknown_4[28];
  u32_be unknown_32;
};
GUEST_OFFSET(RaceDialog, unknown_32, 32);

struct CareerResults {
  uint8_t unknown_0[184];
  ScreenPages pages;
};
GUEST_OFFSET(CareerResults, pages, 184);
GUEST_SIZE(CareerResults, 212);

}  // namespace game
