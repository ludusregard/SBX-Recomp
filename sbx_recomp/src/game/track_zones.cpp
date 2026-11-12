// The track zones as our own C++ (--native_track_zones, on by default):
// the 70 stretches of the course's route the player's car is checked
// against each tick for the HUD (road names and signs): the zones' owner,
// the zones, their route ranges and the fixed-size lists they're kept in
// (game/track_zones.h, game/fixed_list.h).
//
// The 70 zones' own set-ups (TrackZones::Create) stay recompiled: they run
// once a race, building the zones with the functions here.

#include "game/track_zones.h"
#include "game/vehicle.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_track_zones, true, "SBX",
                    "Our own C++ for the track zones (the HUD's route zones checked against "
                    "the player's car each tick, their route ranges and lists) instead of the "
                    "recompiled functions; their once-a-race set-up stays recompiled");

namespace {

using game::FixedList;
using game::RoutePoint;
using game::RouteRange;
using game::RouteSection;
using game::TrackZones;
using game::Zone;
using guest::AddressOf;
using guest::At;
using guest::Guest;
using guest::i32_be;
using guest::u32_be;

// The slices counted along a range's path by the last OnPath.
constexpr uint32_t kSlicesCounted = 0x825F31FC;
// The racers, by number (from +717 words in), and the flag of one in the
// race.
constexpr uint32_t kRacers = 0x826C7D9C;
constexpr uint32_t kRacersFirst = 717;
constexpr uint32_t kRacerActive = 0x40;
// The zones' vtables: one class each, 12 bytes apart.
constexpr uint32_t kZoneVtables = 0x820117C0;

bool On() { return REXCVAR_GET(native_track_zones); }

RouteSection* Section(int32_t number) {
  game::Route* route = At<game::Route>(*At<u32_be>(game::kRoute));
  return *guest::Element<guest::ptr<RouteSection>>(route->sections.address,
                                                   uint32_t(number) & 0xFFFF);
}

// RouteRange::ToNeighbor: `point` moved onto `section`. Where the two
// sections lead on into the same one, the slice is moved by the difference
// of their slice counts. Returns false when that leaves it below 0 (the
// point is still written).
bool ToNeighbor(RoutePoint* out, int16_t section, RoutePoint point) {
  const RouteSection* from = Section(point.section);
  const RouteSection* to = Section(section);
  bool shared = false;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      const int16_t a = from->links[i].section, b = to->links[j].section;
      if (a != -1 && b != -1 && a == b) {
        shared = true;
      }
    }
  }
  int32_t slice = point.slice;
  bool ok = true;
  if (shared && from->slice_count != to->slice_count) {
    slice = slice - from->slice_count + to->slice_count;
    ok = slice >= 0;
  }
  out->section = section;
  out->slice = int16_t(slice);
  return ok;
}

// RouteRange::WithinEnds: whether the point isn't before the start on the
// start's section or past the end on the end's. With the sections beside
// the route counted, the same against the ends moved onto those.
bool WithinEnds(RouteRange* range, RoutePoint point) {
  const RoutePoint start = range->start, end = range->end;
  const int16_t on = point.section;
  const int16_t slice = point.slice;
  bool after_start = !(start.section == on && start.slice > slice);
  bool before_end = !(end.section == on && end.slice < slice);
  if (range->include_beside != 0) {
    const RouteSection* start_section = Section(start.section);
    const RouteSection* end_section = Section(end.section);
    for (int k = 0; k < 4; k++) {
      RoutePoint a{start_section->links[RouteSection::kBeside + k].section, -1};
      RoutePoint b{end_section->links[RouteSection::kBeside + k].section, -1};
      if (a.section != -1 && ToNeighbor(&a, a.section, start)) {
        if (a.section == on && a.slice > slice) {
          after_start = false;
        }
        // As the game has it: b isn't moved yet here, and any slice from 0
        // on counts as past it.
        if (b.section == on && slice > -1) {
          before_end = false;
        }
      }
      if (b.section != -1 && ToNeighbor(&b, b.section, end)) {
        if (a.section == on && a.slice > slice) {
          after_start = false;
        }
        if (b.section == on && b.slice < slice) {
          before_end = false;
        }
      }
    }
  }
  return after_start && before_end;
}

// RouteRange::OnPath: whether the point is on the path from the start's
// section to the end's, taking the range's forks. Counts the slices along
// the way (kSlicesCounted).
uint32_t OnPath(RouteRange* range, RoutePoint point) {
  if (range->state == RouteRange::kOff) {
    return RouteRange::kOff;
  }
  u32_be* counted = At<u32_be>(kSlicesCounted);
  *counted = uint32_t(int32_t(point.slice));
  const int16_t on = point.section;
  const int16_t end = range->end.section;
  int16_t at = range->start.section;
  if (on == at) {
    return WithinEnds(range, point) ? RouteRange::kOnIt : RouteRange::kNotOnIt;
  }
  if (end == at) {
    return RouteRange::kNotOnIt;
  }
  uint32_t result = RouteRange::kNotOnIt;
  for (;;) {
    const RouteSection* section = Section(at);
    const int16_t was = at;
    if (on == at && WithinEnds(range, point)) {
      result = RouteRange::kOnIt;
    }
    *counted = *counted + uint32_t(int32_t(section->slice_count));

    // A link onto the point's section ends the walk there.
    int links = 0;
    for (int i = 0; i < 3; i++) {
      const int16_t next = section->links[RouteSection::kForward + i].section;
      if (on == next) {
        if (WithinEnds(range, point)) {
          result = RouteRange::kOnIt;
        }
        break;
      }
      if (next != -1) {
        links++;
      }
    }
    if (result == RouteRange::kOnIt) {
      return result;
    }

    if (links == 1) {
      int i = 0;
      while (i < 3 && section->links[i].section == -1) {
        i++;
      }
      if (i == 3) {
        return result;
      }
      at = section->links[i].section;
    } else if (links > 1) {
      // A fork: the last link that's one of the range's forks is taken;
      // the search stops once a link before it was the section reached.
      int16_t previous = -1;
      for (int i = 0; i < 3; i++) {
        if (at == previous) {
          break;
        }
        previous = section->links[i].section;
        if (range->forks.Contains(previous)) {
          at = previous;
        }
      }
    } else {
      return result;
    }
    if (was == at || end == at) {
      return result;
    }
  }
}

// RouteRange::Test: the range's state for a point: not on it on an
// excluded section; else on its path, or (with the sections beside the
// route counted) on it once moved onto one of those.
uint32_t Test(RouteRange* range, RoutePoint point) {
  if (range->state == RouteRange::kOff) {
    return RouteRange::kOff;
  }
  if (range->excluded.Contains(point.section)) {
    return RouteRange::kNotOnIt;
  }
  const uint32_t result = OnPath(range, point);
  if (range->include_beside == 0 || result == RouteRange::kOnIt) {
    return result;
  }
  const RouteSection* section = Section(point.section);
  for (int k = 0; k < 4; k++) {
    const int16_t beside = section->links[RouteSection::kBeside + k].section;
    RoutePoint moved;
    if (beside != -1 && ToNeighbor(&moved, beside, point) &&
        OnPath(range, moved) == RouteRange::kOnIt) {
      return RouteRange::kOnIt;
    }
  }
  return result;
}

// RouteRange::Update: with the car on the route, the range's state tested
// again. Returns whether the car is on it.
bool UpdateRange(RouteRange* range, RoutePoint point) {
  if (uint16_t(point.section) < 0x8000) {
    range->state = Test(range, point);
  }
  return range->state == RouteRange::kOnIt;
}

// Zone::Update: whether the zone's vehicle is on one of its ranges.
void UpdateZone(Guest& g, Zone* zone) {
  const game::RoutePosition* position =
      At<game::RoutePosition>(g.Call(sub_820BFFA0, zone->vehicle));  // Vehicle::Identity
  const RoutePoint point{int16_t(position->section), int16_t(position->slice)};
  zone->inside = 0;
  for (uint32_t i = 0; i < uint32_t(int32_t(zone->ranges.count)); i++) {
    if (UpdateRange(zone->ranges.ItemAtPosition(i), point)) {
      zone->inside = 1;
      break;
    }
  }
}

Zone* ZoneAt(TrackZones* zones, uint32_t position) {
  return *zones->list.ItemAtPosition(position);
}

}  // namespace

#define SBX_TRACK_ZONES_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                   \
  if (!On()) {                           \
    __imp__##name(ctx, base);            \
    return;                              \
  }                                      \
  Guest g(ctx, base);

// --- The lists: r3 = the list ---

// List16::Link, List64::Link, List128::Link: r4 = the position.
extern "C" REX_FUNC(sub_82151288) {
  SBX_TRACK_ZONES_FALLBACK(sub_82151288)
  At<FixedList<16, RouteRange>>(ctx.r3.u32)->Link(int16_t(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE150) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE150)
  At<FixedList<64, i32_be>>(ctx.r3.u32)->Link(int16_t(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_82288988) {
  SBX_TRACK_ZONES_FALLBACK(sub_82288988)
  At<FixedList<128, u32_be>>(ctx.r3.u32)->Link(int16_t(ctx.r4.u32));
}

// List64::Insert, List128::Insert: r4 = the word's address, r5 = the
// position.
extern "C" REX_FUNC(sub_820FE1B8) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE1B8)
  At<FixedList<64, i32_be>>(ctx.r3.u32)->Insert(*At<i32_be>(ctx.r4.u32), ctx.r5.u32);
}

extern "C" REX_FUNC(sub_82289000) {
  SBX_TRACK_ZONES_FALLBACK(sub_82289000)
  At<FixedList<128, u32_be>>(ctx.r3.u32)->Insert(*At<u32_be>(ctx.r4.u32), ctx.r5.u32);
}

// --- The route ranges: r3 = the range ---

// RouteRange::Init: r4 = the start and the end.
extern "C" REX_FUNC(sub_820FDE20) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FDE20)
  RouteRange* range = At<RouteRange>(ctx.r3.u32);
  const RoutePoint* ends = At<RoutePoint>(ctx.r4.u32);
  range->start = ends[0];
  range->end = ends[1];
  range->forks.Reset();
  range->excluded.Reset();
  range->state = RouteRange::kNotOnIt;
  range->include_beside = 1;
}

// RouteRange::AddBranch: r4 = a section to take where the route forks.
extern "C" REX_FUNC(sub_820FE750) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE750)
  RouteRange* range = At<RouteRange>(ctx.r3.u32);
  range->forks.Insert(int32_t(ctx.r4.u32), uint32_t(int32_t(range->forks.count)));
}

// RouteRange::ToNeighbor: r3 = the point out, r4 = the section, r5 = the
// point's address.
extern "C" REX_FUNC(sub_820FDD60) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FDD60)
  RoutePoint* out = At<RoutePoint>(ctx.r3.u32);
  const RoutePoint point = *At<RoutePoint>(ctx.r5.u32);
  ctx.r3.u64 = ToNeighbor(out, int16_t(ctx.r4.u32), point);
}

// RouteRange::WithinEnds, OnPath, Test, Update: r4 = the point's address.
extern "C" REX_FUNC(sub_820FDEE8) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FDEE8)
  ctx.r3.u64 = WithinEnds(At<RouteRange>(ctx.r3.u32), *At<RoutePoint>(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE220) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE220)
  ctx.r3.u64 = OnPath(At<RouteRange>(ctx.r3.u32), *At<RoutePoint>(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE580) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE580)
  ctx.r3.u64 = Test(At<RouteRange>(ctx.r3.u32), *At<RoutePoint>(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE700) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE700)
  ctx.r3.u64 = UpdateRange(At<RouteRange>(ctx.r3.u32), *At<RoutePoint>(ctx.r4.u32));
}

// --- The zones: r3 = the zone ---

// Zone::Reset: r4 = the index and the vehicle.
extern "C" REX_FUNC(sub_82288928) {
  SBX_TRACK_ZONES_FALLBACK(sub_82288928)
  Zone* zone = At<Zone>(ctx.r3.u32);
  const u32_be* values = At<u32_be>(ctx.r4.u32);
  zone->ranges.Reset();
  zone->index = values[0];
  zone->inside = 0;
  zone->vehicle = values[1];
}

// Zone::IsInside.
extern "C" REX_FUNC(sub_822888E0) {
  SBX_TRACK_ZONES_FALLBACK(sub_822888E0)
  ctx.r3.u64 = At<Zone>(ctx.r3.u32)->inside;
}

// ZoneRanges::Insert: r3 = the zone's range list, r4 = a range (copied in),
// r5 = the position. Returns the copy.
extern "C" REX_FUNC(sub_82288F90) {
  SBX_TRACK_ZONES_FALLBACK(sub_82288F90)
  auto* list = At<FixedList<16, RouteRange>>(ctx.r3.u32);
  const RouteRange* range = At<RouteRange>(ctx.r4.u32);
  const uint32_t position = ctx.r5.u32;
  list->Link(int16_t(position));
  RouteRange* copy = list->ItemAtPosition(position);
  *copy = *range;
  ctx.r3.u64 = AddressOf(copy);
}

// Zone::Update: r3 = the zone.
extern "C" REX_FUNC(sub_82289068) {
  SBX_TRACK_ZONES_FALLBACK(sub_82289068)
  UpdateZone(g, At<Zone>(ctx.r3.u32));
}

// Racers::Vehicle: r3 = the racer's number; its vehicle, or 0 when there's
// no such racer in the race.
extern "C" REX_FUNC(sub_820C94B8) {
  SBX_TRACK_ZONES_FALLBACK(sub_820C94B8)
  struct Racer {
    u32_be flags;
    uint8_t unknown_4[16];
    u32_be vehicle;  // A handle (Vehicle::Get).
  };
  const uint32_t racers = *At<u32_be>(kRacers);
  Racer* racer = At<Racer>(*guest::Element<u32_be>(racers, ctx.r3.u32 + kRacersFirst));
  if (racer == nullptr || (racer->flags & kRacerActive) == 0) {
    ctx.r3.u64 = 0;
    return;
  }
  g.Call(sub_820C0288, racer->vehicle);  // Vehicle::Get
}

// --- The zones' owner ---

// TrackZones::TrackZones: r3 the owner; each zone's class set. Returns r3.
extern "C" REX_FUNC(sub_822889F0) {
  SBX_TRACK_ZONES_FALLBACK(sub_822889F0)
  TrackZones* zones = At<TrackZones>(ctx.r3.u32);
  for (int i = 0; i < TrackZones::kZones; i++) {
    zones->zones[i].vtable = kZoneVtables + 12 * i;
  }
}

// TrackZones::Destroy: the owner deleted, if there is one.
extern "C" REX_FUNC(sub_822888E8) {
  SBX_TRACK_ZONES_FALLBACK(sub_822888E8)
  u32_be* owner = At<u32_be>(game::kTrackZones);
  if (*owner != 0) {
    g.Call(sub_8258C868, *owner);  // operator delete
    *owner = 0;
  }
}

// TrackZones::Update: r3 = the owner; each zone's Update (its virtual
// slot 1) in list order.
extern "C" REX_FUNC(sub_82289118) {
  SBX_TRACK_ZONES_FALLBACK(sub_82289118)
  TrackZones* zones = At<TrackZones>(ctx.r3.u32);
  for (uint32_t i = 0; i < uint32_t(int32_t(zones->list.count)); i++) {
    g.CallVirtual(ZoneAt(zones, i), 1);
  }
}

// TrackZones::IsInside: r3 = the owner, r4 = the zone's place in the list;
// that zone's IsInside (its virtual slot 2).
extern "C" REX_FUNC(sub_822891B8) {
  SBX_TRACK_ZONES_FALLBACK(sub_822891B8)
  TrackZones* zones = At<TrackZones>(ctx.r3.u32);
  g.CallVirtual(ZoneAt(zones, ctx.r4.u32), 2);
}
