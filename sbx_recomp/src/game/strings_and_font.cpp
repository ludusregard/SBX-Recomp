// The string table as our own C++ (--native_strings_and_font, on by
// default): its set-up and release, the load checks (the font's too), a
// string by its id, and a text's pages (game/strings.h).
//
// The font's glyph, layout and draw code and the formatting (swprintf)
// stay recompiled.

#include "game/strings.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_strings_and_font, true, "SBX",
                    "Our own C++ for the string table (its set-up, release, load checks, a "
                    "string by its id, a text's pages) instead of the recompiled functions; the "
                    "font's drawing and the formatting stay recompiled");

namespace {

using game::Font;
using game::StringTable;
using guest::AddressOf;
using guest::At;
using guest::Guest;
using guest::i32_be;
using guest::u16_be;
using guest::u32_be;

constexpr uint32_t kMemCommit = 0x1000;
constexpr uint32_t kMemRelease = 0x8000;
constexpr uint32_t kPageReadWrite = 4;

// The font pages and string files Text::Load asks for.
constexpr uint32_t kTextFontPages = 0xF;
constexpr uint32_t kTextStringFiles = 0x8001;

bool On() { return REXCVAR_GET(native_strings_and_font); }

Font* TheFont() { return At<Font>(*At<u32_be>(game::kFont)); }
StringTable* TheTable() { return At<StringTable>(*At<u32_be>(game::kStringTable)); }

// Font::IsLoaded: whether a page is asked for and each one asked for is in.
bool FontIsLoaded(Font* font) {
  int asked = 0;
  for (int i = 0; i < 4; i++) {
    if ((font->page_mask & (1u << i)) == 0) {
      continue;
    }
    asked++;
    if (font->pages[i]->loaded == 0) {
      return false;
    }
  }
  return asked != 0;
}

// Strings::IsLoaded: whether a file is asked for, each one asked for is
// in, and the font is.
bool TableIsLoaded(StringTable* table) {
  bool all = true;
  int asked = 0;
  for (int i = 0; i < StringTable::kFiles; i++) {
    if ((table->file_mask & (1u << i)) == 0) {
      continue;
    }
    if (table->loaded[i] == 0) {
      all = false;
    }
    asked++;
  }
  return asked > 0 && all && FontIsLoaded(TheFont());
}

}  // namespace

#define SBX_STRINGS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();               \
  if (!On()) {                       \
    __imp__##name(ctx, base);        \
    return;                          \
  }                                  \
  Guest g(ctx, base);

// Strings::Init: the table made once, its files, counts and mask cleared.
extern "C" REX_FUNC(sub_8210A990) {
  SBX_STRINGS_FALLBACK(sub_8210A990)
  u32_be* holder = At<u32_be>(game::kStringTable);
  if (*holder != 0) {
    return;
  }
  *holder = g.Call(sub_82292DD8, 0u, uint32_t(sizeof(StringTable)), kMemCommit,
                   kPageReadWrite);  // VirtualAlloc
  StringTable* table = TheTable();
  for (int i = 0; i < StringTable::kFiles; i++) {
    table->files[i] = 0;
    table->use_counts[i] = 0;
  }
  table->file_mask = 0;
}

// Strings::OnLoaded: the archive's callback; r6 the file's loaded flag.
extern "C" REX_FUNC(sub_8210A980) {
  SBX_STRINGS_FALLBACK(sub_8210A980)
  *At<u32_be>(ctx.r6.u32) = 1;
}

// Strings::Release: r3 the table, r4 a mask of files. Each one's use count
// down by one; at 0 it's freed.
extern "C" REX_FUNC(sub_8210A668) {
  SBX_STRINGS_FALLBACK(sub_8210A668)
  StringTable* table = At<StringTable>(ctx.r3.u32);
  const uint32_t mask = ctx.r4.u32;
  for (int i = 0; i < StringTable::kFiles; i++) {
    const uint32_t bit = 1u << i;
    if ((mask & bit) == 0 || table->files[i] == 0) {
      continue;
    }
    table->use_counts[i] -= 1;
    if (table->use_counts[i] != 0) {
      continue;
    }
    g.Call(sub_82292D70, table->files[i], 0u, kMemRelease);  // VirtualFree
    table->files[i] = 0;
    table->loaded[i] = 0;
    table->file_mask &= ~bit;
  }
}

// Font::IsLoaded: r3 the font.
extern "C" REX_FUNC(sub_821082C0) {
  SBX_STRINGS_FALLBACK(sub_821082C0)
  ctx.r3.u64 = FontIsLoaded(At<Font>(ctx.r3.u32));
}

// Strings::IsLoaded: r3 the table.
extern "C" REX_FUNC(sub_8210A898) {
  SBX_STRINGS_FALLBACK(sub_8210A898)
  ctx.r3.u64 = TableIsLoaded(At<StringTable>(ctx.r3.u32));
}

// Text::IsLoaded: the table and the font.
extern "C" REX_FUNC(sub_8220FA18) {
  SBX_STRINGS_FALLBACK(sub_8220FA18)
  ctx.r3.u64 = TableIsLoaded(TheTable()) && FontIsLoaded(TheFont());
}

// Strings::Get: r3 the table, r4 the string's id (its low 16 bits). The
// string (UTF-16) from the file whose ids it's among. An id past the last
// file leaves r3 as given.
extern "C" REX_FUNC(sub_8210A930) {
  SBX_STRINGS_FALLBACK(sub_8210A930)
  StringTable* table = At<StringTable>(ctx.r3.u32);
  const i32_be* first_ids = At<i32_be>(game::kStringFileFirstIds);
  const int32_t id = int32_t(ctx.r4.u32 & 0xFFFF);
  for (int i = 0; i < StringTable::kFiles; i++) {
    if (id < first_ids[i + 1]) {
      g.Call(sub_82292170, table->files[i], id - first_ids[i], 0u);  // DataFile::GetEntry
      return;
    }
  }
}

// Strings::PageCount: r4 the text; how many page breaks it has.
extern "C" REX_FUNC(sub_8210A6F0) {
  SBX_STRINGS_FALLBACK(sub_8210A6F0)
  uint32_t count = 0;
  for (const u16_be* c = At<u16_be>(ctx.r4.u32); *c != 0; c++) {
    if (*c == game::kPageBreak) {
      count++;
    }
  }
  ctx.r3.u64 = count;
}

// Strings::CopyPage: r4 out, r5 the text, r6 the page, r7 the lines to
// skip. The page's characters (up to the next break) appended to `out`,
// from the line after the skipped ones. Returns out.
extern "C" REX_FUNC(sub_8210A718) {
  SBX_STRINGS_FALLBACK(sub_8210A718)
  u16_be* out = At<u16_be>(ctx.r4.u32);
  const u16_be* text = At<u16_be>(ctx.r5.u32);
  int32_t page = ctx.r6.s32;
  int32_t skip = ctx.r7.s32;
  while (page > 0 && *text != 0) {
    if (*text == game::kPageBreak) {
      page--;
    }
    text++;
  }
  u16_be* end = out;
  while (*end != 0) {
    end++;
  }
  for (; *text != 0 && *text != game::kPageBreak; text++) {
    if (skip <= 0) {
      *end++ = *text;
      *end = 0;
    }
    if (*text == '\n') {
      skip--;
    }
  }
  ctx.r3.u64 = AddressOf(out);
}

// Text::Draw: the font's draw (recompiled).
extern "C" REX_FUNC(sub_8210A888) {
  SBX_STRINGS_FALLBACK(sub_8210A888)
  g.Call(sub_8210A530, TheFont());
}

// Text::Load: the font's pages and the text's string files asked for.
extern "C" REX_FUNC(sub_8210AF20) {
  SBX_STRINGS_FALLBACK(sub_8210AF20)
  g.Call(sub_821095B8, TheFont(), kTextFontPages);    // Font::Load
  g.Call(sub_8210AA00, TheTable(), kTextStringFiles);  // Strings::Load
}
