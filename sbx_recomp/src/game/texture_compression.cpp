// The texture compression's objects as our own C++
// (--native_texture_compression, on by default): the blitter's constructor
// and the image codec's destructor. The encoders stay recompiled.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_texture_compression, true, "SBX",
                    "Our own C++ for the blitter's constructor and the image codec's "
                    "destructor instead of the recompiled functions");

namespace {

using guest::u32_be;

constexpr uint32_t kImageCodecVtable = 0x82038620;

struct Blitter {
  u32_be unknown_0;
  u32_be unknown_4;
};

struct ImageCodec {
  u32_be vtable;
  uint8_t unknown_4[52];
  u32_be buffer_56;  // Three blocks on the heap.
  uint8_t unknown_60[28];
  u32_be buffer_88;
  u32_be buffer_92;
};
GUEST_OFFSET(ImageCodec, buffer_88, 88);

bool On() { return REXCVAR_GET(native_texture_compression); }

}  // namespace

// Blitter::Blitter: r3 the blitter. Returns it.
extern "C" REX_FUNC(sub_8235F798) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8235F798(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  Blitter* blitter = guest::At<Blitter>(ctx.r3.u32);
  blitter->unknown_4 = 0;
  blitter->unknown_0 = 0;
}

// ImageCodec::~ImageCodec: r3 the codec; its blocks deleted.
extern "C" REX_FUNC(sub_823D6F20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_823D6F20(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  ImageCodec* codec = guest::At<ImageCodec>(ctx.r3.u32);
  codec->vtable = kImageCodecVtable;
  for (uint32_t block : {uint32_t(codec->buffer_56), uint32_t(codec->buffer_88),
                         uint32_t(codec->buffer_92)}) {
    g.Call(sub_8258C868, block);  // operator delete
  }
}
