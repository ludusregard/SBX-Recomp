// The game's string table (the menus' and HUD's text, in UTF-16) and the
// font's pages.

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::ptr;
using guest::u32_be;

// The strings' container files (DataFile), 17 of them, loaded and freed
// by a mask of files with a use count each.
struct StringTable {
  static constexpr int kFiles = 17;

  u32_be files[kFiles];   // Each a DataFile, or 0.
  u32_be loaded[kFiles];  // Set by the archive's callback.
  u32_be file_mask;       // The files asked for.
  uint8_t unknown_140[2048];
  u32_be use_counts[kFiles];
};
GUEST_SIZE(StringTable, 2256);
GUEST_OFFSET(StringTable, file_mask, 136);
GUEST_OFFSET(StringTable, use_counts, 2188);

// The font: up to four pages of glyphs.
struct FontPage {
  uint8_t unknown_0[24];
  u32_be loaded;
};

struct Font {
  uint8_t unknown_0[12];
  u32_be page_mask;  // The pages asked for.
  uint8_t unknown_16[4];
  ptr<FontPage> pages[4];
};
GUEST_OFFSET(Font, pages, 20);

constexpr uint32_t kFont = 0x826B58E0;         // Holds the font's address.
constexpr uint32_t kStringTable = 0x826B5914;  // Holds the table's address.
// Each file's first string id, and the end of the last (18 words).
constexpr uint32_t kStringFileFirstIds = 0x82007DB0;

// The character that starts a new page of a text.
constexpr uint16_t kPageBreak = 0xFFD1;

}  // namespace game
