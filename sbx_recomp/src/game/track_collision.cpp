// The track's collision as our own C++ (--native_track_collision, on by
// default): the route's data prepared and freed, and the track's collision
// unloaded.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_track_collision, true, "SBX",
                    "Our own C++ for the track route's prepare and free and the track "
                    "collision's unload instead of the recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kMemRelease = 0x8000;

// The route: its data (the loaded file) and whether it's prepared.
struct TrackRoute {
  u32_be data;
  u32_be unknown_4;
  u32_be prepared;
};
constexpr uint32_t kTrackRoute = 0x826B57F0;
constexpr uint32_t kTrackRoute2 = 0x826B57FC;   // A second object, freed with it.
constexpr uint32_t kTrackBlock = 0x826B5808;    // Holds a block freed on unload.

bool On() { return REXCVAR_GET(native_track_collision); }

}  // namespace

#define SBX_TRACK_COLLISION_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                       \
  if (!On()) {                               \
    __imp__##name(ctx, base);                \
    return;                                  \
  }                                          \
  Guest g(ctx, base);

// TrackRoute::Prepare: r3 the route. Returns it.
extern "C" REX_FUNC(sub_820F84E0) {
  SBX_TRACK_COLLISION_FALLBACK(sub_820F84E0)
  TrackRoute* route = At<TrackRoute>(ctx.r3.u32);
  route->prepared = 1;
  route->data = 0;
}

// TrackRoute::Free: r3 the route; its data given back.
extern "C" REX_FUNC(sub_820F84F8) {
  SBX_TRACK_COLLISION_FALLBACK(sub_820F84F8)
  TrackRoute* route = At<TrackRoute>(ctx.r3.u32);
  route->prepared = 0;
  g.Call(sub_82292D70, route->data, 0u, kMemRelease);  // VirtualFree
  route->data = 0;
}

// TrackCollision::Unload: r3 the collision world.
extern "C" REX_FUNC(sub_820FF100) {
  SBX_TRACK_COLLISION_FALLBACK(sub_820FF100)
  g.Call(sub_820B8550, ctx.r3.u32);         // CollisionWorld::Clear
  g.Call(sub_820F6820, kTrackRoute2);
  g.Call(sub_820F84F8, kTrackRoute);        // TrackRoute::Free
  g.Call(sub_8258C868, *At<u32_be>(kTrackBlock));  // operator delete
}
