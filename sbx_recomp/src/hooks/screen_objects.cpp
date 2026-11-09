// The race's screen object as our own C++ (--native_screen_objects, on by
// default): ScreenObject::SetSpeed (the effect's strength, every tick),
// ScreenObject::Release (its vertex declaration and texture given back
// with the device locked) and its destructor.
//
// Written from SBX's generated code. The object: +0 the vertex declaration,
// +4 the texture, +24 whether it's running, +27 set by SetSpeed, +52 the
// strength (a float), +76096 its UvMap (the ring points' texture
// coordinates). The UvMap and UvColumn trees (the C++ library's std::map
// as the game instantiated it, on its heap), the rings' set-up
// (BuildRings, AddRingPoint) and the draw stay recompiled.

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_screen_objects, true, "SBX",
                    "Our own C++ for the race's screen object (its strength each tick, its "
                    "release and destructor) instead of the recompiled functions; its trees and "
                    "set-up stay recompiled");

namespace {

constexpr uint32_t kOne = 0x82000A20;   // 1.0f
constexpr uint32_t kZero = 0x82000A24;  // 0.0f
constexpr uint32_t kVideo = 0x8265CD68;  // [it] the device's owner (slot 7, then slot 3)
constexpr uint32_t kUvMap = 76096;

// The device as the original finds it: [kVideo]'s slot 7 (+28), then the
// result's slot 3 (+12).
uint32_t Device(PPCContext& ctx, uint8_t* base) {
  ctx.r3.u64 = guest::LoadU32(base, kVideo);
  REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, ctx.r3.u32) + 28));
  REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, ctx.r3.u32) + 12));
  return ctx.r3.u32;
}

// Resource::Release (sub_822A3DB8).
void ReleaseResource(PPCContext& ctx, uint8_t* base, uint32_t resource) {
  ctx.r3.u64 = resource;
  sub_822A3DB8(ctx, base);
}

// ScreenObject::Release: the device locked (sub_82573468; its word in the
// original's 128-byte frame at +80), the vertex declaration and texture
// released and cleared, the device unlocked (sub_825734A8 with the word).
void Release(PPCContext& ctx, uint8_t* base, uint32_t object) {
  guest::StackFrame frame(ctx, 128);
  const uint32_t local = frame.Local(80);
  ctx.r3.u64 = Device(ctx, base);
  sub_82573468(ctx, base);
  guest::StoreU32(base, local, ctx.r3.u32);
  ReleaseResource(ctx, base, guest::LoadU32(base, object + 0));
  guest::StoreU32(base, object + 0, 0);
  ReleaseResource(ctx, base, guest::LoadU32(base, object + 4));
  guest::StoreU32(base, object + 4, 0);
  ctx.r3.u64 = Device(ctx, base);
  ctx.r4.u64 = local;
  sub_825734A8(ctx, base);
}

bool On() { return REXCVAR_GET(native_screen_objects); }

}  // namespace

// ScreenObject::SetSpeed: r3 the object, f1 the strength. When the object
// is running (+24) sub_8212E440 is called first with 0.0; then +27 = 1 and
// +52 = the strength clamped to [0.0, 1.0] (a NaN made 1.0: the original's
// ble isn't taken on an unordered compare).
extern "C" REX_FUNC(sub_8212E928) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8212E928(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  double strength = ctx.f1.f64;
  const double low = guest::LoadFloat(base, kZero);
  if (guest::LoadU8(base, object + 24) != 0) {
    ctx.f1.f64 = low;
    sub_8212E440(ctx, base);
  }
  const double high = guest::LoadFloat(base, kOne);
  guest::StoreU8(base, object + 27, 1);
  if (!(strength <= high)) {
    strength = high;
  } else if (strength < low) {
    strength = low;
  }
  guest::StoreFloat(base, object + 52, float(strength));
}

extern "C" REX_FUNC(sub_8212DDE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8212DDE0(ctx, base);
    return;
  }
  Release(ctx, base, ctx.r3.u32);
}

// ScreenObject::~ScreenObject: Release, then UvMap::~UvMap (sub_82131B28)
// on +76096.
extern "C" REX_FUNC(sub_82131B98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82131B98(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  Release(ctx, base, object);
  ctx.r3.u64 = object + kUvMap;
  sub_82131B28(ctx, base);
}
