// The load polls' set-ups as our own C++ (--native_load_poll, on by
// default): the wheels' and lights' records and caliper texture, the
// parts', interior's, cockpit's and traffic's paints applied as their
// loads finish, the car model's load checks and the gauges' table.
//
// Written from SBX's generated code: the stores as tables in their order,
// the calls in their place. The polls themselves and the textures' loads
// stay recompiled.
//
// A car model: +3748 its state and its two loads' at +3972 and +3996 (2:
// not loaded), +7256 its record (+8 its kind).

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_load_poll, true, "SBX",
                    "Our own C++ for the scene's records and paints set as the loads finish, and the car model's load checks, instead of the recompiled functions; the polls stay recompiled");

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

// SceneLights::ApplyRecord (sub_820DC838).
// 8 stores
constexpr Fill kSceneLights_ApplyRecord0[] = {
    {104, kWord, 0x0, 8, 4},
};

bool On() { return REXCVAR_GET(native_load_poll); }

using guest::LoadU32;

constexpr uint32_t kGauges = 0x820035E0;  // 16 bytes each, two a gauge

}  // namespace

// Wheels::SetRecord.
extern "C" REX_FUNC(sub_820D8B88) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D8B88(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 28;
  ctx.r5.u64 = 0x9Cu;
  sub_8258CB90(ctx, base);
}

// Wheels::SetCaliperTexture.
extern "C" REX_FUNC(sub_820D8B98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D8B98(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 2336;
  ctx.r4.u64 = guest::LoadU32(base, object + 116);
  ctx.r5.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D1420(ctx, base);
  ctx.r3.u64 = object + 2800;
  ctx.r4.u64 = guest::LoadU32(base, object + 116);
  ctx.r5.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D1420(ctx, base);
}

// SceneLights::SetRecord.
extern "C" REX_FUNC(sub_820DC7E8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DC7E8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 32;
  ctx.r5.u64 = 0x34u;
  sub_8258CB90(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, object + 28);
  ctx.r4.u64 = args[4];
  sub_820D6710(ctx, base);
}

// SceneLights::ApplyRecord.
extern "C" REX_FUNC(sub_820DC838) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DC838(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneLights_ApplyRecord0, args);
  ctx.r3.u64 = object + 0;
  sub_820DC7E8(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, object + 13152);
  ctx.r4.u64 = guest::LoadU32(base, object + 32);
  ctx.r5.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D1420(ctx, base);
}

// SceneParts::ApplyPaint.
extern "C" REX_FUNC(sub_820E04B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E04B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 104);
  sub_820D7038(ctx, base);
}

// SceneInterior::ApplyPaint.
extern "C" REX_FUNC(sub_820E3E18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E3E18(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 36);
  sub_820D74F0(ctx, base);
}

// SceneCockpit::ApplyPaint.
extern "C" REX_FUNC(sub_820DBDA8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DBDA8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = guest::LoadU32(base, object + 32);
  sub_820D5FD0(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, object + 28);
  ctx.r4.u64 = args[4];
  ctx.r5.u64 = args[5];
  ctx.r6.u64 = args[6];
  ctx.r7.u64 = args[7];
  ctx.r8.u64 = args[8];
  ctx.r9.u64 = args[9];
  sub_820E3E18(ctx, base);
}

// SceneTraffic::ApplyPaint.
extern "C" REX_FUNC(sub_820E5DC0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E5DC0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 9760);
  sub_820D7AC8(ctx, base);
}


// GaugeTableEntry: r3 the gauge, r4 the level (3 and up: its second
// entry).
extern "C" REX_FUNC(sub_820D32A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D32A0(ctx, base);
    return;
  }
  const uint32_t second = int32_t(ctx.r4.u32) >= 3 ? 1 : 0;
  ctx.r3.u64 = (((ctx.r3.u32 << 1) + second) << 4) + kGauges;
}

// CarModel::IsReady: CarModel::IsLoaded (sub_820D38C8, a tail call) for a
// record of kind 0 or 1 ([+7256] + 8), else 0.
extern "C" REX_FUNC(sub_820D3CE8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3CE8(ctx, base);
    return;
  }
  const uint32_t model = ctx.r3.u32;
  if (LoadU32(base, LoadU32(base, model + 7256) + 8) > 1) {
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r3.u64 = model;
  sub_820D38C8(ctx, base);
}
