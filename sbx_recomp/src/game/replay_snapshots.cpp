// The race's snapshots as our own C++ (--native_replay_snapshots, on by
// default): the copies a race's state goes through when the replay saves
// and restores it (the bodies' states, the cars' parts and members, the
// track positions). Each save and restore is one list of field copies
// (types in game/replay_snapshots.h) run one way or the other; the copies
// from snapshot to snapshot copy every field. The memcpys the originals make
// are kept as calls. The replay's own save and restore (RaceReplay's four,
// RaceSnapshot::Save, Replay::Keep), the record's and member's that loop
// over counts they read, and Member1136::Copy stay recompiled.

#include "game/replay_snapshots.h"

#include <rex/cvar.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(native_replay_snapshots, true, "SBX",
                    "Our own C++ for the race snapshots' copies (the bodies' states, the cars' "
                    "parts and members, the track positions) instead of the recompiled "
                    "functions; the replay's own save and restore stay recompiled");

namespace {

using game::BodyState;
using game::Member1136;
using game::Member1136Snapshot;
using game::Member4080;
using game::MemberWheelState;
using game::Part464;
using game::PartPoint;
using game::PartShapes;
using game::PartSnapshot;
using game::RoutePosition;
using game::TrackSnapshot;
using game::Vehicle;
using guest::At;
using guest::Guest;

bool On() { return REXCVAR_GET(native_replay_snapshots); }

// A field's bytes copied (the floats by their bits).
template <typename T>
void Copy(T& to, const T& from) {
  std::memcpy(&to, &from, sizeof(T));
}

// The same through the game's memcpy (sub_8258CB90), as the original does.
template <typename T>
void Memcpy(Guest& g, T& to, const T& from) {
  g.Call(sub_8258CB90, &to, &from, uint32_t(sizeof(T)));
}

// --- The track positions (r4 the vehicle) ---

template <typename To, typename From>
void TransferTrack(Guest& g, To& to, const From& from, RoutePosition* to_routes[5],
                   const RoutePosition* from_routes[5]) {
  to.in_world = from.in_world;
  to.unknown_6152 = from.unknown_6152;
  Memcpy(g, to.track_state, from.track_state);
  for (int i = 0; i < 5; i++) {
    to_routes[i]->CopyFrom(*from_routes[i]);
  }
}

void VehicleRoutes(Vehicle& vehicle, RoutePosition* routes[5]) {
  routes[0] = &vehicle.route_position;
  for (int i = 0; i < Vehicle::kWheels; i++) {
    routes[i + 1] = &vehicle.wheel_route_positions[i];
  }
}

// --- The parts ---

template <typename To, typename From>
void TransferPart(To& to, const From& from) {
  Copy(to.field_16, from.field_16);
  Copy(to.field_48, from.field_48);
  Copy(to.field_128, from.field_128);
  Copy(to.field_176, from.field_176);
  for (int i = 0; i < 5; i++) {
    Copy(to.points[i].field_0, from.points[i].field_0);
  }
  Copy(to.field_480, from.field_480);
}

// A part's snapshot copied as the original does: its fields from the
// start.
void CopyPartSnapshot(PartSnapshot& to, const PartSnapshot& from) {
  Copy(to.field_128, from.field_128);
  Copy(to.field_176, from.field_176);
  for (int i = 0; i < 5; i++) {
    Copy(to.points[i].field_0, from.points[i].field_0);
  }
  Copy(to.field_480, from.field_480);
  Copy(to.field_16, from.field_16);
  Copy(to.field_48, from.field_48);
}

void CopyShapes(PartShapes& to, const PartShapes& from) {
  CopyPartSnapshot(to.parts[0], from.parts[0]);
  CopyPartSnapshot(to.parts[1], from.parts[1]);
  Copy(to.field_928, from.field_928);
}

// --- The bodies' states (the same layout both ways) ---

void CopyBody(BodyState& to, const BodyState& from) {
  to.field_0 = from.field_0;
  Copy(to.field_4, from.field_4);
  Copy(to.field_16, from.field_16);
  Copy(to.field_64, from.field_64);
  Copy(to.field_128, from.field_128);
  Copy(to.field_176, from.field_176);
  Copy(to.field_400, from.field_400);
}

// --- Member1136 ---

void CopyWheel(MemberWheelState& to, const MemberWheelState& from) {
  Copy(to.field_0, from.field_0);
  Copy(to.field_4, from.field_4);
  Copy(to.field_32, from.field_32);
  Copy(to.field_96, from.field_96);
  Copy(to.field_224, from.field_224);
  Copy(to.field_228, from.field_228);
  Copy(to.field_352, from.field_352);
}

// In the original's order.
template <typename To, typename From>
void TransferMember(Guest& g, To& to, const From& from) {
  Copy(to.field_0, from.field_0);
  Copy(to.field_64, from.field_64);
  Copy(to.field_276, from.field_276);
  Copy(to.field_280, from.field_280);
  Copy(to.field_348, from.field_348);
  for (int i = 0; i < 4; i++) {
    CopyWheel(to.wheels[i], from.wheels[i]);
  }
  Copy(to.field_1904, from.field_1904);
  Copy(to.field_2016, from.field_2016);
  Copy(to.field_2176, from.field_2176);
  Memcpy(g, to.field_2204, from.field_2204);
  Copy(to.field_2348, from.field_2348);
  Copy(to.field_2380, from.field_2380);
  Copy(to.field_2388, from.field_2388);
  Copy(to.field_2400, from.field_2400);
  Copy(to.field_2404, from.field_2404);
  Copy(to.field_2428, from.field_2428);
  Copy(to.field_2416, from.field_2416);
  Copy(to.field_2424, from.field_2424);
  Copy(to.field_2440, from.field_2440);
  Copy(to.field_2444, from.field_2444);
  Copy(to.field_2448, from.field_2448);
  Copy(to.field_2456, from.field_2456);
  Copy(to.field_2464, from.field_2464);
  Copy(to.field_2504, from.field_2504);
  Copy(to.field_2508, from.field_2508);
  Copy(to.field_2512, from.field_2512);
  Copy(to.field_2516, from.field_2516);
  Copy(to.field_2624, from.field_2624);
  Copy(to.field_2628, from.field_2628);
  Copy(to.field_2664, from.field_2664);
  Copy(to.field_2924, from.field_2924);
  Copy(to.field_2928, from.field_2928);
  Copy(to.field_2932, from.field_2932);
  Copy(to.field_2948, from.field_2948);
  Copy(to.field_2952, from.field_2952);
  Copy(to.field_2960, from.field_2960);
  Copy(to.field_2964, from.field_2964);
  Memcpy(g, to.field_2984, from.field_2984);
  Memcpy(g, to.field_3044, from.field_3044);
  Copy(to.field_3104, from.field_3104);
  Copy(to.field_3128, from.field_3128);
  Copy(to.field_3132, from.field_3132);
  Copy(to.field_3168, from.field_3168);
  Copy(to.field_3240, from.field_3240);
}

}  // namespace

#define SBX_SNAPSHOTS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();               \
  if (!On()) {                       \
    __imp__##name(ctx, base);        \
    return;                          \
  }                                  \
  Guest g(ctx, base);

// Each: r3 the snapshot (or the copy's destination), r4 the live object (or
// the copy's source). The copies return their destination; the saves and
// restores leave r3 as their last copy does (not kept: no caller reads it).

// Record5232::RestoreTrack and SaveTrack: r4 the vehicle.
extern "C" REX_FUNC(sub_82104480) {
  SBX_SNAPSHOTS_FALLBACK(sub_82104480)
  TrackSnapshot& snapshot = *At<TrackSnapshot>(ctx.r3.u32);
  Vehicle& vehicle = *At<Vehicle>(ctx.r4.u32);
  RoutePosition* to[5];
  VehicleRoutes(vehicle, to);
  const RoutePosition* from[5];
  for (int i = 0; i < 5; i++) {
    from[i] = &snapshot.routes[i];
  }
  TransferTrack(g, vehicle, snapshot, to, from);
}

extern "C" REX_FUNC(sub_821044F0) {
  SBX_SNAPSHOTS_FALLBACK(sub_821044F0)
  TrackSnapshot& snapshot = *At<TrackSnapshot>(ctx.r3.u32);
  Vehicle& vehicle = *At<Vehicle>(ctx.r4.u32);
  RoutePosition* routes[5];
  VehicleRoutes(vehicle, routes);
  RoutePosition* to[5];
  const RoutePosition* from[5];
  for (int i = 0; i < 5; i++) {
    to[i] = &snapshot.routes[i];
    from[i] = routes[i];
  }
  TransferTrack(g, snapshot, vehicle, to, from);
}

// Record5232::CopyTrack: snapshot to snapshot.
extern "C" REX_FUNC(sub_82104560) {
  SBX_SNAPSHOTS_FALLBACK(sub_82104560)
  TrackSnapshot& to = *At<TrackSnapshot>(ctx.r3.u32);
  const TrackSnapshot& from = *At<TrackSnapshot>(ctx.r4.u32);
  RoutePosition* to_routes[5];
  const RoutePosition* from_routes[5];
  for (int i = 0; i < 5; i++) {
    to_routes[i] = &to.routes[i];
    from_routes[i] = &from.routes[i];
  }
  TransferTrack(g, to, from, to_routes, from_routes);
  ctx.r3.u64 = guest::AddressOf(&to);
}

// BodyState::Restore, Save and Copy.
extern "C" REX_FUNC(sub_82103320) {
  SBX_SNAPSHOTS_FALLBACK(sub_82103320)
  CopyBody(*At<BodyState>(ctx.r4.u32), *At<BodyState>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_82103B68) {
  SBX_SNAPSHOTS_FALLBACK(sub_82103B68)
  CopyBody(*At<BodyState>(ctx.r3.u32), *At<BodyState>(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_82103CC8) {
  SBX_SNAPSHOTS_FALLBACK(sub_82103CC8)
  CopyBody(*At<BodyState>(ctx.r3.u32), *At<BodyState>(ctx.r4.u32));
}

// Member1136::Restore and Save.
extern "C" REX_FUNC(sub_82102DE8) {
  SBX_SNAPSHOTS_FALLBACK(sub_82102DE8)
  TransferMember(g, *At<Member1136>(ctx.r4.u32), *At<Member1136Snapshot>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_82103638) {
  SBX_SNAPSHOTS_FALLBACK(sub_82103638)
  TransferMember(g, *At<Member1136Snapshot>(ctx.r3.u32), *At<Member1136>(ctx.r4.u32));
}

// Part464::Restore, Save and Copy.
extern "C" REX_FUNC(sub_82102C30) {
  SBX_SNAPSHOTS_FALLBACK(sub_82102C30)
  TransferPart(*At<Part464>(ctx.r4.u32), *At<PartSnapshot>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_82103480) {
  SBX_SNAPSHOTS_FALLBACK(sub_82103480)
  TransferPart(*At<PartSnapshot>(ctx.r3.u32), *At<Part464>(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_82104330) {
  SBX_SNAPSHOTS_FALLBACK(sub_82104330)
  CopyPartSnapshot(*At<PartSnapshot>(ctx.r3.u32), *At<PartSnapshot>(ctx.r4.u32));
}

// Member4080::CopyShapes (its two parts' snapshots and 100 bytes) and Copy.
extern "C" REX_FUNC(sub_821045D0) {
  SBX_SNAPSHOTS_FALLBACK(sub_821045D0)
  CopyShapes(*At<PartShapes>(ctx.r3.u32), *At<PartShapes>(ctx.r4.u32));
}

extern "C" REX_FUNC(sub_82104708) {
  SBX_SNAPSHOTS_FALLBACK(sub_82104708)
  Member4080& to = *At<Member4080>(ctx.r3.u32);
  const Member4080& from = *At<Member4080>(ctx.r4.u32);
  Copy(to.field_0, from.field_0);
  CopyShapes(to.shapes, from.shapes);
  Copy(to.field_1056, from.field_1056);
  Copy(to.field_1104, from.field_1104);
  Copy(to.field_1136, from.field_1136);
  ctx.r3.u64 = guest::AddressOf(&to);
}
