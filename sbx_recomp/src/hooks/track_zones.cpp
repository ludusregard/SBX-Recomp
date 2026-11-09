// The track zones as our own C++ (--native_track_zones, on by default):
// the 70 stretches of the course's route the player's car is checked
// against each tick for the HUD (road names and signs): the zones' owner,
// the zones, their route ranges and the fixed-size lists they're kept in.
//
// Written from SBX's generated code. The racers' table is at [0x826C7D9C]
// + 4 x (racer + 717). The owner's set-up (TrackZones::TrackZones,
// TrackZones::Create) and the 70 zones' set-ups stay recompiled: they run
// once a race, building the zones with the functions here.
//
// The owner, the zones and the ranges are guest data shared with the
// recompiled code, so these work on guest memory with the originals' loads
// and stores in their order. A track position is two halfwords: the
// route's section and the slice along it.

#include "guest.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_track_zones, true, "SBX",
                    "Our own C++ for the track zones (the HUD's route zones checked against "
                    "the player's car each tick, their route ranges and lists) instead of the "
                    "recompiled functions; their once-a-race set-up stays recompiled");

namespace {

constexpr uint32_t kRoute = 0x826B57F0;       // [it] the route: +12 the sections table
constexpr uint32_t kSliceCount = 0x825F31FC;  // the slices counted along a range's path
constexpr uint32_t kRacers = 0x826C7D9C;      // [it] + 4 x (racer + 717): the racers
constexpr uint32_t kOwner = 0x8280F1F4;       // [it] the zones' owner

// A zone: +4 its 16-slot list of ranges (the ranges at +80, 1,064 bytes
// apart), +17104 its index, +17108 its vehicle, +17112 whether the car is
// inside one of its ranges.
constexpr uint32_t kZoneRanges = 4;
constexpr uint32_t kZoneIndex = 17104;
constexpr uint32_t kZoneVehicle = 17108;
constexpr uint32_t kZoneInside = 17112;
constexpr uint32_t kRangeBytes = 1064;

struct Position {
  uint16_t section;
  uint16_t slice;
};

Position LoadPosition(uint8_t* base, uint32_t address) {
  return {guest::LoadU16(base, address), guest::LoadU16(base, address + 2)};
}

int32_t LoadS16(uint8_t* base, uint32_t address) { return int16_t(guest::LoadU16(base, address)); }

void Store16(uint8_t* base, uint32_t address, uint32_t value) {
  guest::StoreU16(base, address, uint16_t(value));
}

// --- The fixed-size lists --------------------------------------------------
//
// One layout for N slots: N nodes of two halfwords (the next node, then the
// data slot), the free list's head (+4N), the list's head (+4N + 4), the
// count (+4N + 8), then the data. Indices are signed halfwords, times 4
// from the list (32 bits, as the originals make them).

uint32_t Node(uint32_t list, int32_t index) { return uint32_t(index) * 4 + list; }

// The node `steps` after the head's node (the steps 32 bits, as the
// originals count them).
uint32_t NodeAt(uint8_t* base, uint32_t list, uint32_t head, uint32_t steps) {
  uint32_t node = Node(list, LoadS16(base, head));
  for (uint32_t n = steps; n != 0; n--) {
    node = Node(list, LoadS16(base, node));
  }
  return node;
}

int32_t Slot(uint8_t* base, uint32_t node) { return LoadS16(base, node + 2); }

// Link: the free list's head taken and linked in after `position` nodes
// (the list's head for 0), the count up by 1. No check for a full list; a
// negative position walks as the original does.
template <uint32_t N>
void Link(uint8_t* base, uint32_t list, int16_t position) {
  constexpr uint32_t kFree = 4 * N, kHead = 4 * N + 4, kCount = 4 * N + 8;
  uint32_t after;
  if (position == 0) {
    after = list + kHead;
  } else {
    int32_t index = LoadS16(base, list + kHead);
    uint32_t n = uint32_t(int32_t(position));
    for (;;) {
      after = Node(list, index);
      if (--n == 0) {
        break;
      }
      index = LoadS16(base, after);
    }
  }
  const uint32_t free_node = Node(list, LoadS16(base, list + kFree));
  const uint16_t free_index = guest::LoadU16(base, list + kFree);
  Store16(base, list + kFree, guest::LoadU16(base, free_node));
  Store16(base, free_node, free_index);
  Store16(base, free_node, guest::LoadU16(base, after));
  Store16(base, after, free_index);
  Store16(base, list + kCount, guest::LoadU16(base, list + kCount) + 1);
}

// Insert (lists of words, the data at +4N + 12): Link at the position's
// low halfword, then the word stored in the new node's slot.
template <uint32_t N>
void Insert(uint8_t* base, uint32_t list, uint32_t word, uint32_t position) {
  Link<N>(base, list, int16_t(position));
  const uint32_t node = NodeAt(base, list, list + 4 * N + 4, position);
  guest::StoreU32(base, uint32_t(Slot(base, node) + int32_t(N + 3)) * 4 + list, word);
}

// A list emptied: node i's slot i and next i + 1, then the free head and
// count 0, the last node's next, the head and the two halfwords after it
// -1, in the originals' order (`order` 0 for the zone's and the range's
// first list, 1 for the range's second).
template <uint32_t N>
void Reset(uint8_t* base, uint32_t list, int order) {
  for (uint32_t i = 0; i < N; i++) {
    Store16(base, list + 4 * i + 2, i);
    Store16(base, list + 4 * i, i + 1);
  }
  const uint32_t free_head = list + 4 * N, head = free_head + 4, count = free_head + 8;
  if (order == 0) {
    Store16(base, free_head, 0);
    Store16(base, count, 0);
    Store16(base, free_head - 4, 0xFFFF);
    Store16(base, head, 0xFFFF);
    Store16(base, head + 2, 0xFFFF);
    Store16(base, free_head + 2, 0xFFFF);
  } else {
    Store16(base, free_head - 4, 0xFFFF);
    Store16(base, head, 0xFFFF);
    Store16(base, head + 2, 0xFFFF);
    Store16(base, free_head, 0);
    Store16(base, free_head + 2, 0xFFFF);
    Store16(base, count, 0);
  }
}

// Whether `value` is in a range's 64-slot list of words at `list`, its
// nodes walked from the head for each slot in turn as the originals do.
bool InList64(uint8_t* base, uint32_t list, int32_t value) {
  const uint32_t count = uint32_t(LoadS16(base, list + 264));
  for (uint32_t i = 0; i < count; i++) {
    const uint32_t node = NodeAt(base, list, list + 260, i);
    if (value == int32_t(guest::LoadU32(base, uint32_t(Slot(base, node) + 67) * 4 + list))) {
      return true;
    }
  }
  return false;
}

// --- The route ranges ------------------------------------------------------
//
// A range (1,064 bytes): +0 its state (0 off, 1 the car not on it, 2 on
// it), +4 the start, +8 the end, +12 a 64-slot list of the sections to take
// at forks, +536 one of sections left out, +1060 whether the sections
// beside the route (links 6 to 9) count too.

// The route's section record: +0 its slice count, +28 its links (+28 to
// +36 forward, +52 to +64 beside).
uint32_t Section(uint8_t* base, uint32_t section) {
  return guest::LoadU32(
      base, guest::LoadU32(base, guest::LoadU32(base, kRoute) + 12) + 4 * (section & 0xFFFF));
}

// RouteRange::ToNeighbor (sub_820FDD60): `position` moved onto `section`.
// The slice moves by the two sections' slice counts' difference when one of
// the position's section's forward links 0 and 1 is one of `section`'s; 0
// when that leaves it below 0 (out still written), else 1.
uint32_t ToNeighbor(uint8_t* base, Position* out, uint16_t section, Position position) {
  const uint32_t from = Section(base, position.section);
  const uint32_t to = Section(base, section);
  const int32_t from_count = LoadS16(base, from);
  const int32_t to_count = LoadS16(base, to);
  int32_t slice = int16_t(position.slice);
  bool shared = false;
  for (uint32_t i = 0; i < 2; i++) {
    const int32_t a = LoadS16(base, from + 28 + 4 * i);
    for (uint32_t j = 0; j < 2; j++) {
      const int32_t b = LoadS16(base, to + 28 + 4 * j);
      if (a != -1 && b != -1 && a == b) {
        shared = true;
      }
    }
  }
  uint32_t result = 1;
  if (shared && from_count != to_count) {
    slice = slice - from_count + to_count;
    if (slice < 0) {
      result = 0;
    }
  }
  out->section = section;
  out->slice = uint16_t(slice);
  return result;
}

// RouteRange::WithinEnds (sub_820FDEE8): 0 when the position is on the
// start's section before its slice, or on the end's after its slice; when
// +1060 is set, the same against the ends moved onto the sections beside
// them. Else 1.
uint32_t WithinEnds(uint8_t* base, uint32_t range, Position position) {
  const Position start = LoadPosition(base, range + 4);
  const Position end = LoadPosition(base, range + 8);
  const int32_t on = int16_t(position.section);
  const int32_t slice = int16_t(position.slice);
  bool after_start = true, before_end = true;
  if (int16_t(start.section) == on && int16_t(start.slice) > slice) {
    after_start = false;
  }
  if (int16_t(end.section) == on && int16_t(end.slice) < slice) {
    before_end = false;
  }
  if (guest::LoadU32(base, range + 1060) != 0) {
    const uint32_t start_record = Section(base, start.section);
    const uint32_t end_record = Section(base, end.section);
    for (uint32_t k = 0; k < 4; k++) {
      // The slices -1 until moved, as the original's frame words.
      Position a = {guest::LoadU16(base, start_record + 52 + 4 * k), 0xFFFF};
      Position b = {guest::LoadU16(base, end_record + 52 + 4 * k), 0xFFFF};
      if (int16_t(a.section) != -1 && ToNeighbor(base, &a, a.section, start) != 0) {
        if (int16_t(a.section) == on && int16_t(a.slice) > slice) {
          after_start = false;
        }
        // As the original has it: b's section (not yet moved) and any
        // slice above -1.
        if (int16_t(b.section) == on && slice > -1) {
          before_end = false;
        }
      }
      if (int16_t(b.section) != -1 && ToNeighbor(base, &b, b.section, end) != 0) {
        if (int16_t(a.section) == on && int16_t(a.slice) > slice) {
          after_start = false;
        }
        if (int16_t(b.section) == on && int16_t(b.slice) < slice) {
          before_end = false;
        }
      }
    }
  }
  return after_start && before_end ? 1 : 0;
}

// RouteRange::OnPath (sub_820FE220): 0 (state 0), 1 (not on the path) or 2
// (on it) for the path from the start's section to the end's, through the
// sections to take at forks. kSliceCount = the slices counted along it.
uint32_t OnPath(uint8_t* base, uint32_t range, Position position) {
  if (guest::LoadU32(base, range) == 0) {
    return 0;
  }
  uint32_t counted = uint32_t(int32_t(int16_t(position.slice)));
  uint32_t result = 1;
  guest::StoreU32(base, kSliceCount, counted);
  int32_t at = LoadS16(base, range + 4);
  const int32_t on = int16_t(position.section);
  const int32_t end = LoadS16(base, range + 8);
  if (on == at) {
    return WithinEnds(base, range, position) != 0 ? 2 : 1;
  }
  if (end == at) {
    return 1;
  }
  const uint32_t route = guest::LoadU32(base, kRoute);
  for (;;) {
    if (result == 2) {
      return result;
    }
    const uint32_t record =
        guest::LoadU32(base, guest::LoadU32(base, route + 12) + 4 * (uint32_t(at) & 0xFFFF));
    int32_t links = 0;
    const int32_t was = at;
    if (on == at && WithinEnds(base, range, position) != 0) {
      result = 2;
    }
    counted += uint32_t(LoadS16(base, record));
    guest::StoreU32(base, kSliceCount, counted);
    // The forward links 0 to 2: one onto the position's section ends the
    // count there.
    const uint32_t link = record + 28;
    for (uint32_t i = 0; i < 3; i++) {
      const int32_t next = LoadS16(base, link + 4 * i);
      if (on == next) {
        if (WithinEnds(base, range, position) != 0) {
          result = 2;
        }
        break;
      }
      if (next != -1) {
        links++;
      }
    }
    if (result == 2) {
      return result;
    }
    if (links == 1) {
      uint32_t i = 0;
      while (i < 3 && guest::LoadU16(base, link + 4 * i) == 0xFFFF) {
        i++;
      }
      if (i == 3) {
        return result;
      }
      at = LoadS16(base, link + 4 * i);
    } else if (links > 1) {
      // A fork: the links' sections in the list of sections to take; the
      // last one found wins, and the search stops once a link before it
      // matched the section reached (as the original's test has it).
      int32_t previous = -1;
      for (uint32_t i = 0; i < 3; i++) {
        if (at == previous) {
          break;
        }
        previous = LoadS16(base, link + 4 * i);
        if (InList64(base, range + 12, previous)) {
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

// RouteRange::Test (sub_820FE580): 0 (state 0), 1 (the section left out, or
// not on the path) or 2 (on it, or, with +1060, on it once moved onto a
// section beside, links 6 to 9).
uint32_t Test(uint8_t* base, uint32_t range, Position position) {
  if (guest::LoadU32(base, range) == 0) {
    return 0;
  }
  if (InList64(base, range + 536, int16_t(position.section))) {
    return 1;
  }
  const uint32_t result = OnPath(base, range, position);
  if (guest::LoadU32(base, range + 1060) == 0 || result == 2) {
    return result;
  }
  const uint32_t record = Section(base, position.section);
  for (uint32_t k = 6; k <= 9; k++) {
    const int32_t beside = LoadS16(base, record + 28 + 4 * k);
    if (beside == -1) {
      continue;
    }
    Position moved;
    if (ToNeighbor(base, &moved, uint16_t(beside), position) == 0) {
      continue;
    }
    if (OnPath(base, range, moved) == 2) {
      return 2;
    }
  }
  return result;
}

// RouteRange::Update (sub_820FE700): with a section below 0x8000 (a car on
// the route), the state = Test; 1 when it's 2, else 0.
uint32_t RangeUpdate(uint8_t* base, uint32_t range, Position position) {
  if (position.section < 0x8000) {
    guest::StoreU32(base, range, Test(base, range, position));
  }
  return guest::LoadU32(base, range) == 2 ? 1 : 0;
}

}  // namespace

#define SBX_TRACK_ZONES_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                     \
  if (!REXCVAR_GET(native_track_zones)) {  \
    __imp__##name(ctx, base);              \
    return;                                \
  }

// --- The lists: r3 = the list ---

// List16::Link, List64::Link, List128::Link: r4 = the position (its low
// halfword). r3 left.
extern "C" REX_FUNC(sub_82151288) {
  SBX_TRACK_ZONES_FALLBACK(sub_82151288)
  Link<16>(base, ctx.r3.u32, int16_t(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE150) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE150)
  Link<64>(base, ctx.r3.u32, int16_t(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_82288988) {
  SBX_TRACK_ZONES_FALLBACK(sub_82288988)
  Link<128>(base, ctx.r3.u32, int16_t(ctx.r4.u32));
}

// List64::Insert, List128::Insert: r4 = the word's address, r5 = the
// position. r3 left as Link leaves it (the list).
extern "C" REX_FUNC(sub_820FE1B8) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE1B8)
  Insert<64>(base, ctx.r3.u32, guest::LoadU32(base, ctx.r4.u32), ctx.r5.u32);
}

extern "C" REX_FUNC(sub_82289000) {
  SBX_TRACK_ZONES_FALLBACK(sub_82289000)
  Insert<128>(base, ctx.r3.u32, guest::LoadU32(base, ctx.r4.u32), ctx.r5.u32);
}

// --- The route ranges: r3 = the range ---

// RouteRange::Init: r4 = the start and the end (two positions). Both lists
// emptied, the state and +1060 = 1.
extern "C" REX_FUNC(sub_820FDE20) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FDE20)
  const uint32_t range = ctx.r3.u32;
  const uint32_t ends = ctx.r4.u32;
  Store16(base, range + 4, guest::LoadU16(base, ends + 0));
  Store16(base, range + 6, guest::LoadU16(base, ends + 2));
  Store16(base, range + 8, guest::LoadU16(base, ends + 4));
  Store16(base, range + 10, guest::LoadU16(base, ends + 6));
  Reset<64>(base, range + 12, 0);
  Reset<64>(base, range + 536, 1);
  guest::StoreU32(base, range + 0, 1);
  guest::StoreU32(base, range + 1060, 1);
}

// RouteRange::AddBranch: r4 = a section to take, added at the end of the
// list. As the original, the word is stored to the caller's frame (+28 from
// its r1) and inserted from there.
extern "C" REX_FUNC(sub_820FE750) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE750)
  const uint32_t list = ctx.r3.u32 + 12;
  guest::StoreU32(base, ctx.r1.u32 + 28, ctx.r4.u32);
  Insert<64>(base, list, ctx.r4.u32, uint32_t(LoadS16(base, list + 264)));
  ctx.r3.u64 = list;
}

// RouteRange::ToNeighbor: r3 = the position out, r4 = the section, r5 =
// the position's address.
extern "C" REX_FUNC(sub_820FDD60) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FDD60)
  const uint32_t out = ctx.r3.u32;
  Position moved;
  const uint32_t result =
      ToNeighbor(base, &moved, uint16_t(ctx.r4.u32), LoadPosition(base, ctx.r5.u32));
  Store16(base, out, moved.section);
  Store16(base, out + 2, moved.slice);
  ctx.r3.u64 = result;
}

// RouteRange::WithinEnds, OnPath, Test, Update: r4 = the position's
// address.
extern "C" REX_FUNC(sub_820FDEE8) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FDEE8)
  ctx.r3.u64 = WithinEnds(base, ctx.r3.u32, LoadPosition(base, ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE220) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE220)
  ctx.r3.u64 = OnPath(base, ctx.r3.u32, LoadPosition(base, ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE580) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE580)
  ctx.r3.u64 = Test(base, ctx.r3.u32, LoadPosition(base, ctx.r4.u32));
}

extern "C" REX_FUNC(sub_820FE700) {
  SBX_TRACK_ZONES_FALLBACK(sub_820FE700)
  ctx.r3.u64 = RangeUpdate(base, ctx.r3.u32, LoadPosition(base, ctx.r4.u32));
}

// --- The zones: r3 = the zone ---

// Zone::Reset: r4 = the index and the vehicle. The range list emptied, then
// the index, the flag (0) and the vehicle.
extern "C" REX_FUNC(sub_82288928) {
  SBX_TRACK_ZONES_FALLBACK(sub_82288928)
  const uint32_t zone = ctx.r3.u32;
  Reset<16>(base, zone + kZoneRanges, 0);
  guest::StoreU32(base, zone + kZoneIndex, guest::LoadU32(base, ctx.r4.u32));
  guest::StoreU32(base, zone + kZoneInside, 0);
  guest::StoreU32(base, zone + kZoneVehicle, guest::LoadU32(base, ctx.r4.u32 + 4));
}

// Zone::IsInside.
extern "C" REX_FUNC(sub_822888E0) {
  SBX_TRACK_ZONES_FALLBACK(sub_822888E0)
  ctx.r3.u64 = guest::LoadU32(base, ctx.r3.u32 + kZoneInside);
}

// ZoneRanges::Insert: r3 = the zone's range list, r4 = a range, r5 = the
// position. Linked in, then the range copied into the new node's slot
// (memcpy, recompiled). r3 = the copy's destination.
extern "C" REX_FUNC(sub_82288F90) {
  SBX_TRACK_ZONES_FALLBACK(sub_82288F90)
  const uint32_t list = ctx.r3.u32;
  const uint32_t range = ctx.r4.u32;
  const uint32_t position = ctx.r5.u32;
  Link<16>(base, list, int16_t(position));
  const uint32_t node = NodeAt(base, list, list + 68, position);
  const uint32_t to = uint32_t(Slot(base, node)) * kRangeBytes + list + 76;
  guest_call::InFrame(ctx, base, 112, 0x82288FF8, [&](uint32_t) {
    ctx.r3.u64 = to;
    ctx.r4.u64 = range;
    ctx.r5.u64 = kRangeBytes;
    sub_8258CB90(ctx, base);  // memcpy
  });
}

// Zone::Update: the vehicle's track position (Vehicle::Identity's +4, as
// the original copies its 96 bytes to its frame and reads them there), the
// flag cleared, then each range in list order updated until one answers 1
// (the flag set). r3 = the last range's answer, or the copy's (its frame +
// 80) with no ranges.
extern "C" REX_FUNC(sub_82289068) {
  SBX_TRACK_ZONES_FALLBACK(sub_82289068)
  const uint32_t zone = ctx.r3.u32;
  guest_call::InFrame(ctx, base, 208, 0x82289080, [&](uint32_t frame) {
    ctx.r3.u64 = guest::LoadU32(base, zone + kZoneVehicle);
    sub_820BFFA0(ctx, base);  // Vehicle::Identity
    ctx.r4.u64 = ctx.r3.u32;
    ctx.r3.u64 = frame + 80;
    ctx.r5.u64 = 96;
    ctx.lr = 0x82289090;
    sub_8258CB90(ctx, base);  // memcpy
    const Position position = LoadPosition(base, frame + 84);
    guest::StoreU32(base, zone + kZoneInside, 0);
    const uint32_t list = zone + kZoneRanges;
    for (uint32_t i = 0; i < uint32_t(LoadS16(base, zone + 76)); i++) {
      const uint32_t node = NodeAt(base, list, list + 68, i);
      const uint32_t range = uint32_t(Slot(base, node)) * kRangeBytes + list + 76;
      ctx.r3.u64 = RangeUpdate(base, range, position);
      if (ctx.r3.u32 != 0) {
        guest::StoreU32(base, zone + kZoneInside, 1);
        break;
      }
    }
  });
}

// Racers::Vehicle: r3 = the racer; its vehicle (Vehicle::Get of its +20),
// or 0 when there's no racer or its +0 lacks 0x40.
extern "C" REX_FUNC(sub_820C94B8) {
  SBX_TRACK_ZONES_FALLBACK(sub_820C94B8)
  const uint32_t racer =
      guest::LoadU32(base, guest::LoadU32(base, kRacers) + (ctx.r3.u32 + 717) * 4);
  if (racer == 0 || (guest::LoadU32(base, racer) & 0x40) == 0) {
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r3.u64 = guest::LoadU32(base, racer + 20);
  sub_820C0288(ctx, base);  // Vehicle::Get
}

// --- The zones' owner: a 128-slot list of the zones at +0 ---

// TrackZones::Destroy: the owner deleted when there is one, and cleared.
extern "C" REX_FUNC(sub_822888E8) {
  SBX_TRACK_ZONES_FALLBACK(sub_822888E8)
  ctx.r3.u64 = guest::LoadU32(base, kOwner);
  if (ctx.r3.u32 != 0) {
    guest_call::InFrame(ctx, base, 96, 0x8228890C,
                        [&](uint32_t) { sub_8258C868(ctx, base); });  // operator delete
    guest::StoreU32(base, kOwner, 0);
  }
}

// TrackZones::Update: r3 = the owner; each zone's vtable slot 1 (+4,
// Zone::Update) in list order.
extern "C" REX_FUNC(sub_82289118) {
  SBX_TRACK_ZONES_FALLBACK(sub_82289118)
  const uint32_t owner = ctx.r3.u32;
  for (uint32_t i = 0; i < uint32_t(LoadS16(base, owner + 520)); i++) {
    const uint32_t node = NodeAt(base, owner, owner + 516, i);
    const uint32_t zone = guest::LoadU32(base, uint32_t(Slot(base, node) + 131) * 4 + owner);
    guest_call::InFrame(ctx, base, 112, 0x8228918C, [&](uint32_t) {
      ctx.r3.u64 = zone;
      REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, zone) + 4));
    });
  }
}

// TrackZones::IsInside: r3 = the owner, r4 = the zone's place in the list;
// that zone's vtable slot 2 (+8, Zone::IsInside). No bounds check.
extern "C" REX_FUNC(sub_822891B8) {
  SBX_TRACK_ZONES_FALLBACK(sub_822891B8)
  const uint32_t owner = ctx.r3.u32;
  const uint32_t node = NodeAt(base, owner, owner + 516, ctx.r4.u32);
  const uint32_t zone = guest::LoadU32(base, uint32_t(Slot(base, node) + 131) * 4 + owner);
  ctx.r3.u64 = zone;
  REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, zone) + 8));
}
