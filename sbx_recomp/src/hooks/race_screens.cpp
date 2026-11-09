// The race screens' small parts as our own C++ (--native_race_screens, on
// by default): the screens' page steps (go, finish), the menu lists'
// selection and entries, the pause and after-race menus' answers, the race
// dialog's answer and destroy, the results screens' creates and starts.
//
// Written from SBX's generated code. Screen pages: +0 the page, +4 the
// next one, +12 and +16 a pending pair, +20 and +24 cleared on finish. The
// screens themselves (their ticks, draws and transitions) stay recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_screens, true, "SBX",
                    "Our own C++ for the race screens' small parts (page steps, menu lists, "
                    "the menus' and dialog's answers, the results' creates) instead of the "
                    "recompiled functions; the screens' ticks and draws stay recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// ScreenPages::Init (sub_82209840).
// 6 stores
constexpr Fill kScreenPages_Init0[] = {
    {4, kArg, 4}, {0, kArg, 4, 2, 8}, {20, kWord, 0xFFFFFFFF}, {12, kWord, 0xFFFFFFFF},
    {24, kWord, 0x0},
};

// RaceDialog::Destroy (sub_822159C8).
// 3 stores
constexpr Fill kRaceDialog_Destroy0[] = {
    {0x826C7FA0, kAbsWord, 0x0}, {0x826C7F9C, kAbsWord, 0x0, 2, 8},
};

bool On() { return REXCVAR_GET(native_race_screens); }

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kPauseMenu = 0x826C7F7C;
constexpr uint32_t kAfterRaceMenu = 0x826C7FB4;
constexpr uint32_t kRaceDialog = 0x826C7FA4;
constexpr uint32_t kCareerResults = 0x826C7FA8;
constexpr uint32_t kAfterRaceScreen = 0x826C7FB0;
constexpr uint32_t kAfterRacePages = 0x826C7FB8;
constexpr uint32_t kRaceResults = 0x8280DE28;
constexpr uint32_t kSplitResults = 0x8280DE2C;

// ScreenPages::Go (sub_82209810): r4 the page, r5 the next (-1: none) and
// r6 its own.
void Go(uint8_t* base, uint32_t pages, uint32_t page, uint32_t next, uint32_t after) {
  if (int32_t(next) == -1) {
    StoreU32(base, pages + 0, page);
    StoreU32(base, pages + 12, 0xFFFFFFFFu);
    StoreU32(base, pages + 16, 0xFFFFFFFFu);
    return;
  }
  StoreU32(base, pages + 12, next);
  StoreU32(base, pages + 16, after);
  StoreU32(base, pages + 4, page);
  StoreU32(base, pages + 0, 0xFFFFFFFFu);
}

// VirtualAlloc (sub_82292DD8) of `bytes` into the word at `at`.
void AllocTo(PPCContext& ctx, uint8_t* base, uint32_t bytes, uint32_t at) {
  ctx.r3.u64 = 0;
  ctx.r4.u64 = bytes;
  ctx.r5.u64 = 4096;  // MEM_COMMIT
  ctx.r6.u64 = 4;     // PAGE_READWRITE
  sub_82292DD8(ctx, base);
  StoreU32(base, at, ctx.r3.u32);
}

}  // namespace

// ScreenPages::Init.
extern "C" REX_FUNC(sub_82209840) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82209840(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kScreenPages_Init0, args);
  ctx.r3.u64 = object;
}

// RaceDialog::Destroy.
extern "C" REX_FUNC(sub_822159C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822159C8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = guest::LoadU32(base, 0x826C7F9C);
  sub_82209958(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826C7FA0);
  ctx.r4.u64 = 0x0u;
  ctx.r5.u64 = 0x8000u;
  sub_82292D70(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826C7F9C);
  ctx.r4.u64 = 0x0u;
  ctx.r5.u64 = 0x8000u;
  sub_82292D70(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, 0x826C7FA4);
  ctx.r4.u64 = 0x0u;
  ctx.r5.u64 = 0x8000u;
  sub_82292D70(ctx, base);
  guest_memory::Apply(base, object, kRaceDialog_Destroy0, args);
}

// AfterRaceMenu::Close.
extern "C" REX_FUNC(sub_82218140) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82218140(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, 0x826C7FBC);
  sub_82209958(ctx, base);
}

// PauseMenu::Close.
extern "C" REX_FUNC(sub_8220B5C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8220B5C8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_8220B3D8(ctx, base);
}


extern "C" REX_FUNC(sub_82209810) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82209810(ctx, base);
    return;
  }
  Go(base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32);
}

// ScreenPages::Finish: with r4 set, the pending page taken (or the next
// one when none is pending), then +24 = 0.
extern "C" REX_FUNC(sub_822097D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822097D0(ctx, base);
    return;
  }
  const uint32_t pages = ctx.r3.u32;
  if (int32_t(ctx.r4.u32) == 0) {
    return;
  }
  const uint32_t pending = LoadU32(base, pages + 16);
  if (int32_t(pending) == -1) {
    const uint32_t next = LoadU32(base, pages + 4);
    StoreU32(base, pages + 12, 0xFFFFFFFFu);
    StoreU32(base, pages + 0, next);
  } else {
    StoreU32(base, pages + 20, 0);
    StoreU32(base, pages + 12, pending);
    StoreU32(base, pages + 16, 0xFFFFFFFFu);
  }
  StoreU32(base, pages + 24, 0);
}

// MenuList::SetSelection: +16 = r4, +20 the old one.
extern "C" REX_FUNC(sub_822099C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822099C8(ctx, base);
    return;
  }
  const uint32_t list = ctx.r3.u32;
  const uint32_t old = LoadU32(base, list + 16);
  StoreU32(base, list + 16, ctx.r4.u32);
  StoreU32(base, list + 20, old);
}

// MenuList::SetEnabled and IsEnabled: the entry r4's flag ([+68] + 4 x
// r4).
extern "C" REX_FUNC(sub_822099A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822099A8(ctx, base);
    return;
  }
  StoreU32(base, LoadU32(base, ctx.r3.u32 + 68) + (ctx.r4.u32 << 2), ctx.r5.u32);
}

extern "C" REX_FUNC(sub_822099B8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822099B8(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, LoadU32(base, ctx.r3.u32 + 68) + (ctx.r4.u32 << 2));
}

// PauseMenu::AnswerPage and AfterRaceMenu::AnswerPage.
extern "C" REX_FUNC(sub_8220A368) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8220A368(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, LoadU32(base, kPauseMenu) + 140);
}

extern "C" REX_FUNC(sub_822183C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822183C8(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, LoadU32(base, kAfterRaceMenu) + 12);
}

// RaceDialog::Answer: +32 cleared; 1 when the dialog's choice (+0) is 1,
// else 2.
extern "C" REX_FUNC(sub_82215A38) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82215A38(ctx, base);
    return;
  }
  StoreU32(base, LoadU32(base, kRaceDialog) + 32, 0);
  ctx.r3.u64 = LoadU32(base, LoadU32(base, kRaceDialog)) == 1 ? 1 : 2;
}

// CareerResults::Create, RaceResults::Create, SplitResults::Create: their
// pages (212, 52 and 40 bytes).
extern "C" REX_FUNC(sub_82216E08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82216E08(ctx, base);
    return;
  }
  AllocTo(ctx, base, 212, kCareerResults);
}

extern "C" REX_FUNC(sub_8221CF18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8221CF18(ctx, base);
    return;
  }
  AllocTo(ctx, base, 52, kRaceResults);
}

extern "C" REX_FUNC(sub_8221D5B8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8221D5B8(ctx, base);
    return;
  }
  AllocTo(ctx, base, 40, kSplitResults);
}

// AfterRaceMenu::Start: the screen's set-up (sub_82219410), its pages to
// page 1; returns 0.
extern "C" REX_FUNC(sub_82219508) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82219508(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, kAfterRaceScreen);
  sub_82219410(ctx, base);
  Go(base, LoadU32(base, kAfterRacePages), 1, 1, 0xFFFFFFFFu);
  ctx.r3.u64 = 0;
}

// CareerResults::Start: its pages (+184) to page 1; returns 0.
extern "C" REX_FUNC(sub_82216EA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82216EA0(ctx, base);
    return;
  }
  Go(base, LoadU32(base, kCareerResults) + 184, 1, 1, 0xFFFFFFFFu);
  ctx.r3.u64 = 0;
}
