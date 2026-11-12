// The track zones: the 70 stretches of the course's route the player's car
// is checked against each tick, for the HUD's road names and signs.

#pragma once

#include "game/fixed_list.h"
#include "game/guest_types.h"

namespace game {

using guest::i16_be;
using guest::i32_be;
using guest::ptr;
using guest::u16_be;
using guest::u32_be;

// A place along the route: the route's section, and the slice along it.
struct RoutePoint {
  i16_be section;
  i16_be slice;
};

// One of the route's sections.
struct RouteSection {
  struct Link {
    i16_be section;  // -1 for none.
    i16_be unknown_2;
  };
  static constexpr int kForward = 0;  // Links 0 to 2 go on along the route.
  static constexpr int kBeside = 6;   // Links 6 to 9 are the sections beside it.

  i16_be slice_count;
  uint8_t unknown_2[26];
  Link links[10];
};
GUEST_OFFSET(RouteSection, links, 28);

struct Route {
  uint8_t unknown_0[12];
  ptr<ptr<RouteSection>> sections;  // By number.
};
constexpr uint32_t kRoute = 0x826B57F0;  // Holds the route's address.

// A stretch of the route a zone covers.
struct RouteRange {
  enum State : uint32_t { kOff = 0, kNotOnIt = 1, kOnIt = 2 };

  u32_be state;
  RoutePoint start;
  RoutePoint end;
  FixedList<64, i32_be> forks;     // The sections to take where the route forks.
  FixedList<64, i32_be> excluded;  // Sections that don't count.
  u32_be include_beside;           // The sections beside the route count too.
};
GUEST_SIZE(RouteRange, 1064);
GUEST_OFFSET(RouteRange, forks, 12);
GUEST_OFFSET(RouteRange, excluded, 536);

struct Zone {
  u32_be vtable;
  FixedList<16, RouteRange> ranges;
  u32_be index;
  u32_be vehicle;  // A vehicle handle (Vehicle::Get).
  u32_be inside;   // The car is on one of its ranges.
};
GUEST_SIZE(Zone, 17116);
GUEST_OFFSET(Zone, index, 17104);

struct TrackZones {
  static constexpr int kZones = 70;

  FixedList<128, ptr<Zone>> list;
  Zone zones[kZones];
};
GUEST_OFFSET(TrackZones, zones, 1036);
constexpr uint32_t kTrackZones = 0x8280F1F4;  // Holds the zones' owner.

}  // namespace game
