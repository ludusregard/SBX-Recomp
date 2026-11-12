// The race world as our own C++ (--native_race_world, on by default): a
// route position moved onto the main road, the race event's starts
// placed, the paused tick's answer and a sorted list's clear.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_world, true, "SBX",
                    "Our own C++ for the race world's small steps (the main road, the "
                    "starts, the paused tick, a sorted list's clear) instead of the "
                    "recompiled functions");

namespace {

constexpr uint32_t kTrackRoute = 0x826B57F0;
constexpr uint32_t kRaceEvent = 0x826B5D50;  // Holds the race event's address.
// What RaceWorld::OnPausedTick answers.
constexpr uint32_t kPausedTickResult = 400;

struct SortedList {
  uint8_t unknown_0[1600];
  guest::u32_be count;
  guest::u32_be unknown_1604[2];
};

bool On() { return REXCVAR_GET(native_race_world); }

}  // namespace

#define SBX_RACE_WORLD_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  guest::Guest g(ctx, base);

// RoutePosition::ToMainRoad: r3 the position, r4 passed on; the route
// moves it (recompiled).
extern "C" REX_FUNC(sub_82141F90) {
  SBX_RACE_WORLD_FALLBACK(sub_82141F90)
  g.Call(sub_820FA608, kTrackRoute, ctx.r3.u32, ctx.r4.u32);
}

// RaceEvent::PlaceStartsCurrent: r3 and r4 passed on to the race event's
// PlaceStarts (recompiled).
extern "C" REX_FUNC(sub_821433C0) {
  SBX_RACE_WORLD_FALLBACK(sub_821433C0)
  g.Call(sub_82143120, *guest::At<guest::u32_be>(kRaceEvent), ctx.r3.u32, ctx.r4.u32);
}

// RaceWorld::OnPausedTick.
extern "C" REX_FUNC(sub_8216E408) {
  SBX_RACE_WORLD_FALLBACK(sub_8216E408)
  ctx.r3.u64 = kPausedTickResult;
}

// SortedList::Clear: r3 the list. Returns it.
extern "C" REX_FUNC(sub_8216E930) {
  SBX_RACE_WORLD_FALLBACK(sub_8216E930)
  SortedList* list = guest::At<SortedList>(ctx.r3.u32);
  list->count = 0;
  list->unknown_1604[0] = 0;
  list->unknown_1604[1] = 0;
}
