// The car models' loading pieces as our own C++ (--native_car_models, on by
// default): a load slot's reset and the TGA reader's constructor and
// clears.

#include "game/std_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_car_models, true, "SBX",
                    "Our own C++ for the car models' load slot reset and the TGA reader's "
                    "set-up instead of the recompiled functions");

namespace {

using game::String;
using guest::At;
using guest::Guest;
using guest::i32_be;
using guest::u16_be;
using guest::u32_be;

constexpr uint32_t kEmptyText = 0x820021B0;  // "".

struct LoadSlot {
  u32_be unknown_0;
  i32_be unknown_4;
  u32_be unknown_8[4];
};

// A TGA file's header, as the game keeps it (aligned).
struct TGAHeader {
  uint8_t id_length;
  uint8_t color_map_type;
  uint8_t image_type;
  uint8_t pad_3;
  u16_be color_map_first;
  u16_be color_map_length;
  uint8_t color_map_depth;
  uint8_t pad_9;
  u16_be x_origin;
  u16_be y_origin;
  u16_be width;
  u16_be height;
  uint8_t bits_per_pixel;
  uint8_t descriptor;
};
GUEST_SIZE(TGAHeader, 20);

struct TGAImage {
  u32_be unknown_0;
  u32_be unknown_4[6];
};

struct TGAReader {
  String name;
  u32_be unknown_28;
  u32_be unknown_32;
  TGAHeader header;
  TGAImage image;
};
GUEST_OFFSET(TGAReader, header, 36);
GUEST_OFFSET(TGAReader, image, 56);

void ClearHeader(TGAHeader* header) {
  header->id_length = 0;
  header->color_map_type = 0;
  header->image_type = 0;
  header->color_map_first = 0;
  header->color_map_length = 0;
  header->color_map_depth = 0;
  header->x_origin = 0;
  header->y_origin = 0;
  header->width = 0;
  header->height = 0;
  header->bits_per_pixel = 0;
  header->descriptor = 0;
}

void ClearImage(TGAImage* image) {
  for (u32_be& word : image->unknown_4) {
    word = 0;
  }
}

bool On() { return REXCVAR_GET(native_car_models); }

}  // namespace

#define SBX_CAR_MODELS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  Guest g(ctx, base);

// LoadSlot::Reset: r3 the slot. Returns it.
extern "C" REX_FUNC(sub_820D04F0) {
  SBX_CAR_MODELS_FALLBACK(sub_820D04F0)
  LoadSlot* slot = At<LoadSlot>(ctx.r3.u32);
  slot->unknown_0 = 0;
  slot->unknown_4 = -1;
  for (u32_be& word : slot->unknown_8) {
    word = 0;
  }
}

// TGAReader::TGAReader: r3 the reader; an empty name and a clear header
// and image. Returns it.
extern "C" REX_FUNC(sub_820EE9C8) {
  SBX_CAR_MODELS_FALLBACK(sub_820EE9C8)
  TGAReader* reader = At<TGAReader>(ctx.r3.u32);
  g.Call(sub_820913F0, &reader->name, 0u, 0u);  // string::_Tidy
  ClearHeader(&reader->header);
  ClearImage(&reader->image);
  g.Call(sub_820918D0, &reader->name, kEmptyText, 0u);  // string::assign
  reader->unknown_32 = 0;
  ctx.r3.u64 = guest::AddressOf(reader);
}

// TGAHeader::Clear and TGAImage::Clear: r3 the header or image. Return r3.
extern "C" REX_FUNC(sub_820EEA30) {
  SBX_CAR_MODELS_FALLBACK(sub_820EEA30)
  ClearHeader(At<TGAHeader>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_820EEBA8) {
  SBX_CAR_MODELS_FALLBACK(sub_820EEBA8)
  ClearImage(At<TGAImage>(ctx.r3.u32));
}
