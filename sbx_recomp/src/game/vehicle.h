// A race vehicle (the player's and the rivals' cars as the race sees them):
// its physics part (game/physics.h), the car's numbers, and where it and its
// wheels are along the course's route.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"
#include "game/physics.h"

namespace game {

using guest::f32_be;
using guest::ptr;
using guest::u16_be;
using guest::u32_be;

// Where something is along the course's route, kept up to date from a point
// in the world (RoutePosition::Update).
struct RoutePosition {
  u16_be unknown_0[2];
  u16_be section;  // The route's section, and the slice along it.
  u16_be slice;
  uint8_t unknown_8[8];
  Vector point;  // The world position it follows.
  Vector unknown_32[3];
  u16_be unknown_80[2];
  uint8_t unknown_84[12];

  // Its fields copied from another, as RoutePosition::Copy does.
  void CopyFrom(const RoutePosition& other) {
    unknown_0[0] = other.unknown_0[0];
    unknown_0[1] = other.unknown_0[1];
    section = other.section;
    slice = other.slice;
    point = other.point;
    for (int i = 0; i < 3; i++) {
      unknown_32[i] = other.unknown_32[i];
    }
    unknown_80[0] = other.unknown_80[0];
    unknown_80[1] = other.unknown_80[1];
  }
};
GUEST_SIZE(RoutePosition, 96);
GUEST_OFFSET(RoutePosition, point, 16);

struct Vehicle {
  static constexpr int kWheels = 4;

  VehicleDynamics dynamics;
  CarData car_data;
  RoutePosition route_position;  // The body's.
  RoutePosition wheel_route_positions[kWheels];
  uint8_t unknown_5712[436];
  u32_be in_world;  // Cleared when it's taken out of the collision world.
  u32_be unknown_6152;
  uint8_t unknown_6156[4];
  uint8_t track_state[112];  // Kept by the replay's snapshots.
};
GUEST_SIZE(Vehicle, 6272);
GUEST_OFFSET(Vehicle, car_data, 3376);
GUEST_OFFSET(Vehicle, route_position, 5232);
GUEST_OFFSET(Vehicle, wheel_route_positions, 5328);
GUEST_OFFSET(Vehicle, in_world, 6148);
GUEST_OFFSET(Vehicle, track_state, 6160);

// The race's vehicles, one block.
struct Vehicles {
  static constexpr int kCount = 7;
  Vehicle vehicles[kCount];
};

}  // namespace game
