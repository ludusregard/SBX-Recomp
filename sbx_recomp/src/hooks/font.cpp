// The game's text font (docs/game_data.md, "Font"): a glyph table and a
// TSPFONT sheet in one container, BUILD 22656.
//
// Font::LoadSheet (sub_82109280) fills the font's description from the
// container: +0 the table's header (cell width and height, u16 each), +4 the
// glyph entries, +8 the XPR, +12 the texture, +16 and +20 the texture's width
// and height (its level 0 description, sub_822A3160), +24 1.
//
// The glyph code (Font::AddGlyph, sub_82109680; Font::ClipGlyph,
// sub_82108DB0) works out each glyph's texture coordinates in the sheet's
// texels: columns = width / cell width, u = column * cell width / width,
// v = row * cell height / height. The cell size also sets the glyph's size on
// screen, so it has to stay as it is. A sheet drawn at N times the original's
// size (576 x 928) is given the original's size here instead of its own, so
// the coordinates come out as the same fractions of the larger sheet and the
// text keeps its size and spacing. Any other sheet is left as loaded.

#include "guest.h"

#include <rex/logging.h>

namespace {

// The original sheet's size.
constexpr uint32_t kSheetWidth = 576;
constexpr uint32_t kSheetHeight = 928;

// Offsets in the font's description.
constexpr uint32_t kTextureWidth = 16;
constexpr uint32_t kTextureHeight = 20;

}  // namespace

// Font::LoadSheet: r3 the font, r4 the container, r6 the description.
extern "C" REX_FUNC(sub_82109280) {
  REX_FUNC_PROLOGUE();
  const uint32_t description = ctx.r6.u32;
  __imp__sub_82109280(ctx, base);

  const uint32_t width = guest::LoadU32(base, description + kTextureWidth);
  const uint32_t height = guest::LoadU32(base, description + kTextureHeight);
  if (width == kSheetWidth && height == kSheetHeight) {
    return;
  }
  const uint32_t scale = width / kSheetWidth;
  if (scale < 2 || width != kSheetWidth * scale || height != kSheetHeight * scale) {
    REXLOG_INFO("font: sheet is {}x{}, not a multiple of {}x{}; left as loaded", width, height,
                kSheetWidth, kSheetHeight);
    return;
  }
  // PATCH: the glyph coordinates are worked out on the original sheet's size.
  guest::StoreU32(base, description + kTextureWidth, kSheetWidth);
  guest::StoreU32(base, description + kTextureHeight, kSheetHeight);
  REXLOG_INFO("font: sheet is {}x{} ({}x); glyphs mapped as on {}x{}", width, height, scale,
              kSheetWidth, kSheetHeight);
}
