// The physics' small accessors as our own C++ (--native_vehicle_accessors,
// on by default): the rigid body's, collision object's, contact's and
// vehicle's getters and setters, the vector and record copies, and two
// debug switches (game/physics.h).

#include "game/physics.h"
#include "game/vehicle.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_vehicle_accessors, true, "SBX",
                    "Our own C++ for the vehicle accessors' set-ups, resets, small setters and getters "
                    "instead of the recompiled functions");

namespace {

using game::CollisionObject;
using game::Contact;
using game::RigidBody;
using game::Vector;
using game::Vehicle;
using game::VehicleDynamics;
using guest::AddressOf;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u16_be;
using guest::u32_be;

// The collision world's two current objects, kept in globals.
constexpr uint32_t kCollisionCurrentA = 0x826B2A58;
constexpr uint32_t kCollisionCurrentB = 0x826B2A5C;

// A race stage's object (not worked out beyond this word).
struct Stages {
  uint8_t unknown_0[808];
  u32_be unknown_808;
};

// An object the vehicle keeps at +344 (not worked out).
struct Object344 {
  uint8_t unknown_0[28];
  f32_be unknown_28;
};

// A pair of halfwords.
struct Pair {
  u16_be first, second;
};

// The collision world's 68-byte record.
struct CollisionRecord {
  u32_be words[17];
};

// The library's list head (not the C++ library's): two words cleared.
struct List {
  u32_be first;
  u32_be unknown_4;
  u32_be count;
};

bool On() { return REXCVAR_GET(native_vehicle_accessors); }

}  // namespace

#define SBX_ACCESSORS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                 \
  if (!On()) {                         \
    __imp__##name(ctx, base);          \
    return;                            \
  }                                    \
  Guest g(ctx, base);

// A getter of a member's address: r3 the object; r3 the member.
#define SBX_MEMBER_ADDRESS(address, Type, member)                    \
  extern "C" REX_FUNC(sub_##address) {                               \
    SBX_ACCESSORS_FALLBACK(sub_##address)                            \
    ctx.r3.u64 = AddressOf(&At<Type>(ctx.r3.u32)->member);           \
  }

SBX_MEMBER_ADDRESS(82098DE8, RigidBody, force)             // RigidBody::Force
SBX_MEMBER_ADDRESS(820B65A0, Contact, normal)              // Contact::Position
SBX_MEMBER_ADDRESS(820B6710, RigidBody, rotation)          // RigidBody::Field80
SBX_MEMBER_ADDRESS(820B6718, RigidBody, velocity)          // RigidBody::Velocity
SBX_MEMBER_ADDRESS(820B6720, RigidBody, unknown_144)       // RigidBody::Field144
SBX_MEMBER_ADDRESS(820B6728, RigidBody, unknown_160)       // RigidBody::Field160
SBX_MEMBER_ADDRESS(820B67C8, RigidBody, orientation)       // RigidBody::Orientation
SBX_MEMBER_ADDRESS(820B67D0, RigidBody, transform)         // RigidBody::Matrix
SBX_MEMBER_ADDRESS(820B67E8, RigidBody, angular_velocity)  // RigidBody::AngularVelocity
SBX_MEMBER_ADDRESS(820BFFA0, Vehicle, route_position)      // Vehicle::Identity
SBX_MEMBER_ADDRESS(82249148, RigidBody, position)          // RigidBody::Position
SBX_MEMBER_ADDRESS(8236FD40, RigidBody, torque)            // RigidBody::Torque

#undef SBX_MEMBER_ADDRESS

// RigidBody::AddForce and AddTorque: r3 the body, r4 the vector added.
extern "C" REX_FUNC(sub_820B67F8) {
  SBX_ACCESSORS_FALLBACK(sub_820B67F8)
  g.Call(sub_82291DF8, &At<RigidBody>(ctx.r3.u32)->force, ctx.r4.u32);  // Vector::Add
}

extern "C" REX_FUNC(sub_820B6800) {
  SBX_ACCESSORS_FALLBACK(sub_820B6800)
  g.Call(sub_82291DF8, &At<RigidBody>(ctx.r3.u32)->torque, ctx.r4.u32);  // Vector::Add
}

// CollisionObject::SetWord264: r3 the object, r4 the word. Returns r3.
extern "C" REX_FUNC(sub_820B7B60) {
  SBX_ACCESSORS_FALLBACK(sub_820B7B60)
  At<CollisionObject>(ctx.r3.u32)->unknown_264 = ctx.r4.u32;
}

// Collision::SetGlobal78 and SetGlobal7C: r3 the object, made the
// collision world's current one. Returns r3.
extern "C" REX_FUNC(sub_820B8600) {
  SBX_ACCESSORS_FALLBACK(sub_820B8600)
  *At<u32_be>(kCollisionCurrentA) = ctx.r3.u32;
}

extern "C" REX_FUNC(sub_820B8610) {
  SBX_ACCESSORS_FALLBACK(sub_820B8610)
  *At<u32_be>(kCollisionCurrentB) = ctx.r3.u32;
}

// Stages::SetWord808: r3 the stages, r4 the word. Returns r3.
extern "C" REX_FUNC(sub_820FFC00) {
  SBX_ACCESSORS_FALLBACK(sub_820FFC00)
  At<Stages>(ctx.r3.u32)->unknown_808 = ctx.r4.u32;
}

// Two debug switches: always on, always off.
extern "C" REX_FUNC(sub_82432A68) {
  SBX_ACCESSORS_FALLBACK(sub_82432A68)
  ctx.r3.u64 = 1;
}

extern "C" REX_FUNC(sub_82433058) {
  SBX_ACCESSORS_FALLBACK(sub_82433058)
  ctx.r3.u64 = 0;
}

// List::Init: r3 the list, made empty. Returns r3.
extern "C" REX_FUNC(sub_82579560) {
  SBX_ACCESSORS_FALLBACK(sub_82579560)
  List* list = At<List>(ctx.r3.u32);
  list->first = 0;
  list->count = 0;
}

// RigidBody::IsFixed: r3 the body.
extern "C" REX_FUNC(sub_820B64D0) {
  SBX_ACCESSORS_FALLBACK(sub_820B64D0)
  ctx.r3.u64 = At<RigidBody>(ctx.r3.u32)->fixed;
}

// CollisionObject::Word264: r3 the object.
extern "C" REX_FUNC(sub_820B7B58) {
  SBX_ACCESSORS_FALLBACK(sub_820B7B58)
  ctx.r3.u64 = At<CollisionObject>(ctx.r3.u32)->unknown_264;
}

// Float getters: r3 the object; the value in f1.
#define SBX_FLOAT_GETTER(address, Type, member)                             \
  extern "C" REX_FUNC(sub_##address) {                                      \
    SBX_ACCESSORS_FALLBACK(sub_##address)                                   \
    ctx.fpscr.disableFlushMode();                                           \
    ctx.f1.f64 = double(float(At<Type>(ctx.r3.u32)->member));               \
  }

SBX_FLOAT_GETTER(820B8B98, Contact, strength)           // Contact::Strength
SBX_FLOAT_GETTER(820B94F0, Object344, unknown_28)       // Object344::Float28
SBX_FLOAT_GETTER(820BFF78, VehicleDynamics, unknown_2224)  // Vehicle::Float2224
SBX_FLOAT_GETTER(820BFF80, VehicleDynamics, speed)      // Vehicle::Speed
SBX_FLOAT_GETTER(825741C0, RigidBody, mass)             // RigidBody::Mass

#undef SBX_FLOAT_GETTER

// Stages::Word808: r3 the stages.
extern "C" REX_FUNC(sub_820FFDB8) {
  SBX_ACCESSORS_FALLBACK(sub_820FFDB8)
  ctx.r3.u64 = At<Stages>(ctx.r3.u32)->unknown_808;
}

// CollisionObject::SetSize: r3 the object, r4 the size. Returns the size's
// address in the object.
extern "C" REX_FUNC(sub_820B7C08) {
  SBX_ACCESSORS_FALLBACK(sub_820B7C08)
  CollisionObject* object = At<CollisionObject>(ctx.r3.u32);
  object->size = *At<Vector>(ctx.r4.u32);
  ctx.r3.u64 = AddressOf(&object->size);
}

// Copies: r4 into r3. Each returns r3.
extern "C" REX_FUNC(sub_820B7C10) {  // CollisionRecord::Copy
  SBX_ACCESSORS_FALLBACK(sub_820B7C10)
  *At<CollisionRecord>(ctx.r3.u32) = *At<CollisionRecord>(ctx.r4.u32);
}

extern "C" REX_FUNC(sub_820FFFE8) {  // Pair::Copy
  SBX_ACCESSORS_FALLBACK(sub_820FFFE8)
  *At<Pair>(ctx.r3.u32) = *At<Pair>(ctx.r4.u32);
}

extern "C" REX_FUNC(sub_82291B20) {  // Vector::Copy
  SBX_ACCESSORS_FALLBACK(sub_82291B20)
  *At<Vector>(ctx.r3.u32) = *At<Vector>(ctx.r4.u32);
}
