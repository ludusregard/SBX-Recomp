// The car physics' set-ups and accessors as our own C++
// (--native_car_physics, on by default): the car data's and vehicle's
// constructors, the rigid bodies' and collision objects' set-ups, the
// collision surfaces' values, the vehicle's input setters (throttle,
// brake, handbrake, steering ramps), the wheels' surface values and the
// wheel records' copies.
//
// Written from SBX's generated code. Float steps are the originals': a
// single add, subtract, multiply or divide in float, the fused ones
// (fmadds, fnmsubs) as float(std::fma(...)) over the registers' doubles,
// compares as they branch (an unordered compare takes neither ble nor bge).
// Arguments in f1 are the callers' doubles, used as they are. The
// vehicle's step (its suspension, tires, drivetrain, gearbox, aero and
// the rigid body's integration), the collision world and the tuning stay
// recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

#include <cmath>

REXCVAR_DEFINE_BOOL(native_car_physics, true, "SBX",
                    "Our own C++ for the car physics' set-ups and accessors (constructors, "
                    "collision surfaces, input setters, surface values, wheel copies) instead "
                    "of the recompiled functions; the vehicle's step stays recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// CarData::SetDefaults (sub_820AEBA0).
// 25 stores
constexpr Fill kCarData_SetDefaults0[] = {
    {0, kImage, 0x82000A20, 4, 4}, {32, kImage, 0x82000A20, 3, 4}, {44, kImage, 0x82000A24}, {64, kImage, 0x82000A20, 3, 4},
    {76, kImage, 0x82000A24}, {80, kImage, 0x82955D00}, {84, kImage, 0x82955D04}, {88, kImage, 0x82955D08},
    {92, kImage, 0x82955D0C}, {112, kImage, 0x82955D00}, {116, kImage, 0x82955D04}, {120, kImage, 0x82955D08},
    {124, kImage, 0x82955D0C}, {128, kImage, 0x82000A24, 3, 4}, {264, kImage, 0x82000A24, 2, 4},
};
// 4 stores
constexpr Fill kCarData_SetDefaults1[] = {
    {316, kImage, 0x82000A20}, {324, kImage, 0x82000A24, 2, 8}, {320, kImage, 0x82002928},
};
// 1 stores
constexpr Fill kCarData_SetDefaults2[] = {
    {1264, kImage, 0x82000A24},
};
// 32 stores
constexpr Fill kCarData_SetDefaults3[] = {
    {180, kImage, 0x82000A24, 2, 1524}, {1808, kImage, 0x82000A24}, {1708, kImage, 0x82000A24, 3, 4}, {1812, kImage, 0x82000A24, 2, 4},
    {1720, kImage, 0x82000A24, 3, 4}, {1736, kImage, 0x82000A24, 3, 4}, {1756, kImage, 0x82000A24}, {1748, kImage, 0x82000A24},
    {1760, kImage, 0x82001BE0, 3, 4}, {1772, kImage, 0x82000A24}, {1776, kImage, 0x82001BDC, 3, 4}, {1788, kImage, 0x82000A24, 3, 4},
    {496, kImage, 0x82000A20, 2, 4}, {488, kImage, 0x82000A20, 2, 280}, {772, kImage, 0x82000A20}, {760, kImage, 0x82000A20},
};

// CarData::CarData (sub_820AEE68).
// 22 stores
constexpr Fill kCarData_CarData0[] = {
    {44, kImage, 0x82000A24, 6, 16}, {348, kImage, 0x82000A24, 2, 80}, {588, kImage, 0x82000A24, 2, 32}, {700, kImage, 0x82000A24, 2, 160},
    {892, kImage, 0x82000A24, 2, 16}, {988, kImage, 0x82000A24, 2, 16}, {1084, kImage, 0x82000A24, 2, 16}, {1180, kImage, 0x82000A24, 2, 16},
    {1772, kImage, 0x82000A24, 2, 16},
};

// Vehicle::Vehicle (sub_820B4160).
// 74 stores
constexpr Fill kVehicle_Vehicle0[] = {
    {12, kImage, 0x82000A24, 3, 16}, {76, kImage, 0x82000A24, 2, 32}, {124, kImage, 0x82000A24, 2, 16}, {364, kImage, 0x82000A24, 4, 16},
    {492, kImage, 0x82000A24, 2, 16}, {540, kImage, 0x82000A24, 4, 16}, {764, kImage, 0x82000A24, 2, 96}, {876, kImage, 0x82000A24, 2, 32},
    {924, kImage, 0x82000A24, 3, 16}, {1132, kImage, 0x82000A24, 2, 96}, {1244, kImage, 0x82000A24, 2, 32}, {1292, kImage, 0x82000A24, 3, 16},
    {1500, kImage, 0x82000A24, 2, 96}, {1612, kImage, 0x82000A24, 2, 32}, {1660, kImage, 0x82000A24, 3, 16}, {1868, kImage, 0x82000A24, 2, 32},
    {1916, kImage, 0x82000A24, 5, 16}, {2028, kImage, 0x82000A24, 6, 16}, {2476, kImage, 0x82000A24, 2, 16}, {2540, kImage, 0x82000A24, 6, 16},
    {3180, kImage, 0x82000A24, 4, 16}, {3260, kImage, 0x82000A20, 8, 16}, {336, kWord, 0x0, 3, 4},
};
// 1 stores
constexpr Fill kVehicle_Vehicle1[] = {
    {2624, kByte, 0x0},
};

// CollisionSurfaces::Reset (sub_820BC280).
// 16 stores
constexpr Fill kCollisionSurfaces_Reset0[] = {
    {0x826B2AD0, kAbsImage, 0x82001BDC}, {0x826B2AF4, kAbsImage, 0x82000A20}, {0x826B2ADC, kAbsImage, 0x82001BDC}, {0x826B2AE8, kAbsImage, 0x82000A20},
    {0x826B2AF0, kAbsImage, 0x82001BDC}, {0x826B2B08, kAbsImage, 0x82000A20}, {0x826B2AE4, kAbsImage, 0x82001BDC}, {0x826B2B04, kAbsImage, 0x82000A20},
    {0x826B2AEC, kAbsImage, 0x82001BDC}, {0x826B2AD8, kAbsImage, 0x82000A20}, {0x826B2B00, kAbsImage, 0x82001BDC}, {0x826B2AD4, kAbsImage, 0x82000A20},
    {0x826B2AE0, kAbsImage, 0x82001BDC}, {0x826B2B0C, kAbsImage, 0x82000A20}, {0x826B2AF8, kAbsImage, 0x82001BDC}, {0x826B2AFC, kAbsImage, 0x82000A20},
};

// CollisionWorld::ResetSurfaces (sub_820BC398).
// 16 stores
constexpr Fill kCollisionWorld_ResetSurfaces0[] = {
    {0x826B2AD0, kAbsImage, 0x82001BDC}, {0x826B2AF4, kAbsImage, 0x82000A20}, {0x826B2ADC, kAbsImage, 0x82001BDC}, {0x826B2AE8, kAbsImage, 0x82000A20},
    {0x826B2AF0, kAbsImage, 0x82001BDC}, {0x826B2B08, kAbsImage, 0x82000A20}, {0x826B2AE4, kAbsImage, 0x82001BDC}, {0x826B2B04, kAbsImage, 0x82000A20},
    {0x826B2AEC, kAbsImage, 0x82001BDC}, {0x826B2AD8, kAbsImage, 0x82000A20}, {0x826B2B00, kAbsImage, 0x82001BDC}, {0x826B2AD4, kAbsImage, 0x82000A20},
    {0x826B2AE0, kAbsImage, 0x82001BDC}, {0x826B2B0C, kAbsImage, 0x82000A20}, {0x826B2AF8, kAbsImage, 0x82001BDC}, {0x826B2AFC, kAbsImage, 0x82000A20},
};

// CollisionShape::Init (sub_820BA8D0).
// 5 stores
constexpr Fill kCollisionShape_Init0[] = {
    {28, kImage, 0x82000A24}, {4, kWord, 0x0}, {44, kImage, 0x82000A24}, {8, kWord, 0x0},
    {0, kWord, 0x0},
};

// CollisionObject::CollisionObject (sub_820B7988).
// 10 stores
constexpr Fill kCollisionObject_CollisionObject0[] = {
    {0, kWord, 0x82002A74}, {60, kImage, 0x82000A24}, {36, kWord, 0x0}, {76, kImage, 0x82000A24},
    {40, kWord, 0x0}, {32, kWord, 0x0}, {172, kImage, 0x82000A24, 2, 16}, {220, kImage, 0x82000A24, 2, 96},
};

// CollisionObject::~CollisionObject (sub_820B79E0).
// 2 stores
constexpr Fill kCollisionObject_DestroyCollisionObject0[] = {
    {0, kWord, 0x82002A74}, {268, kWord, 0x0},
};

// RigidBody::RigidBody (sub_820B77C8).
// 14 stores
constexpr Fill kRigidBody_RigidBody0[] = {
    {28, kImage, 0x82000A24, 2, 16}, {76, kImage, 0x82000A24, 3, 32}, {156, kImage, 0x82000A24, 2, 32}, {204, kImage, 0x82000A24, 2, 16},
    {364, kImage, 0x82000A24, 2, 16}, {412, kImage, 0x82000A24, 2, 80}, {508, kImage, 0x82000A24},
};

bool On() { return REXCVAR_GET(native_car_physics); }

using guest::LoadFloat;
using guest::LoadU32;
using guest::LoadU8;
using guest::StoreFloat;
using guest::StoreU32;
using guest::StoreU8;

constexpr uint32_t kOne = 0x82000A20;   // 1.0f
constexpr uint32_t kZero = 0x82000A24;  // 0.0f
constexpr uint32_t kSteerRate = 0x82002A04;
constexpr uint32_t kSurfaces = 0x826B0000;  // + the offsets below

// float(fma(a, b, c)): fmadds.
inline float FusedMultiplyAdd(double a, double b, double c) {
  return float(std::fma(a, b, c));
}

// float(-fma(a, b, -c)): fnmsubs (c - a x b, fused).
inline float FusedNegativeMultiplySubtract(double a, double b, double c) {
  return float(-std::fma(a, b, -c));
}

// The collision surfaces' four values, by a surface's bits 0x01000000,
// 0x02000000 and 0x04000000 (in that order), else the default: each
// getter's and setter's words (the setters write the default slot for any
// of the three bits, then the bits' own). The D setter's default slot
// differs from its getter's, as in the original.
struct Surface {
  uint32_t bit7, bit6, bit5, get_default, set_default;
};
constexpr Surface kSurfaceA{11008, 10980, 11000, 10972, 10972};
constexpr Surface kSurfaceB{10964, 11012, 11004, 10984, 10984};
constexpr Surface kSurfaceC{10988, 10992, 10976, 10960, 10960};
constexpr Surface kSurfaceD{10968, 11016, 11020, 10984, 10996};

double SurfaceGet(uint8_t* base, const Surface& s, uint32_t bits) {
  uint32_t at = s.get_default;
  if (bits & 0x01000000) {
    at = s.bit7;
  } else if (bits & 0x02000000) {
    at = s.bit6;
  } else if (bits & 0x04000000) {
    at = s.bit5;
  }
  return LoadFloat(base, kSurfaces + at);
}

void SurfaceSet(uint8_t* base, const Surface& s, uint32_t bits, double value) {
  const float v = float(value);
  if (bits & 0x07000000) StoreFloat(base, kSurfaces + s.set_default, v);
  if (bits & 0x01000000) StoreFloat(base, kSurfaces + s.bit7, v);
  if (bits & 0x02000000) StoreFloat(base, kSurfaces + s.bit6, v);
  if (bits & 0x04000000) StoreFloat(base, kSurfaces + s.bit5, v);
}

// A value clamped to [0.0, 1.0] the way the input setters branch: below
// 0.0 (blt) is 0.0; then anything not at most 1.0 (a NaN too) is 1.0.
double ClampInput(uint8_t* base, double v) {
  const double zero = LoadFloat(base, kZero);
  if (v < zero) {
    return zero;
  }
  const double one = LoadFloat(base, kOne);
  return v <= one ? v : one;
}

// SetValue2388's clamp: not at least 0.0 (a NaN too) is 0.0, above 1.0 is
// 1.0.
double ClampLow(double v, double zero, double one) {
  if (!(v >= zero)) {
    return zero;
  }
  return v <= one ? v : one;
}

// A wheel's surface record: the vehicle's +2664 set, then for wheel i
// ([vehicle + 368i + 592] its surface) the record at [vehicle + 4 x
// (surface + 667)] (the front wheels, 0 and 1) or + 699 (the rear); 0 when
// there's none.
uint32_t SurfaceRecord(uint8_t* base, uint32_t vehicle, uint32_t wheel) {
  if (LoadU8(base, vehicle + 2664) == 0) {
    return 0;
  }
  const uint32_t surface = LoadU32(base, vehicle + 368 * wheel + 592);
  const uint32_t slot = surface + (int32_t(wheel) == 0 || int32_t(wheel) == 1 ? 667 : 699);
  return LoadU32(base, vehicle + (slot << 2));
}

// The ramps: with r4's low byte set, v = fmadds(rate, f1, v) up to 1.0
// (anything not at most 1.0 is 1.0); else v = 0.0.
void Ramp(uint8_t* base, uint32_t vehicle, uint32_t at, uint32_t rate_at, uint32_t on,
          double step) {
  if ((on & 0xFF) == 0) {
    StoreU32(base, vehicle + at, LoadU32(base, kZero));
    return;
  }
  const double rate = LoadFloat(base, LoadU32(base, vehicle + 272) + rate_at);
  const float v = FusedMultiplyAdd(rate, step, LoadFloat(base, vehicle + at));
  StoreFloat(base, vehicle + at, v);
  const float one = LoadFloat(base, kOne);
  if (!(v <= one)) {
    StoreFloat(base, vehicle + at, one);
  }
}

// The words and bytes copied, in order, by Wheel::Copy (sub_820B0938);
// a byte run is marked.
struct CopyStep {
  uint32_t offset, bytes;
  bool byte_wise;
};
constexpr CopyStep kWheelCopy[] = {
    {0, 16, false},   {16, 2, true},    {20, 20, false},  {48, 16, false},
    {64, 16, false},  {80, 16, false},  {96, 4, false},   {112, 16, false},
    {128, 16, false}, {144, 16, false}, {160, 16, false}, {176, 64, false},
    {240, 4, true},   {244, 92, false}, {336, 16, false}, {352, 8, false},
};

void CopySteps(uint8_t* base, uint32_t to, uint32_t from, const CopyStep* steps, size_t n) {
  for (size_t s = 0; s < n; s++) {
    const CopyStep& c = steps[s];
    if (c.byte_wise) {
      for (uint32_t i = 0; i < c.bytes; i++) {
        StoreU8(base, to + c.offset + i, LoadU8(base, from + c.offset + i));
      }
    } else {
      guest_memory::CopyRun(base, to, from, {c.offset, c.bytes});
    }
  }
}

}  // namespace

// CarData::SetDefaults.
extern "C" REX_FUNC(sub_820AEBA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820AEBA0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCarData_SetDefaults0, args);
  guest_memory::Clear(base, object + 276, 32);
  guest_memory::Apply(base, object, kCarData_SetDefaults1, args);
  guest_memory::Clear(base, object + 336, 272);
  guest_memory::Clear(base, object + 608, 272);
  guest_memory::Clear(base, object + 880, 384);
  guest_memory::Apply(base, object, kCarData_SetDefaults2, args);
  guest_memory::Clear(base, object + 1268, 436);
  guest_memory::Apply(base, object, kCarData_SetDefaults3, args);
  ctx.r3.u64 = object + 1776;
}

// CarData::CarData.
extern "C" REX_FUNC(sub_820AEE68) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820AEE68(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCarData_CarData0, args);
  ctx.r3.u64 = object + 0;
  sub_820AEBA0(ctx, base);
  ctx.r3.u64 = object;
}

// Vehicle::Vehicle.
extern "C" REX_FUNC(sub_820B4160) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B4160(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kVehicle_Vehicle0, args);
  ctx.r3.u64 = object + 0;
  sub_820B0DD0(ctx, base);
  guest_memory::Apply(base, object, kVehicle_Vehicle1, args);
  ctx.r3.u64 = object;
}

// CollisionSurfaces::Reset.
extern "C" REX_FUNC(sub_820BC280) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BC280(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCollisionSurfaces_Reset0, args);
  ctx.r3.u64 = object;
}

// CollisionWorld::ResetSurfaces.
extern "C" REX_FUNC(sub_820BC398) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BC398(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCollisionWorld_ResetSurfaces0, args);
  ctx.r3.u64 = 0xFFFFFFFF826B2B10ull;
}

// CollisionShape::Init.
extern "C" REX_FUNC(sub_820BA8D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA8D0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCollisionShape_Init0, args);
  ctx.r3.u64 = object;
}

// CollisionObject::CollisionObject.
extern "C" REX_FUNC(sub_820B7988) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B7988(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCollisionObject_CollisionObject0, args);
  ctx.r3.u64 = object;
}

// CollisionObject::~CollisionObject.
extern "C" REX_FUNC(sub_820B79E0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B79E0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCollisionObject_DestroyCollisionObject0, args);
  ctx.r3.u64 = object + 32;
  ctx.r9.u64 = 0x0u;
  ctx.r10.u64 = 0xFFFFFFFF82002A74u;
  sub_820BA8F8(ctx, base);
}

// RigidBody::RigidBody.
extern "C" REX_FUNC(sub_820B77C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B77C8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRigidBody_RigidBody0, args);
  ctx.r3.u64 = object + 0;
  sub_820B7670(ctx, base);
  ctx.r3.u64 = object;
}

// Vehicle::RefreshFromBody.
extern "C" REX_FUNC(sub_820B6180) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B6180(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_820B12B0(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820B42B8(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820B19F8(ctx, base);
}


// The collision surfaces' getters (f1 by r3's bits) and setters (r4's
// bits, f1); OfA to OfD are the getters again.
#define SBX_SURFACE_GET(address, surface)              \
  extern "C" REX_FUNC(sub_##address) {                  \
    REX_FUNC_PROLOGUE();                                \
    if (!On()) {                                        \
      __imp__sub_##address(ctx, base);                  \
      return;                                           \
    }                                                   \
    ctx.f1.f64 = SurfaceGet(base, surface, ctx.r3.u32); \
  }

#define SBX_SURFACE_SET(address, surface)                  \
  extern "C" REX_FUNC(sub_##address) {                     \
    REX_FUNC_PROLOGUE();                                   \
    if (!On()) {                                           \
      __imp__sub_##address(ctx, base);                     \
      return;                                              \
    }                                                      \
    SurfaceSet(base, surface, ctx.r4.u32, ctx.f1.f64);     \
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


// Vehicle::SetBrake (+292), SetHandbrake (+296), SetThrottle (+284): f1
// clamped to [0.0, 1.0].
#define SBX_INPUT_SETTER(address, at)                                  \
  extern "C" REX_FUNC(sub_##address) {                                 \
    REX_FUNC_PROLOGUE();                                               \
    if (!On()) {                                                       \
      __imp__sub_##address(ctx, base);                                 \
      return;                                                          \
    }                                                                  \
    ctx.f1.f64 = ClampInput(base, ctx.f1.f64);                         \
    StoreFloat(base, ctx.r3.u32 + at, float(ctx.f1.f64));              \
  }

SBX_INPUT_SETTER(820B3CA0, 292)
SBX_INPUT_SETTER(820B3CD0, 296)
SBX_INPUT_SETTER(820B3D00, 284)

#undef SBX_INPUT_SETTER

// Vehicle::SetValue2388: +2388 = f1 clamped, +2385 = r4's byte, +2396 =
// (1.0 - f1 x [0x82002A04]) clamped (fused).
extern "C" REX_FUNC(sub_820B3AB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B3AB8(ctx, base);
    return;
  }
  const uint32_t vehicle = ctx.r3.u32;
  const double zero = LoadFloat(base, kZero), one = LoadFloat(base, kOne);
  const double v = ctx.f1.f64;
  StoreFloat(base, vehicle + 2388, float(ClampLow(v, zero, one)));
  StoreU8(base, vehicle + 2385, uint8_t(ctx.r4.u32));
  const double rest = FusedNegativeMultiplySubtract(v, LoadFloat(base, kSteerRate), one);
  StoreFloat(base, vehicle + 2396, float(ClampLow(rest, zero, one)));
}

// Vehicle::SetGearValues: the gear (+2252; -1 is 0, 0 is 1) picks the car
// data's (+272's +1268) two ratios: +2304 = |ratio(74 + g) x [+332]|,
// +2308 = ratio(58 + g) x [+276]; +2312 = +2336 + +2328, +2320 = 0.0,
// +2324 = +2328.
extern "C" REX_FUNC(sub_820B3B28) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B3B28(ctx, base);
    return;
  }
  const uint32_t vehicle = ctx.r3.u32;
  int32_t gear = int32_t(LoadU32(base, vehicle + 2252));
  if (gear == -1) {
    gear = 0;
  } else if (gear == 0) {
    gear = 1;
  }
  const uint32_t data = LoadU32(base, vehicle + 272) + 1268;
  const float a = LoadFloat(base, vehicle + 2328);
  const float sum = LoadFloat(base, vehicle + 2336) + a;
  const float ratio = LoadFloat(base, data + (uint32_t(gear + 74) << 2)) *
                      LoadFloat(base, data + 332);
  StoreFloat(base, vehicle + 2324, a);
  StoreU32(base, vehicle + 2320, LoadU32(base, kZero));
  StoreFloat(base, vehicle + 2312, sum);
  StoreFloat(base, vehicle + 2304, std::fabs(ratio));
  StoreFloat(base, vehicle + 2308, LoadFloat(base, data + (uint32_t(gear + 58) << 2)) *
                                       LoadFloat(base, data + 276));
}

// Vehicle::RampReverseThrottle (+288, rate [+272] + 1264) and
// RampHandbrake (+296, rate +128).
extern "C" REX_FUNC(sub_820B0360) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B0360(ctx, base);
    return;
  }
  Ramp(base, ctx.r3.u32, 288, 1264, ctx.r4.u32, ctx.f1.f64);
}

extern "C" REX_FUNC(sub_820B03A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B03A8(ctx, base);
    return;
  }
  Ramp(base, ctx.r3.u32, 296, 128, ctx.r4.u32, ctx.f1.f64);
}

// Vehicle::SurfaceValue: r4 the wheel; f1 the surface record's +0, or 0.0.
extern "C" REX_FUNC(sub_820B03F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B03F0(ctx, base);
    return;
  }
  const uint32_t record = SurfaceRecord(base, ctx.r3.u32, ctx.r4.u32);
  ctx.f1.f64 = LoadFloat(base, record != 0 ? record : kZero);
}

// The surface values a wheel keeps: the record's +20, +12 or +16 (0.0,
// 1.0 or 1.0 without one) into the wheel's +612, +600 or +604, and f1.
#define SBX_SURFACE_VALUE(address, field, fallback, keep)                    \
  extern "C" REX_FUNC(sub_##address) {                                       \
    REX_FUNC_PROLOGUE();                                                     \
    if (!On()) {                                                             \
      __imp__sub_##address(ctx, base);                                       \
      return;                                                                \
    }                                                                        \
    const uint32_t vehicle = ctx.r3.u32, wheel = ctx.r4.u32;                 \
    const uint32_t record = SurfaceRecord(base, vehicle, wheel);             \
    ctx.f1.f64 = LoadFloat(base, record != 0 ? record + field : fallback);   \
    StoreFloat(base, vehicle + 368 * wheel + keep, float(ctx.f1.f64));       \
  }

SBX_SURFACE_VALUE(820B0448, 20, kZero, 612)  // Vehicle::SurfaceValue20
SBX_SURFACE_VALUE(820B04A0, 12, kOne, 600)   // Vehicle::SurfaceValue12
SBX_SURFACE_VALUE(820B04F8, 16, kOne, 604)   // Vehicle::SurfaceValue16

#undef SBX_SURFACE_VALUE

// Wheel::Copy: r4 into r3, field by field; returns r3.
extern "C" REX_FUNC(sub_820B0938) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B0938(ctx, base);
    return;
  }
  CopySteps(base, ctx.r3.u32, ctx.r4.u32, kWheelCopy, std::size(kWheelCopy));
}

// WheelRecord2::Copy: the 120 bytes, word by word; returns r3.
extern "C" REX_FUNC(sub_820B0BA8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B0BA8(ctx, base);
    return;
  }
  guest_memory::CopyRun(base, ctx.r3.u32, ctx.r4.u32, {0, 120});
}

// RigidBody::SetCenterOfMassPosition: sub_82291258 on +224 with r4, then
// RigidBody::UpdateOrigin.
extern "C" REX_FUNC(sub_820B7050) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B7050(ctx, base);
    return;
  }
  const uint32_t body = ctx.r3.u32;
  ctx.r3.u64 = body + 224;
  sub_82291258(ctx, base);
  ctx.r3.u64 = body;
  sub_820B6D10(ctx, base);
}

// RigidBody::SetOrientation: r4's 16 bytes to +80, normalized
// (sub_82291950, r4 as given), then UpdateOrientation and UpdateOrigin.
extern "C" REX_FUNC(sub_820B7088) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B7088(ctx, base);
    return;
  }
  const uint32_t body = ctx.r3.u32, from = ctx.r4.u32;
  for (uint32_t i = 0; i < 16; i += 4) {
    StoreU32(base, body + 80 + i, LoadU32(base, from + i));
  }
  ctx.r3.u64 = body + 80;
  ctx.r4.u64 = from;
  sub_82291950(ctx, base);
  ctx.r3.u64 = body;
  sub_820B6DD8(ctx, base);
  ctx.r3.u64 = body;
  sub_820B6D10(ctx, base);
}

// RigidBody::SetVelocity: r4 scaled by +4 (sub_82291E70 into the
// original's 112-byte frame at +80), copied to +176, then UpdateVelocity.
extern "C" REX_FUNC(sub_820B70D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B70D8(ctx, base);
    return;
  }
  const uint32_t body = ctx.r3.u32, velocity = ctx.r4.u32;
  guest::StackFrame frame(ctx, 112);
  ctx.r3.u64 = frame.Local(80);
  ctx.r4.u64 = velocity;
  ctx.f1.f64 = LoadFloat(base, body + 4);
  sub_82291E70(ctx, base);
  guest_memory::CopyRun(base, body + 176, ctx.r3.u32, {0, 16});
  ctx.r3.u64 = body;
  sub_820B6F68(ctx, base);
}

// RigidBody::SetInertia: r4's three floats to +352 and their inverses to
// +368, then UpdateOrientation (a tail call).
extern "C" REX_FUNC(sub_820B7120) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B7120(ctx, base);
    return;
  }
  const uint32_t body = ctx.r3.u32, inertia = ctx.r4.u32;
  for (uint32_t i = 0; i < 12; i += 4) {
    StoreU32(base, body + 352 + i, LoadU32(base, inertia + i));
  }
  const float one = LoadFloat(base, kOne);
  const float x = one / LoadFloat(base, body + 352);
  const float y = one / LoadFloat(base, body + 356);
  const float z = one / LoadFloat(base, body + 360);
  StoreFloat(base, body + 368, x);
  StoreFloat(base, body + 372, y);
  StoreFloat(base, body + 376, z);
  sub_820B6DD8(ctx, base);
}
