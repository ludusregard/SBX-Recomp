// The garage's car records as our own C++ (--native_car_makers, on by
// default): a garage car's reset, the garage cars' static set-up and a car
// record's bytes reset.
//
// Written from SBX's generated code: the stores as tables in their order.
// The garage stage's car set-up, its records' copies and reapplies stay
// recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_car_makers, true, "SBX",
                    "Our own C++ for the garage cars' resets and a car record's bytes instead of the recompiled functions; the garage stage stays recompiled");

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

// GarageCar::Reset (sub_82110370).
// 6 stores
constexpr Fill kGarageCar_Reset0[] = {
    {1884, kImage, 0x82000A24}, {1880, kWord, 0x0}, {1888, kImage, 0x82000A24}, {1892, kWord, 0x0, 2, 4},
    {1960, kWord, 0x1},
};

// GarageCars::StaticInit (sub_825BF2C8).
// 1 stores
constexpr Fill kGarageCars_StaticInit0[] = {
    {0x8280E674, kAbsWord, 0x0},
};
// 1 stores
constexpr Fill kGarageCars_StaticInit1[] = {
    {0x8280EE24, kAbsWord, 0x0},
};

// CarRecord::ResetBytes (sub_820D06C0).
// 11 stores
constexpr Fill kCarRecord_ResetBytes0[] = {
    {12, kWord, 0x1}, {16, kWord, 0x2}, {0, kWord, 0x0, 3, 4}, {20, kWord, 0x3},
    {24, kWord, 0x4}, {28, kWord, 0x5}, {32, kWord, 0x0}, {36, kWord, 0x6},
    {40, kWord, 0x7},
};

bool On() { return REXCVAR_GET(native_car_makers); }

}  // namespace

// GarageCar::Reset.
extern "C" REX_FUNC(sub_82110370) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82110370(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820CD590(ctx, base);
  guest_memory::Apply(base, object, kGarageCar_Reset0, args);
  ctx.r3.u64 = object + 1900;
  ctx.r4.u64 = 0xFFFFFFFF826B2D24u;
  ctx.r5.u64 = 0x3Cu;
  sub_8258CB90(ctx, base);
}

// GarageCars::StaticInit.
extern "C" REX_FUNC(sub_825BF2C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_825BF2C8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = 0xFFFFFFFF8280DEC8u;
  sub_82110370(ctx, base);
  guest_memory::Apply(base, object, kGarageCars_StaticInit0, args);
  ctx.r3.u64 = 0xFFFFFFFF8280E678u;
  sub_82110370(ctx, base);
  guest_memory::Apply(base, object, kGarageCars_StaticInit1, args);
}

// CarRecord::ResetBytes.
extern "C" REX_FUNC(sub_820D06C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D06C0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCarRecord_ResetBytes0, args);
  ctx.r3.u64 = object;
}

