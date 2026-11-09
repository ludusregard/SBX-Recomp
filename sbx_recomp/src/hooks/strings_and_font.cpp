// The string table as our own C++ (--native_strings_and_font, on by
// default): the table's set-up and release, its load checks (and the
// font's), a string by its id, and the pages of a text.
//
// Written from SBX's generated code. The table is at [0x826B5914] (2,256
// bytes of pages: +0 the 17 files, +68 their loaded flags, +136 the mask of
// files asked for, +2188 the 17 counts); the font at [0x826B58E0] (+12 the
// mask of pages asked for, +20 the four pages, each loaded at its +24).
// DataFile::GetEntry, wcscat, VirtualAlloc and VirtualFree stay
// recompiled, and so do the font's glyph, layout and draw code and the
// string formatting (swprintf).

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_strings_and_font, true, "SBX",
                    "Our own C++ for the string table (its set-up, release, load checks, a "
                    "string by its id, a text's pages) instead of the recompiled functions; the "
                    "font's drawing and the formatting stay recompiled");

namespace {

using guest::LoadU16;
using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kFont = 0x826B58E0;     // [it] the font
constexpr uint32_t kStrings = 0x826B5914;  // [it] the string table
constexpr uint32_t kBounds = 0x82007DB0;   // the files' first ids, 18 words
constexpr uint32_t kFiles = 17;
constexpr uint32_t kTableBytes = 2256;
constexpr uint32_t kCounts = 2188;
constexpr uint32_t kLoaded = 68;
constexpr uint32_t kMask = 136;
constexpr uint16_t kPageBreak = 0xFFD1;

// Font::IsLoaded (sub_821082C0): 1 when a page is asked for and each one
// asked for is loaded.
uint32_t FontIsLoaded(uint8_t* base, uint32_t font) {
  const uint32_t mask = LoadU32(base, font + 12);
  uint32_t asked = 0;
  for (uint32_t i = 0; i < 4; i++) {
    if ((mask & (1u << i)) == 0) {
      continue;
    }
    asked++;
    if (LoadU32(base, LoadU32(base, font + 20 + 4 * i) + 24) == 0) {
      return 0;
    }
  }
  return asked != 0 ? 1 : 0;
}

// Strings::IsLoaded (sub_8210A898): 1 when a file is asked for, each one
// asked for is loaded, and the font is.
uint32_t StringsIsLoaded(uint8_t* base, uint32_t table) {
  const uint32_t mask = LoadU32(base, table + kMask);
  bool all = true;
  int32_t asked = 0;
  for (uint32_t i = 0; i < kFiles; i++) {
    if ((mask & (1u << i)) == 0) {
      continue;
    }
    if (LoadU32(base, table + kLoaded + 4 * i) == 0) {
      all = false;
    }
    asked++;
  }
  if (asked <= 0 || !all) {
    return 0;
  }
  return FontIsLoaded(base, LoadU32(base, kFont)) != 0 ? 1 : 0;
}

bool On() { return REXCVAR_GET(native_strings_and_font); }

}  // namespace

// Strings::Init: the table made once (VirtualAlloc, sub_82292DD8), its
// files and counts and mask cleared.
extern "C" REX_FUNC(sub_8210A990) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8210A990(ctx, base);
    return;
  }
  if (LoadU32(base, kStrings) != 0) {
    return;
  }
  ctx.r3.u64 = 0;
  ctx.r4.u64 = kTableBytes;
  ctx.r5.u64 = 4096;  // MEM_COMMIT
  ctx.r6.u64 = 4;     // PAGE_READWRITE
  sub_82292DD8(ctx, base);
  const uint32_t table = ctx.r3.u32;
  StoreU32(base, kStrings, table);
  for (uint32_t i = 0; i < kFiles; i++) {
    StoreU32(base, table + 4 * i, 0);
    StoreU32(base, table + kCounts + 4 * i, 0);
  }
  StoreU32(base, table + kMask, 0);
}

// Strings::OnLoaded: the archive's callback; r6 the file's loaded flag.
extern "C" REX_FUNC(sub_8210A980) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8210A980(ctx, base);
    return;
  }
  StoreU32(base, ctx.r6.u32, 1);
}

// Strings::Release: r3 the table, r4 a mask of files; each loaded one's
// count down by one, and at 0 the file freed (VirtualFree, sub_82292D70),
// its word and loaded flag cleared and its bit taken out of the mask.
extern "C" REX_FUNC(sub_8210A668) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8210A668(ctx, base);
    return;
  }
  const uint32_t table = ctx.r3.u32, mask = ctx.r4.u32;
  for (uint32_t i = 0; i < kFiles; i++) {
    const uint32_t bit = 1u << i;
    const uint32_t file = LoadU32(base, table + 4 * i);
    if ((bit & mask) == 0 || file == 0) {
      continue;
    }
    const uint32_t count = LoadU32(base, table + kCounts + 4 * i) - 1;
    StoreU32(base, table + kCounts + 4 * i, count);
    if (count != 0) {
      continue;
    }
    ctx.r3.u64 = file;
    ctx.r4.u64 = 0;
    ctx.r5.u64 = 0x8000;  // MEM_RELEASE
    sub_82292D70(ctx, base);
    StoreU32(base, table + 4 * i, 0);
    StoreU32(base, table + kLoaded + 4 * i, 0);
    StoreU32(base, table + kMask, LoadU32(base, table + kMask) & ~bit);
  }
}

extern "C" REX_FUNC(sub_821082C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821082C0(ctx, base);
    return;
  }
  ctx.r3.u64 = FontIsLoaded(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_8210A898) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8210A898(ctx, base);
    return;
  }
  ctx.r3.u64 = StringsIsLoaded(base, ctx.r3.u32);
}

// Text::IsLoaded: the table and the font.
extern "C" REX_FUNC(sub_8220FA18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8220FA18(ctx, base);
    return;
  }
  ctx.r3.u64 = StringsIsLoaded(base, LoadU32(base, kStrings)) != 0 &&
                       FontIsLoaded(base, LoadU32(base, kFont)) != 0
                   ? 1
                   : 0;
}

// Strings::Get: r3 the table, r4 the id (its low 16 bits). The file whose
// next bound is above the id (signed), and its entry id - its bound
// (DataFile::GetEntry, sub_82292170, r5 = 0: the UTF-16 string). An id past
// the last bound leaves r3 as given.
extern "C" REX_FUNC(sub_8210A930) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8210A930(ctx, base);
    return;
  }
  const int32_t id = int32_t(ctx.r4.u32 & 0xFFFF);
  for (uint32_t i = 0; i < kFiles; i++) {
    if (id < int32_t(LoadU32(base, kBounds + 4 + 4 * i))) {
      ctx.r3.u64 = LoadU32(base, ctx.r3.u32 + 4 * i);
      ctx.r4.u64 = uint64_t(int64_t(id - int32_t(LoadU32(base, kBounds + 4 * i))));
      ctx.r5.u64 = 0;
      sub_82292170(ctx, base);
      return;
    }
  }
}

// Strings::PageCount: r4 the text; its page breaks (0xFFD1).
extern "C" REX_FUNC(sub_8210A6F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8210A6F0(ctx, base);
    return;
  }
  uint32_t count = 0;
  for (uint32_t text = ctx.r4.u32;; text += 2) {
    const uint16_t c = LoadU16(base, text);
    if (c == 0) {
      break;
    }
    if (c == kPageBreak) {
      count++;
    }
  }
  ctx.r3.u64 = count;
}

// Strings::CopyPage: r4 out, r5 the text, r6 the page, r7 the lines to
// skip. Past the page's break, each character up to the next break or the
// end appended to `out` (wcscat, sub_8258D678, of the one-character string
// in the original's 128-byte frame at +80) once `skip` lines are past.
// Returns out.
extern "C" REX_FUNC(sub_8210A718) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8210A718(ctx, base);
    return;
  }
  const uint32_t out = ctx.r4.u32;
  uint32_t text = ctx.r5.u32;
  int32_t page = int32_t(ctx.r6.u32);
  int32_t skip = int32_t(ctx.r7.u32);
  while (page > 0) {
    const uint16_t c = LoadU16(base, text);
    if (c == 0) {
      break;
    }
    if (c == kPageBreak) {
      page--;
    }
    text += 2;
  }
  guest::StackFrame frame(ctx, 128);
  const uint32_t one = frame.Local(80);
  for (;;) {
    const uint16_t c = LoadU16(base, text);
    if (c == 0 || c == kPageBreak) {
      break;
    }
    if (skip <= 0) {
      guest::StoreU16(base, one + 0, c);
      guest::StoreU16(base, one + 2, 0);
      ctx.r3.u64 = out;
      ctx.r4.u64 = one;
      sub_8258D678(ctx, base);
    }
    if (LoadU16(base, text) == 10) {
      skip--;
    }
    text += 2;
  }
  ctx.r3.u64 = out;
}
