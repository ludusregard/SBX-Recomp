// The car physics' set-ups and accessors as our own C++
// (--native_car_physics, on by default): the car data's and vehicle's
// constructors, the rigid bodies' and collision objects' set-ups, the
// collision surfaces' values, the vehicle's input setters (throttle,
// brake, handbrake, steering ramps), the wheels' surface values and the
// wheel records' copies (game/physics.h).
//
// The vehicle's step (its suspension, tires, drivetrain, gearbox, aero and
// the rigid body's integration), the collision world and the tuning stay
// recompiled.
//
// The game's vector class sets w in its constructor (0.0 for directions,
// 1.0 for points); the constructors here do that for each vector member,
// as the game's do.

#include "game/physics.h"

#include <rex/cvar.h>

#include <cmath>

REXCVAR_DEFINE_BOOL(native_car_physics, true, "SBX",
                    "Our own C++ for the car physics' set-ups and accessors (constructors, "
                    "collision surfaces, input setters, surface values, wheel copies) instead "
                    "of the recompiled functions; the vehicle's step stays recompiled");

namespace {

using game::CarData;
using game::CollisionObject;
using game::CollisionSurfaces;
using game::Matrix;
using game::RigidBody;
using game::Vector;
using game::VehicleDynamics;
using game::Wheel;
using guest::At;
using guest::f32_be;
using guest::Guest;

constexpr uint32_t kCollisionObjectVtable = 0x82002A74;

bool On() { return REXCVAR_GET(native_car_physics); }

float Clamp01(float v) {
  // A NaN is kept as it is.
  if (v < 0.0f) {
    return 0.0f;
  }
  return v > 1.0f ? 1.0f : v;
}

void Set(Vector& v, float x, float y, float z, float w) {
  v.x = x;
  v.y = y;
  v.z = z;
  v.w = w;
}

// CarData::SetDefaults: the car's physics numbers before its spec is
// applied.
void SetCarDataDefaults(CarData* data) {
  Set(data->unknown_0, 1.0f, 1.0f, 1.0f, 1.0f);
  Set(data->unknown_32[0], 1.0f, 1.0f, 1.0f, 0.0f);
  Set(data->unknown_32[2], 1.0f, 1.0f, 1.0f, 0.0f);
  data->unknown_32[3] = *At<Vector>(game::kZeroVector);
  data->unknown_32[5] = *At<Vector>(game::kZeroVector);
  data->handbrake_rate = 0.0f;
  data->unknown_132[0] = 0.0f;
  data->unknown_132[1] = 0.0f;
  data->unknown_180 = 0.0f;
  data->unknown_264[0] = 0.0f;
  data->unknown_264[1] = 0.0f;
  for (f32_be& v : data->unknown_276) {
    v = 0.0f;
  }
  data->unknown_316 = 1.0f;
  data->unknown_320 = 2.8f;
  data->unknown_324 = 0.0f;
  data->unknown_332 = 0.0f;
  for (game::DrivetrainAxle& axle : data->axles) {
    for (f32_be& v : axle.values) {
      v = 0.0f;
    }
    axle.values[38] = 1.0f;
    axle.values[40] = 1.0f;
    axle.values[41] = 1.0f;
  }
  for (f32_be& v : data->unknown_880) {
    v = 0.0f;
  }
  data->reverse_throttle_rate = 0.0f;
  for (f32_be& v : data->drivetrain) {
    v = 0.0f;
  }
  Set(data->unknown_1704, 0.0f, 0.0f, 0.0f, 0.0f);
  for (f32_be& v : data->unknown_1720) {
    v = 0.0f;
  }
  for (f32_be& v : data->unknown_1736) {
    v = 0.0f;
  }
  data->unknown_1748 = 0.0f;
  data->unknown_1756 = 0.0f;
  Set(data->unknown_1760, -0.5f, -0.5f, -0.5f, 0.0f);
  Set(data->unknown_1776, 0.5f, 0.5f, 0.5f, 0.0f);
  data->unknown_1792[0] = 0.0f;
  data->unknown_1792[1] = 0.0f;
  for (f32_be& v : data->unknown_1808) {
    v = 0.0f;
  }
}

// Vehicle::Vehicle: the vectors' constructors (w = 0.0, 1.0 for the points
// at the end), no body or collision objects yet, then Vehicle::SetUp.
void ConstructVehicleDynamics(Guest& g, VehicleDynamics* d) {
  for (Vector& v : d->unknown_0) {
    v.w = 0.0f;
  }
  d->unknown_64.w = 0.0f;
  for (Vector& v : d->unknown_96) {
    v.w = 0.0f;
  }
  for (Vector& v : d->unknown_352) {
    v.w = 0.0f;
  }
  d->wheels[0].unknown_0.w = 0.0f;
  for (Wheel& wheel : d->wheels) {
    wheel.position.w = 0.0f;
    wheel.unknown_100[2] = 0.0f;
    wheel.unknown_112[1].w = 0.0f;
    wheel.unknown_112[2].w = 0.0f;
    wheel.unknown_160[0].w = 0.0f;
    wheel.unknown_160[1].w = 0.0f;
    wheel.unknown_360[1] = 0.0f;
  }
  for (Vector& v : d->unknown_1888) {
    v.w = 0.0f;
  }
  for (Vector& v : d->unknown_2016) {
    v.w = 0.0f;
  }
  for (Vector& v : d->unknown_2464) {
    v.w = 0.0f;
  }
  for (Vector& v : d->unknown_2528) {
    v.w = 0.0f;
  }
  for (Vector& v : d->unknown_3168) {
    v.w = 0.0f;
  }
  for (Vector& v : d->unknown_3248) {
    v.w = 1.0f;
  }
  d->body = nullptr;
  d->collision_object = nullptr;
  d->other_object = 0;
  g.Call(sub_820B0DD0, d);  // Vehicle::SetUp
  d->unknown_2624 = 0;
}

void ConstructRigidBody(Guest& g, RigidBody* body) {
  for (Vector* v : {&body->unknown_16, &body->velocity, &body->force, &body->angular_velocity,
                    &body->torque, &body->unknown_144, &body->momentum, &body->unknown_192,
                    &body->position, &body->inertia, &body->inverse_inertia,
                    &body->unknown_400, &body->unknown_480, &body->unknown_496}) {
    v->w = 0.0f;
  }
  g.Call(sub_820B7670, body);  // RigidBody::Reset
}

void InitCollisionShape(game::CollisionShape* shape) {
  shape->bounds_low.w = 0.0f;
  shape->shape = nullptr;
  shape->bounds_high.w = 0.0f;
  shape->previous = nullptr;
  shape->current = nullptr;
}

void ConstructCollisionObject(CollisionObject* object) {
  object->vtable = kCollisionObjectVtable;
  InitCollisionShape(&object->shape);
  object->unknown_160[0].w = 0.0f;
  object->unknown_160[1].w = 0.0f;
  object->size.w = 0.0f;
  object->unknown_304.w = 0.0f;
}

// The collision surfaces: each value's slot in the table, for the three
// kinds of surface and the default. The getters read `get_default` and the
// setters write `set_default`; they differ only for D, as in the game.
struct SurfaceValue {
  int kind_slots[3];  // Bits 0x01000000, 0x02000000, 0x04000000.
  int get_default;
  int set_default;
  float reset;  // Its value after CollisionSurfaces::Reset.
};
constexpr SurfaceValue kSurfaceA{{12, 5, 10}, 3, 3, 0.5f};
constexpr SurfaceValue kSurfaceB{{1, 13, 11}, 6, 6, 1.0f};
constexpr SurfaceValue kSurfaceC{{7, 8, 4}, 0, 0, 0.5f};
constexpr SurfaceValue kSurfaceD{{2, 14, 15}, 6, 9, 1.0f};
constexpr uint32_t kKindBits[3] = {0x01000000, 0x02000000, 0x04000000};

CollisionSurfaces* Surfaces() { return At<CollisionSurfaces>(game::kCollisionSurfaces); }

float SurfaceGet(const SurfaceValue& value, uint32_t bits) {
  for (int kind = 0; kind < 3; kind++) {
    if (bits & kKindBits[kind]) {
      return Surfaces()->slots[value.kind_slots[kind]];
    }
  }
  return Surfaces()->slots[value.get_default];
}

// A setter writes the default for any of the kinds, then each kind given.
void SurfaceSet(const SurfaceValue& value, uint32_t bits, float v) {
  if (bits & (kKindBits[0] | kKindBits[1] | kKindBits[2])) {
    Surfaces()->slots[value.set_default] = v;
  }
  for (int kind = 0; kind < 3; kind++) {
    if (bits & kKindBits[kind]) {
      Surfaces()->slots[value.kind_slots[kind]] = v;
    }
  }
}

void ResetSurfaces() {
  for (const SurfaceValue* value : {&kSurfaceA, &kSurfaceB, &kSurfaceC, &kSurfaceD}) {
    for (int slot : value->kind_slots) {
      Surfaces()->slots[slot] = value->reset;
    }
    Surfaces()->slots[value->get_default] = value->reset;
    Surfaces()->slots[value->set_default] = value->reset;
  }
}

// A wheel's surface record (the front wheels and the rear ones have their
// own), or nullptr.
const f32_be* SurfaceRecord(VehicleDynamics* d, uint32_t wheel) {
  if (d->has_surfaces == 0) {
    return nullptr;
  }
  const uint32_t surface = d->wheels[wheel].surface;
  const bool front = wheel == 0 || wheel == 1;
  // The surface's kind indexes past the arrays' ends as the game's does.
  const uint32_t record = *guest::Element<guest::u32_be>(
      guest::AddressOf(front ? d->front_surfaces : d->rear_surfaces), surface);
  return At<f32_be>(record);
}

// The reverse throttle's and handbrake's ramps: while held, up at the car
// data's rate to 1.0; else off at once.
void Ramp(f32_be& value, float rate, bool held, double step) {
  if (!held) {
    value = 0.0f;
    return;
  }
  value = float(std::fma(double(rate), step, double(float(value))));
  if (value > 1.0f) {
    value = 1.0f;
  }
}

void CopyWheel(Wheel* to, const Wheel* from) {
  to->unknown_0 = from->unknown_0;
  to->unknown_16[0] = from->unknown_16[0];
  to->unknown_16[1] = from->unknown_16[1];
  for (int i = 0; i < 5; i++) {
    to->unknown_20[i] = from->unknown_20[i];
  }
  to->unknown_48 = from->unknown_48;
  to->unknown_64 = from->unknown_64;
  to->position = from->position;
  to->unknown_96 = from->unknown_96;
  for (int i = 0; i < 3; i++) {
    to->unknown_112[i] = from->unknown_112[i];
  }
  to->unknown_160[0] = from->unknown_160[0];
  to->unknown_160[1] = from->unknown_160[1];
  to->surface = from->surface;
  to->unknown_196 = from->unknown_196;
  to->surface_value_12 = from->surface_value_12;
  to->surface_value_16 = from->surface_value_16;
  to->unknown_208 = from->unknown_208;
  to->surface_value_20 = from->surface_value_20;
  for (int i = 0; i < 6; i++) {
    to->unknown_216[i] = from->unknown_216[i];
  }
  for (int i = 0; i < 4; i++) {
    to->unknown_240[i] = from->unknown_240[i];
  }
  for (int i = 0; i < 23; i++) {
    to->unknown_244[i] = from->unknown_244[i];
  }
  to->unknown_336 = from->unknown_336;
  to->unknown_352[0] = from->unknown_352[0];
  to->unknown_352[1] = from->unknown_352[1];
}

// Vehicle::SetGearValues' gear: reverse counts as neutral, neutral as the
// first gear.
int GearIndex(int32_t gear) {
  if (gear == -1) {
    return 0;
  }
  return gear == 0 ? 1 : gear;
}

}  // namespace

#define SBX_CAR_PHYSICS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                   \
  if (!On()) {                           \
    __imp__##name(ctx, base);            \
    return;                              \
  }                                      \
  Guest g(ctx, base);

// CarData::SetDefaults: r3 the car data.
extern "C" REX_FUNC(sub_820AEBA0) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820AEBA0)
  SetCarDataDefaults(At<CarData>(ctx.r3.u32));
}

// CarData::CarData: r3 the car data. Returns it.
extern "C" REX_FUNC(sub_820AEE68) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820AEE68)
  CarData* data = At<CarData>(ctx.r3.u32);
  for (Vector& v : data->unknown_32) {
    v.w = 0.0f;
  }
  data->unknown_1760.w = 0.0f;
  data->unknown_1776.w = 0.0f;
  g.Call(sub_820AEBA0, data);  // CarData::SetDefaults
  ctx.r3.u64 = guest::AddressOf(data);
}

// Vehicle::Vehicle: r3 the vehicle's physics part. Returns it.
extern "C" REX_FUNC(sub_820B4160) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B4160)
  VehicleDynamics* d = At<VehicleDynamics>(ctx.r3.u32);
  ConstructVehicleDynamics(g, d);
  ctx.r3.u64 = guest::AddressOf(d);
}

// CollisionSurfaces::Reset: r3 the table's owner. Returns it.
extern "C" REX_FUNC(sub_820BC280) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820BC280)
  ResetSurfaces();
}

// CollisionWorld::ResetSurfaces.
extern "C" REX_FUNC(sub_820BC398) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820BC398)
  g.Call(sub_820BC280, game::kCollisionSurfacesOwner);  // CollisionSurfaces::Reset
}

// CollisionShape::Init: r3 the shape. Returns it.
extern "C" REX_FUNC(sub_820BA8D0) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820BA8D0)
  InitCollisionShape(At<game::CollisionShape>(ctx.r3.u32));
}

// CollisionObject::CollisionObject: r3 the object. Returns it.
extern "C" REX_FUNC(sub_820B7988) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B7988)
  ConstructCollisionObject(At<CollisionObject>(ctx.r3.u32));
}

// CollisionObject::~CollisionObject: r3 the object.
extern "C" REX_FUNC(sub_820B79E0) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B79E0)
  CollisionObject* object = At<CollisionObject>(ctx.r3.u32);
  object->vtable = kCollisionObjectVtable;
  object->unknown_268 = 0;
  g.Call(sub_820BA8F8, &object->shape);  // CollisionShape::~CollisionShape
}

// RigidBody::RigidBody: r3 the body. Returns it.
extern "C" REX_FUNC(sub_820B77C8) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B77C8)
  RigidBody* body = At<RigidBody>(ctx.r3.u32);
  ConstructRigidBody(g, body);
  ctx.r3.u64 = guest::AddressOf(body);
}

// Vehicle::RefreshFromBody: r3 the vehicle's physics part; three of the
// physics' updates (recompiled, not worked out).
extern "C" REX_FUNC(sub_820B6180) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B6180)
  VehicleDynamics* d = At<VehicleDynamics>(ctx.r3.u32);
  g.Call(sub_820B12B0, d);
  g.Call(sub_820B42B8, d);
  g.Call(sub_820B19F8, d);
}

// The collision surfaces' getters (r3 a surface's bits; the value in f1)
// and setters (r4 the bits, f1 the value). OfA to OfD are the getters
// again.
#define SBX_SURFACE_GET(address, value)                       \
  extern "C" REX_FUNC(sub_##address) {                        \
    SBX_CAR_PHYSICS_FALLBACK(sub_##address)                   \
    ctx.f1.f64 = double(SurfaceGet(value, ctx.r3.u32));       \
  }
#define SBX_SURFACE_SET(address, value)                       \
  extern "C" REX_FUNC(sub_##address) {                        \
    SBX_CAR_PHYSICS_FALLBACK(sub_##address)                   \
    SurfaceSet(value, ctx.r4.u32, float(ctx.f1.f64));         \
  }

SBX_SURFACE_GET(820BC040, kSurfaceA)  // CollisionSurface::A
SBX_SURFACE_SET(820BC160, kSurfaceA)  // CollisionSurface::SetA
SBX_SURFACE_GET(820BC088, kSurfaceB)  // CollisionSurface::B
SBX_SURFACE_SET(820BC1A8, kSurfaceB)  // CollisionSurface::SetB
SBX_SURFACE_GET(820BC0D0, kSurfaceC)  // CollisionSurface::C
SBX_SURFACE_SET(820BC1F0, kSurfaceC)  // CollisionSurface::SetC
SBX_SURFACE_GET(820BC118, kSurfaceD)  // CollisionSurface::D
SBX_SURFACE_SET(820BC238, kSurfaceD)  // CollisionSurface::SetD
SBX_SURFACE_GET(820BC378, kSurfaceC)  // CollisionSurface::OfC
SBX_SURFACE_GET(820BC380, kSurfaceD)  // CollisionSurface::OfD
SBX_SURFACE_GET(820BC388, kSurfaceA)  // CollisionSurface::OfA
SBX_SURFACE_GET(820BC390, kSurfaceB)  // CollisionSurface::OfB

#undef SBX_SURFACE_GET
#undef SBX_SURFACE_SET

// Vehicle::SetBrake, SetHandbrake and SetThrottle: r3 the vehicle's physics
// part, f1 the input (clamped to 0.0 to 1.0, and left in f1).
#define SBX_INPUT_SETTER(address, field)                                  \
  extern "C" REX_FUNC(sub_##address) {                                    \
    SBX_CAR_PHYSICS_FALLBACK(sub_##address)                               \
    const float input = Clamp01(float(ctx.f1.f64));                       \
    At<VehicleDynamics>(ctx.r3.u32)->field = input;                       \
    ctx.f1.f64 = double(input);                                           \
  }

SBX_INPUT_SETTER(820B3CA0, brake)
SBX_INPUT_SETTER(820B3CD0, handbrake)
SBX_INPUT_SETTER(820B3D00, throttle)

#undef SBX_INPUT_SETTER

// Vehicle::SetValue2388: r3 the vehicle's physics part, f1 a value, r4 a
// flag. Keeps the value (clamped) and 1 - value x 0.9 (clamped).
extern "C" REX_FUNC(sub_820B3AB8) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B3AB8)
  VehicleDynamics* d = At<VehicleDynamics>(ctx.r3.u32);
  const double value = ctx.f1.f64;
  d->unknown_2388 = Clamp01(float(value));
  d->unknown_2385 = uint8_t(ctx.r4.u32);
  d->unknown_2396 = Clamp01(float(std::fma(-value, double(0.9f), 1.0)));
}

// Vehicle::SetGearValues: r3 the vehicle's physics part. The gear's two
// ratios from the car data's gearbox.
extern "C" REX_FUNC(sub_820B3B28) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B3B28)
  VehicleDynamics* d = At<VehicleDynamics>(ctx.r3.u32);
  CarData* data = d->car_data;
  const int gear = GearIndex(d->gear);
  // The gearbox's two tables, and the scales they're used with.
  const float ratio_a = data->drivetrain[74 + gear] * data->drivetrain[83];
  const float ratio_b = data->drivetrain[58 + gear] * data->drivetrain[69];
  d->unknown_2324 = d->unknown_2328;
  d->unknown_2320 = 0.0f;
  d->unknown_2312 = d->unknown_2336 + d->unknown_2328;
  d->gear_value_2304 = std::fabs(ratio_a);
  d->gear_value_2308 = ratio_b;
}

// Vehicle::RampReverseThrottle and RampHandbrake: r3 the vehicle's physics
// part, r4 whether it's held, f1 the step's length.
extern "C" REX_FUNC(sub_820B0360) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B0360)
  VehicleDynamics* d = At<VehicleDynamics>(ctx.r3.u32);
  Ramp(d->reverse_throttle, d->car_data->reverse_throttle_rate, (ctx.r4.u32 & 0xFF) != 0,
       ctx.f1.f64);
}

extern "C" REX_FUNC(sub_820B03A8) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B03A8)
  VehicleDynamics* d = At<VehicleDynamics>(ctx.r3.u32);
  Ramp(d->handbrake, d->car_data->handbrake_rate, (ctx.r4.u32 & 0xFF) != 0, ctx.f1.f64);
}

// Vehicle::SurfaceValue: r3 the vehicle's physics part, r4 the wheel; the
// surface record's first value in f1, or 0.0.
extern "C" REX_FUNC(sub_820B03F0) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B03F0)
  const f32_be* record = SurfaceRecord(At<VehicleDynamics>(ctx.r3.u32), ctx.r4.u32);
  ctx.f1.f64 = record != nullptr ? double(float(record[0])) : 0.0;
}

// The surface values a wheel keeps: one of the surface record's values
// (or a default without one) into the wheel, and in f1.
#define SBX_SURFACE_VALUE(address, index, fallback, field)                     \
  extern "C" REX_FUNC(sub_##address) {                                         \
    SBX_CAR_PHYSICS_FALLBACK(sub_##address)                                    \
    VehicleDynamics* d = At<VehicleDynamics>(ctx.r3.u32);                      \
    const uint32_t wheel = ctx.r4.u32;                                         \
    const f32_be* record = SurfaceRecord(d, wheel);                            \
    const float value = record != nullptr ? float(record[index]) : (fallback); \
    guest::Element<Wheel>(guest::AddressOf(d->wheels), wheel)->field = value;  \
    ctx.f1.f64 = double(value);                                                \
  }

SBX_SURFACE_VALUE(820B0448, 5, 0.0f, surface_value_20)  // Vehicle::SurfaceValue20
SBX_SURFACE_VALUE(820B04A0, 3, 1.0f, surface_value_12)  // Vehicle::SurfaceValue12
SBX_SURFACE_VALUE(820B04F8, 4, 1.0f, surface_value_16)  // Vehicle::SurfaceValue16

#undef SBX_SURFACE_VALUE

// Wheel::Copy: r4 into r3. Returns r3.
extern "C" REX_FUNC(sub_820B0938) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B0938)
  CopyWheel(At<Wheel>(ctx.r3.u32), At<Wheel>(ctx.r4.u32));
}

// WheelRecord2::Copy: r4's 120 bytes into r3. Returns r3.
extern "C" REX_FUNC(sub_820B0BA8) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B0BA8)
  struct WheelRecord2 {
    guest::u32_be words[30];
  };
  *At<WheelRecord2>(ctx.r3.u32) = *At<WheelRecord2>(ctx.r4.u32);
}

// RigidBody::SetCenterOfMassPosition: r3 the body, r4 the position, set in
// its orientation's matrix; then RigidBody::UpdateOrigin.
extern "C" REX_FUNC(sub_820B7050) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B7050)
  RigidBody* body = At<RigidBody>(ctx.r3.u32);
  g.Call(sub_82291258, &body->orientation, ctx.r4.u32);  // Matrix::SetTranslation
  g.Call(sub_820B6D10, body);                            // RigidBody::UpdateOrigin
}

// RigidBody::SetOrientation: r3 the body, r4 the rotation (a quaternion,
// normalized here), then its orientation and origin brought up to date.
extern "C" REX_FUNC(sub_820B7088) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B7088)
  RigidBody* body = At<RigidBody>(ctx.r3.u32);
  const Vector* rotation = At<Vector>(ctx.r4.u32);
  body->rotation = *rotation;
  g.Call(sub_82291950, &body->rotation, rotation);  // Quaternion::Normalize
  g.Call(sub_820B6DD8, body);                       // RigidBody::UpdateOrientation
  g.Call(sub_820B6D10, body);                       // RigidBody::UpdateOrigin
}

// RigidBody::SetVelocity: r3 the body, r4 the velocity. Kept as momentum
// (the velocity times the mass), then RigidBody::UpdateVelocity.
extern "C" REX_FUNC(sub_820B70D8) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B70D8)
  RigidBody* body = At<RigidBody>(ctx.r3.u32);
  const uint32_t velocity = ctx.r4.u32;
  guest::StackFrame frame(ctx, 112);
  Vector* scaled = At<Vector>(frame.Local(80));
  ctx.f1.f64 = double(float(body->mass));
  g.Call(sub_82291E70, scaled, velocity);  // Vector::Scaled
  body->momentum = *scaled;
  g.Call(sub_820B6F68, body);  // RigidBody::UpdateVelocity
}

// RigidBody::SetInertia: r3 the body, r4 the inertia (x, y, z). Kept with
// its inverse; then RigidBody::UpdateOrientation.
extern "C" REX_FUNC(sub_820B7120) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B7120)
  RigidBody* body = At<RigidBody>(ctx.r3.u32);
  const Vector* inertia = At<Vector>(ctx.r4.u32);
  body->inertia.x = inertia->x;
  body->inertia.y = inertia->y;
  body->inertia.z = inertia->z;
  body->inverse_inertia.x = 1.0f / body->inertia.x;
  body->inverse_inertia.y = 1.0f / body->inertia.y;
  body->inverse_inertia.z = 1.0f / body->inertia.z;
  g.Call(sub_820B6DD8, body);  // RigidBody::UpdateOrientation
}

// Matrix::Copy (the physics' own): r4 into r3. Returns r3.
extern "C" REX_FUNC(sub_820AFA60) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820AFA60)
  *At<Matrix>(ctx.r3.u32) = *At<Matrix>(ctx.r4.u32);
}

// RigidBody::SetFloat48: r3 the body, r4 the value's address. Returns r3.
extern "C" REX_FUNC(sub_820B6730) {
  SBX_CAR_PHYSICS_FALLBACK(sub_820B6730)
  At<RigidBody>(ctx.r3.u32)->unknown_48 = *At<f32_be>(ctx.r4.u32);
}
