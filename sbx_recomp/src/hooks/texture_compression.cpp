// The texture tools' set-ups as our own C++ (--native_texture_compression,
// on by default): the blitter's constructor and the image codec's
// destructor.
//
// Written from SBX's generated code: the stores as a table, the calls in
// their place. The block encoders (DXT, the codec's), the blitter's
// release and the block dimensions stay recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_texture_compression, true, "SBX",
                    "Our own C++ for the texture blitter's constructor and the image codec's destructor instead of the recompiled functions; the encoders stay recompiled");

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

// Blitter::Blitter (sub_8235F798).
// 2 stores
constexpr Fill kBlitter_Blitter0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x0},
};

// ImageCodec::~ImageCodec (sub_823D6F20).
// 1 stores
constexpr Fill kImageCodec_DestroyImageCodec0[] = {
    {0, kWord, 0x82038620},
};

bool On() { return REXCVAR_GET(native_texture_compression); }

}  // namespace

// Blitter::Blitter.
extern "C" REX_FUNC(sub_8235F798) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8235F798(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kBlitter_Blitter0, args);
  ctx.r3.u64 = object;
}

// ImageCodec::~ImageCodec.
extern "C" REX_FUNC(sub_823D6F20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_823D6F20(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kImageCodec_DestroyImageCodec0, args);
  ctx.r3.u64 = guest::LoadU32(base, object + 56);
  sub_8258C868(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, object + 88);
  sub_8258C868(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, object + 92);
  sub_8258C868(ctx, base);
}

