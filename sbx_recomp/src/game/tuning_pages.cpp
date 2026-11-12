// The tuning pages as our own C++ (--native_tuning_pages, on by default):
// the decal images' sticker movies restored and the vinyl page's busy
// icon set up.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_tuning_pages, true, "SBX",
                    "Our own C++ for the tuning pages' sticker movies and vinyl busy icon "
                    "instead of the recompiled functions");

namespace {

using guest::Guest;
using guest::u32_be;

// The sticker movies' textures: each sticker's first texture is restored
// with kind 3, the one after it with kind 2.
constexpr uint32_t kStickerTextures[16] = {
    159, 161, 163, 165, 167, 169, 173, 176, 178, 180, 182, 190, 192, 194, 196, 198,
};
constexpr uint32_t kFirstTextureKind = 3;
constexpr uint32_t kSecondTextureKind = 2;

struct TuningScreen {
  uint8_t unknown_0[10016];
  u32_be busy_icon[5];  // The vinyl page's busy icon's set-up.
};

bool On() { return REXCVAR_GET(native_tuning_pages); }

void RestoreMovieTexture(Guest& g, uint32_t images, uint32_t kind, uint32_t texture) {
  g.Call(sub_82205FA0, images, kind, texture);  // DecalImages::RestoreMovieTexture
}

}  // namespace

#define SBX_TUNING_PAGES_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                    \
  if (!On()) {                            \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// DecalImages::RestoreStickerMovie: r3 the decal images.
extern "C" REX_FUNC(sub_821F50F8) {
  SBX_TUNING_PAGES_FALLBACK(sub_821F50F8)
  const uint32_t images = ctx.r3.u32;
  for (uint32_t texture : kStickerTextures) {
    RestoreMovieTexture(g, images, kFirstTextureKind, texture);
  }
  for (uint32_t texture : kStickerTextures) {
    RestoreMovieTexture(g, images, kSecondTextureKind, texture + 1);
  }
}

// TuningScreen::SetUpVinylBusyIcon: r3 the screen. Returns it.
extern "C" REX_FUNC(sub_821FC6A0) {
  SBX_TUNING_PAGES_FALLBACK(sub_821FC6A0)
  TuningScreen* screen = guest::At<TuningScreen>(ctx.r3.u32);
  static constexpr uint32_t kBusyIcon[5] = {0, 13, 155, 154, 156};
  for (int i = 0; i < 5; i++) {
    screen->busy_icon[i] = kBusyIcon[i];
  }
}

// DecalImages::RestoreMovieTexture: r3 the images, r4 the kind, r5 the
// texture. The movie brought back (recompiled), then the texture set.
extern "C" REX_FUNC(sub_82205FA0) {
  SBX_TUNING_PAGES_FALLBACK(sub_82205FA0)
  const uint32_t images = ctx.r3.u32, kind = ctx.r4.u32, texture = ctx.r5.u32;
  g.Call(sub_82202780, images);
  g.Call(sub_821F6CD8, images, kind, texture);
}
