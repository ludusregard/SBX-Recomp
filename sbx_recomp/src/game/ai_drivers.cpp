// The AI drivers as our own C++ (--native_ai_drivers, on by default): a
// car's input cleared, the drivers' resets and their paths', corners' and
// skills' set-ups, and the drivers' path points destroyed. The drivers'
// thinking stays recompiled.

#include "game/math.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_ai_drivers, true, "SBX",
                    "Our own C++ for the AI drivers' resets and set-ups instead of the "
                    "recompiled functions");

namespace {

using game::Vector;
using guest::AddressOf;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::i16_be;
using guest::i32_be;
using guest::u16_be;
using guest::u32_be;

struct CarInput {
  u32_be flags;  // Picks digital or analog handling of each.
  u16_be throttle;
  u16_be brake;
  i16_be steering;
  u16_be unknown_10;
  u16_be unknown_12;
  uint8_t unknown_14[3];
  uint8_t control_type;
  uint8_t unknown_18[2];
};
GUEST_SIZE(CarInput, 20);
GUEST_OFFSET(CarInput, flags, 0);
GUEST_OFFSET(CarInput, throttle, 4);
GUEST_OFFSET(CarInput, brake, 6);
GUEST_OFFSET(CarInput, steering, 8);
GUEST_OFFSET(CarInput, control_type, 17);

struct OtherCar {
  f32_be distance;  // 200.0 when there's none.
  f32_be unknown_4[2];
  f32_be unknown_12;  // 1.0.
  f32_be unknown_16;
  f32_be unknown_20;  // -1.0.
  u16_be unknown_24;
  i16_be unknown_26;
  u16_be unknown_28;
  uint8_t unknown_30[2];
  u16_be unknown_32[2];
};
GUEST_SIZE(OtherCar, 36);
GUEST_OFFSET(OtherCar, distance, 0);

struct PathPoint {
  Vector point;
  u16_be unknown_16[2];
  uint8_t unknown_20[44];
};
GUEST_SIZE(PathPoint, 64);
GUEST_OFFSET(PathPoint, point, 0);

struct AiCorner {
  u32_be unknown_0[2];
  u16_be unknown_8[4];
  f32_be unknown_16;  // 1.0.
  f32_be unknown_20;
  u32_be unknown_24;
  f32_be unknown_28[2];
  uint8_t unknown_36[2];
  uint8_t unknown_38[2];
  u32_be kind;
};
GUEST_SIZE(AiCorner, 44);
GUEST_OFFSET(AiCorner, kind, 40);

struct AiPath {
  u32_be driver;
  uint8_t unknown_4[12];
  PathPoint points[80];
  uint8_t unknown_5136[26];
  u16_be corner_count;
  AiCorner corners[8];
  f32_be unknown_5516;
  u32_be unknown_5520;
  i32_be unknown_5524[2];
  u32_be callback;
};
GUEST_SIZE(AiPath, 5536);
GUEST_OFFSET(AiPath, driver, 0);
GUEST_OFFSET(AiPath, points, 16);
GUEST_OFFSET(AiPath, corner_count, 5162);
GUEST_OFFSET(AiPath, corners, 5164);
GUEST_OFFSET(AiPath, callback, 5532);

struct AiDriver {
  CarInput input;
  uint8_t unknown_20[28];
  Vector unknown_48[2];
  Vector unknown_80;
  f32_be unknown_96[16];
  uint8_t unknown_160[4];
  f32_be unknown_164[2];
  OtherCar others[7];
  i32_be other_ids[7];
  u32_be unknown_452;
  u16_be unknown_456[2];
  f32_be unknown_460;
  u32_be unknown_464;
  u16_be unknown_468;
  uint8_t unknown_470[10];
  AiPath path;
  uint8_t unknown_6016[40];
  f32_be unknown_6056;
  uint8_t unknown_6060[4];
  f32_be unknown_6064[3];
  f32_be unknown_6076;
  f32_be unknown_6080[5];
  uint8_t unknown_6100;
  uint8_t unknown_6101[3];
  f32_be unknown_6104[3];
  u32_be callback;
  uint8_t unknown_6120[8];
};
GUEST_SIZE(AiDriver, 6128);
GUEST_OFFSET(AiDriver, input, 0);
GUEST_OFFSET(AiDriver, others, 172);
GUEST_OFFSET(AiDriver, other_ids, 424);
GUEST_OFFSET(AiDriver, path, 480);
GUEST_OFFSET(AiDriver, callback, 6116);

struct AiSkill {
  u32_be unknown_0;
  i16_be unknown_4[7];
  uint8_t unknown_18[2];
  f32_be unknown_20;
  u16_be unknown_24;
  uint8_t unknown_26;
  uint8_t unknown_27[1];
  u16_be unknown_28;
  uint8_t unknown_30;
  uint8_t unknown_31[1];
  u16_be unknown_32;
  uint8_t unknown_34;
  uint8_t unknown_35[1];
  u32_be unknown_36;
};
GUEST_SIZE(AiSkill, 40);


constexpr int kDrivers = 7;
constexpr uint32_t kPathPointDestructor = 0x821611F0;  // An empty one.
constexpr uint32_t kDriverCallback = 0x82138C58;
constexpr uint32_t kPathCallback = 0x82139ED8;

bool On() { return REXCVAR_GET(native_ai_drivers); }

void ClearCarInput(CarInput* input) {
  input->throttle = 0;
  input->brake = 0;
  input->unknown_10 = 0;
  input->steering = 0;
  input->flags = 0;
  input->control_type = 0;
  for (uint8_t& byte : input->unknown_14) {
    byte = 0;
  }
  input->unknown_12 = 0;
}

void ClearOthers(AiDriver* driver) {
  driver->unknown_460 = 0.0f;
  driver->unknown_456[0] = 0;
  driver->unknown_456[1] = 0;
  driver->unknown_464 = 0;
  driver->unknown_468 = 0;
  for (int i = 0; i < 7; i++) {
    OtherCar& other = driver->others[i];
    other.distance = 200.0f;
    other.unknown_4[0] = 0.0f;
    other.unknown_4[1] = 0.0f;
    other.unknown_12 = 1.0f;
    other.unknown_16 = 0.0f;
    other.unknown_20 = -1.0f;
    other.unknown_24 = 0;
    other.unknown_26 = -1;
    other.unknown_28 = 0;
    other.unknown_30[0] = 0;
    other.unknown_30[1] = 0;
    other.unknown_32[0] = 0;
    other.unknown_32[1] = 0;
    driver->other_ids[i] = -1;
  }
}

void ClearCorner(AiCorner& corner) {
  corner.unknown_0[0] = 0;
  corner.unknown_0[1] = 0;
  for (u16_be& half : corner.unknown_8) {
    half = 0;
  }
  corner.unknown_16 = 1.0f;
  corner.unknown_20 = 0.0f;
  corner.unknown_24 = 0;
  corner.unknown_28[0] = 0.0f;
  corner.unknown_28[1] = 0.0f;
  corner.unknown_36[0] = 0;
  corner.unknown_36[1] = 0;
  corner.kind = 0;
}

void ResetSkill(AiSkill* skill) {
  skill->unknown_24 = 0;
  skill->unknown_26 = 0;
  skill->unknown_28 = 0;
  skill->unknown_30 = 0;
  skill->unknown_32 = 0;
  skill->unknown_34 = 0;
  for (i16_be& half : skill->unknown_4) {
    half = 0;
  }
  skill->unknown_20 = 100.0f;
  skill->unknown_4[0] = 500;
  skill->unknown_4[1] = 80;
  skill->unknown_4[2] = -100;
}

}  // namespace

#define SBX_AI_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();          \
  if (!On()) {                  \
    __imp__##name(ctx, base);   \
    return;                     \
  }                             \
  Guest g(ctx, base);

// CarInput::Clear: r3 the input. Returns it.
extern "C" REX_FUNC(sub_820C9488) {
  SBX_AI_FALLBACK(sub_820C9488)
  ClearCarInput(At<CarInput>(ctx.r3.u32));
}

// AiDriver::Reset: r3 the driver.
extern "C" REX_FUNC(sub_82139548) {
  SBX_AI_FALLBACK(sub_82139548)
  AiDriver* driver = At<AiDriver>(ctx.r3.u32);
  g.Call(sub_82145048, driver);
  g.Call(sub_8213CF88, &driver->path, driver);  // AiPath::ResetFor
  driver->unknown_6056 = 0.0f;
  for (f32_be& v : driver->unknown_6064) {
    v = 0.0f;
  }
  driver->unknown_6076 = 1.0f;
  driver->unknown_6100 = 0;
  for (f32_be& v : driver->unknown_6080) {
    v = 0.0f;
  }
  driver->callback = kDriverCallback;
  for (f32_be& v : driver->unknown_6104) {
    v = 0.0f;
  }
  g.Call(sub_82144330, driver);
}

// AiDriver::InitPath: r3 the driver; its path's points' w set. Returns it.
extern "C" REX_FUNC(sub_821397E8) {
  SBX_AI_FALLBACK(sub_821397E8)
  AiDriver* driver = At<AiDriver>(ctx.r3.u32);
  driver->input.throttle = 0;
  driver->input.brake = 0;
  driver->unknown_48[0].w = 1.0f;
  driver->unknown_48[1].w = 1.0f;
  driver->unknown_80.w = 1.0f;
  for (PathPoint& point : driver->path.points) {
    point.point.w = 1.0f;
    point.unknown_16[0] = 0;
    point.unknown_16[1] = 0;
  }
  driver->unknown_6076 = 1.0f;
}

// AiDrivers::DestroyRings: r3 the drivers; each one's path points
// destroyed, the last driver first.
extern "C" REX_FUNC(sub_82139838) {
  SBX_AI_FALLBACK(sub_82139838)
  for (int i = kDrivers - 1; i >= 0; i--) {
    AiDriver* driver = guest::Element<AiDriver>(ctx.r3.u32, uint32_t(i));
    g.Call(sub_820900C0, driver->path.points, uint32_t(sizeof(PathPoint)), 80u,
           kPathPointDestructor);  // The vector destructor iterator.
  }
}

// AiPath::ClearCornerKind: r3 the corner. Returns it.
extern "C" REX_FUNC(sub_8213B190) {
  SBX_AI_FALLBACK(sub_8213B190)
  At<AiCorner>(ctx.r3.u32)->kind = 0;
}

// AiPath::ClearCorners: r3 the path. Returns it.
extern "C" REX_FUNC(sub_8213B1A0) {
  SBX_AI_FALLBACK(sub_8213B1A0)
  AiPath* path = At<AiPath>(ctx.r3.u32);
  path->corner_count = 0;
  for (AiCorner& corner : path->corners) {
    ClearCorner(corner);
  }
}

// AiPath::ResetFor: r3 the path, r4 its driver.
extern "C" REX_FUNC(sub_8213CF88) {
  SBX_AI_FALLBACK(sub_8213CF88)
  AiPath* path = At<AiPath>(ctx.r3.u32);
  path->driver = ctx.r4.u32;
  path->unknown_5516 = 0.0f;
  path->callback = kPathCallback;
  path->unknown_5524[0] = -1;
  path->unknown_5520 = 1;
  path->unknown_5524[1] = -1;
  g.Call(sub_8213BEB8, path);
}

// AiSkill::Reset and AiSkill::Clear: r3 the skill. Return it.
extern "C" REX_FUNC(sub_8213DFA0) {
  SBX_AI_FALLBACK(sub_8213DFA0)
  ResetSkill(At<AiSkill>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_8213E008) {
  SBX_AI_FALLBACK(sub_8213E008)
  AiSkill* skill = At<AiSkill>(ctx.r3.u32);
  ResetSkill(skill);
  skill->unknown_0 = 0;
  skill->unknown_36 = 0;
}

// AiDriver::ClearOthers: r3 the driver; the other cars it watches cleared.
// Returns it.
extern "C" REX_FUNC(sub_82144298) {
  SBX_AI_FALLBACK(sub_82144298)
  ClearOthers(At<AiDriver>(ctx.r3.u32));
}

// AiDriver::ClearCarState: r3 the driver. Returns it.
extern "C" REX_FUNC(sub_82144E18) {
  SBX_AI_FALLBACK(sub_82144E18)
  AiDriver* driver = At<AiDriver>(ctx.r3.u32);
  driver->unknown_80 = Vector{0.0f, 0.0f, 0.0f, 1.0f};
  driver->unknown_452 = 0;
  for (f32_be& v : driver->unknown_96) {
    v = 0.0f;
  }
  driver->unknown_164[0] = 500.0f;
  driver->unknown_164[1] = 500.0f;
  ClearOthers(driver);
}
