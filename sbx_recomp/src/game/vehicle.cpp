// The vehicles as our own C++ (--native_vehicle, on by default): a
// vehicle's construction, refresh and removal from the collision world, its
// route positions each tick (the body's and the four wheels'), a route
// position's copy, and the vehicles' teardown (game/vehicle.h).
//
// The route positions' own update, the body's getters, the collision world,
// the input, the status and the set-ups stay recompiled.

#include "game/vehicle.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_vehicle, true, "SBX",
                    "Our own C++ for the vehicles (construction, refresh, removal, the route "
                    "positions each tick) instead of the recompiled functions; the route "
                    "positions' update and the vehicle's set-up stay recompiled");


namespace {

using game::RoutePosition;
using game::Vector;
using game::Vehicle;
using game::Vehicles;
using guest::At;
using guest::Guest;

bool On() { return REXCVAR_GET(native_vehicle); }

// Recompiled functions this file calls.
void ConstructDynamics(Guest& g, game::VehicleDynamics* d) { g.Call(sub_820B4160, d); }
void DestroyDynamics(Guest& g, game::VehicleDynamics* d) { g.Call(sub_820B0D58, d); }
void RefreshFromBody(Guest& g, game::VehicleDynamics* d) { g.Call(sub_820B6180, d); }
void ConstructCarData(Guest& g, game::CarData* data) { g.Call(sub_820AEE68, data); }
const Vector* BodyPosition(Guest& g, game::RigidBody* body) {
  return At<Vector>(g.Call(sub_82249148, body));
}
void UpdateRoutePosition(Guest& g, RoutePosition* position) { g.Call(sub_820F8350, position); }
void RemoveFromCollisionWorld(Guest& g, game::CollisionObject* object) {
  g.Call(sub_820B92D8, object);
}
// The C runtime's array destructor: `destructor` on each element, last first.
template <typename T, size_t N>
void DestroyArray(Guest& g, T (&array)[N], uint32_t destructor) {
  g.Call(sub_820900C0, array, uint32_t(sizeof(T)), uint32_t(N), destructor);
}
constexpr uint32_t kRoutePositionDestructor = 0x821611F0;

// A point's x, y and z (the w is left as it is).
void SetPoint(Vector& to, const Vector& from) {
  to.x = from.x;
  to.y = from.y;
  to.z = from.z;
}

void Construct(Guest& g, Vehicle* vehicle) {
  ConstructDynamics(g, &vehicle->dynamics);
  ConstructCarData(g, &vehicle->car_data);
  auto init = [](RoutePosition& position) {
    position.point.w = 1.0f;
    for (Vector& v : position.unknown_32) {
      v.w = 1.0f;
    }
  };
  init(vehicle->route_position);
  for (RoutePosition& position : vehicle->wheel_route_positions) {
    init(position);
  }
}

// Each tick: the body's and the wheels' world positions followed along the
// route.
void UpdateRoutePositions(Guest& g, Vehicle* vehicle) {
  SetPoint(vehicle->route_position.point, *BodyPosition(g, vehicle->dynamics.body));
  UpdateRoutePosition(g, &vehicle->route_position);
  for (int i = 0; i < Vehicle::kWheels; i++) {
    RoutePosition& position = vehicle->wheel_route_positions[i];
    SetPoint(position.point, vehicle->dynamics.wheels[i].position);
    UpdateRoutePosition(g, &position);
  }
}

void RemoveFromWorld(Guest& g, Vehicle* vehicle) {
  vehicle->in_world = 0;
  if (game::CollisionObject* object = vehicle->dynamics.collision_object) {
    RemoveFromCollisionWorld(g, object);
    vehicle->dynamics.collision_object = nullptr;
  }
  if (void* other = At<void>(vehicle->dynamics.other_object)) {
    g.CallVirtual(other, 0, 1u);  // Its deleting destructor.
  }
  vehicle->dynamics.other_object = 0;
}

void DestroyAll(Guest& g, Vehicles* all) {
  for (int i = Vehicles::kCount - 1; i >= 0; i--) {
    Vehicle& vehicle = all->vehicles[i];
    DestroyArray(g, vehicle.wheel_route_positions, kRoutePositionDestructor);
    DestroyDynamics(g, &vehicle.dynamics);
  }
}

}  // namespace

// Vehicle::Refresh: r3 the vehicle.
extern "C" REX_FUNC(sub_820BFF50) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BFF50(ctx, base);
    return;
  }
  Guest g(ctx, base);
  RefreshFromBody(g, &At<Vehicle>(ctx.r3.u32)->dynamics);
}

// Vehicles::DestroyAll: r3 the vehicles.
extern "C" REX_FUNC(sub_820C2000) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C2000(ctx, base);
    return;
  }
  Guest g(ctx, base);
  DestroyAll(g, At<Vehicles>(ctx.r3.u32));
}

// Vehicle::Construct: r3 the vehicle. Returns it.
extern "C" REX_FUNC(sub_820C2EF8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C2EF8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  Vehicle* vehicle = At<Vehicle>(ctx.r3.u32);
  Construct(g, vehicle);
  ctx.r3.u64 = guest::AddressOf(vehicle);
}

// RoutePosition::Copy: r4 into r3. Returns r3.
extern "C" REX_FUNC(sub_820C1F70) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C1F70(ctx, base);
    return;
  }
  Guest g(ctx, base);
  At<RoutePosition>(ctx.r3.u32)->CopyFrom(*At<RoutePosition>(ctx.r4.u32));
}

// Vehicle::UpdateTrackPositions: r3 the vehicle.
extern "C" REX_FUNC(sub_820C1B98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C1B98(ctx, base);
    return;
  }
  Guest g(ctx, base);
  UpdateRoutePositions(g, At<Vehicle>(ctx.r3.u32));
}

// Vehicle::RemoveFromWorld: r3 the vehicle.
extern "C" REX_FUNC(sub_820C1F00) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C1F00(ctx, base);
    return;
  }
  Guest g(ctx, base);
  RemoveFromWorld(g, At<Vehicle>(ctx.r3.u32));
}
