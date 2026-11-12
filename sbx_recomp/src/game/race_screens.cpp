// The race screens' small parts as our own C++ (--native_race_screens, on
// by default): the screens' page steps (go, finish), the menu lists'
// selection and entries, the pause and after-race menus' answers, the race
// dialog's answer and destroy, the results screens' creates and starts. The
// screens themselves (their ticks, draws and transitions) stay recompiled.

#include "game/race_screens.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_screens, true, "SBX",
                    "Our own C++ for the race screens' small parts (page steps, menu lists, "
                    "the menus' and dialog's answers, the results' creates) instead of the "
                    "recompiled functions; the screens' ticks and draws stay recompiled");

namespace {

using game::AfterRaceMenu;
using game::CareerResults;
using game::MenuList;
using game::PauseMenu;
using game::RaceDialog;
using game::ScreenPages;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_race_screens); }

// The race screens' globals (writable): [each] the object.
constexpr uint32_t kPauseMenu = 0x826C7F7C;
constexpr uint32_t kDialogList = 0x826C7F9C;
constexpr uint32_t kDialogPages = 0x826C7FA0;
constexpr uint32_t kRaceDialog = 0x826C7FA4;
constexpr uint32_t kCareerResults = 0x826C7FA8;
constexpr uint32_t kAfterRaceScreen = 0x826C7FB0;
constexpr uint32_t kAfterRaceMenu = 0x826C7FB4;
constexpr uint32_t kAfterRacePages = 0x826C7FB8;
constexpr uint32_t kAfterRaceList = 0x826C7FBC;
constexpr uint32_t kRaceResults = 0x8280DE28;
constexpr uint32_t kSplitResults = 0x8280DE2C;

constexpr uint32_t kMemCommit = 0x1000;
constexpr uint32_t kPageReadWrite = 4;
constexpr uint32_t kMemRelease = 0x8000;

u32_be& Global(uint32_t address) { return *At<u32_be>(address); }

template <typename T>
T* Object(uint32_t address) {
  return At<T>(Global(address));
}

// ScreenPages::Go (sub_82209810): to `page` now (no `next`, -1), or to
// `next` then `after` once this page finishes.
void Go(ScreenPages* pages, int32_t page, int32_t next, int32_t after) {
  if (next == -1) {
    pages->page = page;
    pages->pending = -1;
    pages->pending_after = -1;
    return;
  }
  pages->pending = next;
  pages->pending_after = after;
  pages->next = page;
  pages->page = -1;
}

// VirtualAlloc (sub_82292DD8) of `bytes` into the global at `at`.
void AllocTo(Guest& g, uint32_t bytes, uint32_t at) {
  Global(at) = g.Call(sub_82292DD8, 0u, bytes, kMemCommit, kPageReadWrite);
}

// VirtualFree (sub_82292D70).
void Free(Guest& g, uint32_t pages) { g.Call(sub_82292D70, pages, 0u, kMemRelease); }

}  // namespace

#define SBX_RACE_SCREENS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  Guest g(ctx, base);

// ScreenPages::Init: r3 the pages, r4 the first page (and the next).
extern "C" REX_FUNC(sub_82209840) {
  SBX_RACE_SCREENS_FALLBACK(sub_82209840)
  ScreenPages* pages = At<ScreenPages>(ctx.r3.u32);
  const int32_t page = ctx.r4.s32;
  pages->next = page;
  pages->page = page;
  pages->unknown_8 = page;
  pages->unknown_20 = -1;
  pages->pending = -1;
  pages->unknown_24 = 0;
}

// ScreenPages::Go: r3 the pages, r4 the page, r5 the next (-1: none) and
// r6 the one after.
extern "C" REX_FUNC(sub_82209810) {
  SBX_RACE_SCREENS_FALLBACK(sub_82209810)
  Go(At<ScreenPages>(ctx.r3.u32), ctx.r4.s32, ctx.r5.s32, ctx.r6.s32);
}

// ScreenPages::Finish: r3 the pages; with r4 set, the pending page taken
// (or the next one when none is pending), then +24 = 0.
extern "C" REX_FUNC(sub_822097D0) {
  SBX_RACE_SCREENS_FALLBACK(sub_822097D0)
  ScreenPages* pages = At<ScreenPages>(ctx.r3.u32);
  if (ctx.r4.s32 == 0) {
    return;
  }
  const int32_t after = pages->pending_after;
  if (after == -1) {
    const int32_t next = pages->next;
    pages->pending = -1;
    pages->page = next;
  } else {
    pages->unknown_20 = 0;
    pages->pending = after;
    pages->pending_after = -1;
  }
  pages->unknown_24 = 0;
}

// RaceDialog::Destroy: its list destroyed (sub_82209958), its three blocks
// freed, the globals cleared.
extern "C" REX_FUNC(sub_822159C8) {
  SBX_RACE_SCREENS_FALLBACK(sub_822159C8)
  g.Call(sub_82209958, Global(kDialogList));
  Free(g, Global(kDialogPages));
  Free(g, Global(kDialogList));
  Free(g, Global(kRaceDialog));
  Global(kDialogPages) = 0;
  Global(kDialogList) = 0;
  Global(kRaceDialog) = 0;
}

// AfterRaceMenu::Close: its list destroyed (sub_82209958).
extern "C" REX_FUNC(sub_82218140) {
  SBX_RACE_SCREENS_FALLBACK(sub_82218140)
  g.Call(sub_82209958, Global(kAfterRaceList));
}

// PauseMenu::Close: r3 the menu; sub_8220B3D8 on it.
extern "C" REX_FUNC(sub_8220B5C8) {
  SBX_RACE_SCREENS_FALLBACK(sub_8220B5C8)
  g.Call(sub_8220B3D8, ctx.r3.u32);
}

// MenuList::Selection, SetSelection (the old one kept), SetEnabled and
// IsEnabled (the entry r4's flag): r3 the list.
extern "C" REX_FUNC(sub_8242B828) {
  SBX_RACE_SCREENS_FALLBACK(sub_8242B828)
  ctx.r3.u64 = uint32_t(At<MenuList>(ctx.r3.u32)->selection);
}

extern "C" REX_FUNC(sub_822099C8) {
  SBX_RACE_SCREENS_FALLBACK(sub_822099C8)
  MenuList* list = At<MenuList>(ctx.r3.u32);
  const uint32_t old = list->selection;
  list->selection = ctx.r4.u32;
  list->previous_selection = old;
}

extern "C" REX_FUNC(sub_822099A8) {
  SBX_RACE_SCREENS_FALLBACK(sub_822099A8)
  *guest::Element<u32_be>(At<MenuList>(ctx.r3.u32)->enabled, ctx.r4.u32) = ctx.r5.u32;
}

extern "C" REX_FUNC(sub_822099B8) {
  SBX_RACE_SCREENS_FALLBACK(sub_822099B8)
  ctx.r3.u64 = uint32_t(*guest::Element<u32_be>(At<MenuList>(ctx.r3.u32)->enabled, ctx.r4.u32));
}

// PauseMenu::AnswerPage and AfterRaceMenu::AnswerPage.
extern "C" REX_FUNC(sub_8220A368) {
  SBX_RACE_SCREENS_FALLBACK(sub_8220A368)
  ctx.r3.u64 = uint32_t(Object<PauseMenu>(kPauseMenu)->answer_page);
}

extern "C" REX_FUNC(sub_822183C8) {
  SBX_RACE_SCREENS_FALLBACK(sub_822183C8)
  ctx.r3.u64 = uint32_t(Object<AfterRaceMenu>(kAfterRaceMenu)->answer_page);
}

// RaceDialog::Answer: +32 cleared; 1 when the dialog's choice is 1, else 2.
extern "C" REX_FUNC(sub_82215A38) {
  SBX_RACE_SCREENS_FALLBACK(sub_82215A38)
  Object<RaceDialog>(kRaceDialog)->unknown_32 = 0;
  ctx.r3.u64 = Object<RaceDialog>(kRaceDialog)->choice == 1 ? 1 : 2;
}

// CareerResults::Create, RaceResults::Create, SplitResults::Create: their
// pages (212, 52 and 40 bytes).
extern "C" REX_FUNC(sub_82216E08) {
  SBX_RACE_SCREENS_FALLBACK(sub_82216E08)
  AllocTo(g, sizeof(CareerResults), kCareerResults);
}

extern "C" REX_FUNC(sub_8221CF18) {
  SBX_RACE_SCREENS_FALLBACK(sub_8221CF18)
  AllocTo(g, 52, kRaceResults);
}

extern "C" REX_FUNC(sub_8221D5B8) {
  SBX_RACE_SCREENS_FALLBACK(sub_8221D5B8)
  AllocTo(g, 40, kSplitResults);
}

// AfterRaceMenu::Start: the screen's set-up (sub_82219410), its pages to
// page 1; returns 0.
extern "C" REX_FUNC(sub_82219508) {
  SBX_RACE_SCREENS_FALLBACK(sub_82219508)
  g.Call(sub_82219410, Global(kAfterRaceScreen));
  Go(Object<ScreenPages>(kAfterRacePages), 1, 1, -1);
  ctx.r3.u64 = 0;
}

// CareerResults::Start: its pages to page 1; returns 0.
extern "C" REX_FUNC(sub_82216EA0) {
  SBX_RACE_SCREENS_FALLBACK(sub_82216EA0)
  Go(&Object<CareerResults>(kCareerResults)->pages, 1, 1, -1);
  ctx.r3.u64 = 0;
}
