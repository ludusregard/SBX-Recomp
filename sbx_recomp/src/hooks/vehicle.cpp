// The vehicles as our own C++ (--native_vehicle, on by default): a
// vehicle's construction, refresh and removal from the collision world,
// its route positions each tick (the body's and the four wheels'), a
// route position's copy, and the vehicles' teardown.
//
// Written from SBX's generated code. The route positions' own update
// (RoutePosition::Update, sub_820F8350), the body's getters, the collision
// world, the input, the status and the set-ups stay recompiled.
//
// A vehicle: +336 its body, +340 its collision object, +344 its other
// object, +480 + 368 x wheel the wheels' positions, +5232 its route
// position (its point at +16), +5328 + 96 x wheel the wheels', +6148 a
// flag cleared on removal.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_vehicle, true, "SBX",
                    "Our own C++ for the vehicles (construction, refresh, removal, the route "
                    "positions each tick) instead of the recompiled functions; the route "
                    "positions' update and the vehicle's set-up stay recompiled");

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

// Vehicle::Construct (sub_820C2EF8).
// 20 stores
constexpr Fill kVehicle_Construct0[] = {
    {5260, kImage, 0x82000A20, 4, 16}, {5356, kImage, 0x82000A20, 4, 16}, {5452, kImage, 0x82000A20, 4, 16}, {5548, kImage, 0x82000A20, 4, 16},
    {5644, kImage, 0x82000A20, 4, 16},
};

bool On() { return REXCVAR_GET(native_vehicle); }

using guest::LoadU32;
using guest::StoreU32;

// RoutePosition::Update (sub_820F8350).
void UpdateRoutePosition(PPCContext& ctx, uint8_t* base, uint32_t position) {
  ctx.r3.u64 = position;
  sub_820F8350(ctx, base);
}

}  // namespace

// Vehicle::Refresh.
extern "C" REX_FUNC(sub_820BFF50) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BFF50(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_820B6180(ctx, base);
}

// Vehicles::DestroyAll.
extern "C" REX_FUNC(sub_820C2000) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C2000(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 42960;
  ctx.r4.u64 = 0x60u;
  ctx.r5.u64 = 0x4u;
  ctx.r6.u64 = 0xFFFFFFFF821611F0u;
  sub_820900C0(ctx, base);
  ctx.r3.u64 = object + 37632;
  sub_820B0D58(ctx, base);
  ctx.r3.u64 = object + 36688;
  ctx.r4.u64 = 0x60u;
  ctx.r5.u64 = 0x4u;
  ctx.r6.u64 = 0xFFFFFFFF821611F0u;
  sub_820900C0(ctx, base);
  ctx.r3.u64 = object + 31360;
  sub_820B0D58(ctx, base);
  ctx.r3.u64 = object + 30416;
  ctx.r4.u64 = 0x60u;
  ctx.r5.u64 = 0x4u;
  ctx.r6.u64 = 0xFFFFFFFF821611F0u;
  sub_820900C0(ctx, base);
  ctx.r3.u64 = object + 25088;
  sub_820B0D58(ctx, base);
  ctx.r3.u64 = object + 24144;
  ctx.r4.u64 = 0x60u;
  ctx.r5.u64 = 0x4u;
  ctx.r6.u64 = 0xFFFFFFFF821611F0u;
  sub_820900C0(ctx, base);
  ctx.r3.u64 = object + 18816;
  sub_820B0D58(ctx, base);
  ctx.r3.u64 = object + 17872;
  ctx.r4.u64 = 0x60u;
  ctx.r5.u64 = 0x4u;
  ctx.r6.u64 = 0xFFFFFFFF821611F0u;
  sub_820900C0(ctx, base);
  ctx.r3.u64 = object + 12544;
  sub_820B0D58(ctx, base);
  ctx.r3.u64 = object + 11600;
  ctx.r4.u64 = 0x60u;
  ctx.r5.u64 = 0x4u;
  ctx.r6.u64 = 0xFFFFFFFF821611F0u;
  sub_820900C0(ctx, base);
  ctx.r3.u64 = object + 6272;
  sub_820B0D58(ctx, base);
  ctx.r3.u64 = object + 5328;
  ctx.r4.u64 = 0x60u;
  ctx.r5.u64 = 0x4u;
  ctx.r6.u64 = 0xFFFFFFFF821611F0u;
  sub_820900C0(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820B0D58(ctx, base);
}

// Vehicle::Construct.
extern "C" REX_FUNC(sub_820C2EF8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C2EF8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820B4160(ctx, base);
  ctx.r3.u64 = object + 3376;
  sub_820AEE68(ctx, base);
  guest_memory::Apply(base, object, kVehicle_Construct0, args);
  ctx.r3.u64 = object;
}


// TrackPosition::Copy: r4 into r3 (two halfwords' runs and the words
// between); returns r3.
extern "C" REX_FUNC(sub_820C1F70) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C1F70(ctx, base);
    return;
  }
  const uint32_t to = ctx.r3.u32, from = ctx.r4.u32;
  guest_memory::CopyHalves(base, to + 0, from + 0, 8);
  guest_memory::CopyWords(base, to + 16, from + 16, 64);
  guest_memory::CopyHalves(base, to + 80, from + 80, 4);
}

// Vehicle::UpdateTrackPositions: the body's position (sub_82249148 of
// +336) into the route position's point and the route position updated;
// then each wheel's position into its route position's point, and that one
// updated.
extern "C" REX_FUNC(sub_820C1B98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C1B98(ctx, base);
    return;
  }
  const uint32_t vehicle = ctx.r3.u32;
  ctx.r3.u64 = LoadU32(base, vehicle + 336);
  sub_82249148(ctx, base);
  guest_memory::CopyWords(base, vehicle + 5232 + 16, ctx.r3.u32, 12);
  UpdateRoutePosition(ctx, base, vehicle + 5232);
  for (uint32_t wheel = 0; wheel < 4; wheel++) {
    const uint32_t position = vehicle + 5328 + 96 * wheel;
    guest_memory::CopyWords(base, position + 16, vehicle + 480 + 368 * wheel, 12);
    UpdateRoutePosition(ctx, base, position);
  }
}

// Vehicle::RemoveFromWorld: +6148 = 0; the collision object (+340) taken
// out of the world (CollisionWorld::RemoveObject, sub_820B92D8) and
// cleared; the other object (+344) deleted (its vtable's slot 0, r4 = 1)
// and cleared.
extern "C" REX_FUNC(sub_820C1F00) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C1F00(ctx, base);
    return;
  }
  const uint32_t vehicle = ctx.r3.u32;
  StoreU32(base, vehicle + 6148, 0);
  const uint32_t object = LoadU32(base, vehicle + 340);
  if (object != 0) {
    ctx.r3.u64 = object;
    sub_820B92D8(ctx, base);
    StoreU32(base, vehicle + 340, 0);
  }
  const uint32_t other = LoadU32(base, vehicle + 344);
  if (other != 0) {
    ctx.r3.u64 = other;
    ctx.r4.u64 = 1;
    REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, other)));
  }
  StoreU32(base, vehicle + 344, 0);
}
