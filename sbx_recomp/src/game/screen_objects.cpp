// The race's screen object as our own C++ (--native_screen_objects, on by
// default): the full-screen speed effect drawn over the race.
// ScreenObject::SetSpeed (the effect's strength, every tick),
// ScreenObject::Release and its destructor.
//
// Its UvMap and UvColumn trees (the C++ library's std::map on the game's
// heap), the rings' set-up and the draw stay recompiled.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_screen_objects, true, "SBX",
                    "Our own C++ for the race's screen object (its strength each tick, its "
                    "release and destructor) instead of the recompiled functions; its trees and "
                    "set-up stay recompiled");

namespace {

using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u32_be;

struct ScreenObject {
  u32_be vertex_declaration;  // Graphics resources.
  u32_be texture;
  uint8_t unknown_8[16];
  uint8_t running;
  uint8_t unknown_25[2];
  uint8_t strength_set;
  uint8_t unknown_28[24];
  f32_be strength;  // 0.0 to 1.0.
  uint8_t unknown_56[76040];
  uint8_t uv_map[4];  // The ring points' texture coordinates (a std::map).
};
GUEST_OFFSET(ScreenObject, strength, 52);
GUEST_OFFSET(ScreenObject, uv_map, 76096);

constexpr uint32_t kVideo = 0x8265CD68;  // Holds the device's owner.

bool On() { return REXCVAR_GET(native_screen_objects); }

// The graphics device: the video object's slot 7, then that one's slot 3.
void* Device(Guest& g) {
  void* owner = At<void>(g.CallVirtual(At<void>(*At<u32_be>(kVideo)), 7));
  return At<void>(g.CallVirtual(owner, 3));
}

void ReleaseResource(Guest& g, u32_be& resource) {
  g.Call(sub_822A3DB8, resource);  // Resource::Release
  resource = 0;
}

// Gives the object's graphics resources back, with the device locked.
void Release(Guest& g, ScreenObject* object) {
  guest::StackFrame frame(g.ctx, 128);
  u32_be* lock = At<u32_be>(frame.Local(80));
  *lock = g.Call(sub_82573468, Device(g));  // Device::Lock
  ReleaseResource(g, object->vertex_declaration);
  ReleaseResource(g, object->texture);
  g.Call(sub_825734A8, Device(g), lock);  // Device::Unlock
}

}  // namespace

// ScreenObject::SetSpeed: r3 the object, f1 the strength.
extern "C" REX_FUNC(sub_8212E928) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8212E928(ctx, base);
    return;
  }
  Guest g(ctx, base);
  ScreenObject* object = At<ScreenObject>(ctx.r3.u32);
  double strength = ctx.f1.f64;
  if (object->running != 0) {
    ctx.f1.f64 = 0.0;
    g.Call(sub_8212E440, object);
  }
  object->strength_set = 1;
  // Clamped to 0.0 to 1.0 (a NaN is kept as it is).
  if (strength > 1.0) {
    strength = 1.0;
  } else if (strength < 0.0) {
    strength = 0.0;
  }
  object->strength = float(strength);
}

// ScreenObject::Release: r3 the object.
extern "C" REX_FUNC(sub_8212DDE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8212DDE0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  Release(g, At<ScreenObject>(ctx.r3.u32));
}

// ScreenObject::~ScreenObject: r3 the object.
extern "C" REX_FUNC(sub_82131B98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82131B98(ctx, base);
    return;
  }
  Guest g(ctx, base);
  ScreenObject* object = At<ScreenObject>(ctx.r3.u32);
  Release(g, object);
  g.Call(sub_82131B28, object->uv_map);  // UvMap::~UvMap
}
