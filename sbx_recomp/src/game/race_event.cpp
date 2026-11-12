// The race event as our own C++ (--native_race_event, on by default): its
// constructor.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_event, true, "SBX",
                    "Our own C++ for the race event's constructor instead of the recompiled "
                    "function");

namespace {

struct RaceEvent {
  struct Entry {
    guest::u32_be first;
    guest::u32_be second;
    guest::u32_be third;
  };

  uint8_t unknown_0[40];
  Entry entries[10];  // Each one's first two words cleared, and the last's third.
};
GUEST_SIZE(RaceEvent, 160);

}  // namespace

// RaceEvent::RaceEvent: r3 the event. Returns it.
extern "C" REX_FUNC(sub_821418A8) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_race_event)) {
    __imp__sub_821418A8(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  RaceEvent* event = guest::At<RaceEvent>(ctx.r3.u32);
  for (RaceEvent::Entry& entry : event->entries) {
    entry.first = 0;
    entry.second = 0;
  }
  event->entries[9].third = 0;
}
