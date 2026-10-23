// The cars' world matrices and the main camera between ticks (--subtick).
//
// Cars. A car's world matrix (car+16, 4x4 row-major: three rotation rows, then
// the position row) is set once a tick; Car::DrawUpdate (sub_820DA930) draws
// the car's parts with it. Car::DrawUpdate runs once per view in the race
// draw, so each car is recorded once per game tick (by the pacer's tick
// count). Every pass in a race, before Race::DrawStep, the matrix of every car
// recorded this tick is blended between its last two poses by the frame's
// progress through the tick, and put back after the step. On a logic pass,
// cars whose matrix already holds the new pose are recorded before the blend;
// the rest are recorded by Car::DrawUpdate, which on a logic pass is where the
// new pose first shows up (the game sets it during the draw), so
// Car::DrawUpdate also draws from the blend. A matrix is put back only if it
// still holds the blend. A pose is blended only across consecutive ticks, and
// not across a move of more than 8 m (a car recycled elsewhere). The blend
// lerps the position and each rotation row, keeping each row's length, so a
// car's scale is kept.
//
// Camera. The main camera is camera pass 0 (array at 0x826AA8E0, 592 bytes a
// pass). CameraPass::Update (sub_820A3B98) builds its view (+16) and
// view-projection (+144 = view times projection +80) from the eye (+400),
// target (+416), up (+432) and roll (+448), several times a Draw. Each call on
// pass 0 in a race is matched by its order within the pass: the logic pass
// records what each call is given, and on every pass a call given exactly the
// tick's values builds the view from the blend instead. Calls given anything
// else are left alone. More than 8 m between ticks is a cut, not blended.
//
// The blended eye, target, up and roll stay in the pass until the next pass
// starts (CarTransforms_EndFrame puts the tick's values back before any game
// logic runs): the draw reads them besides the view, for the camera position
// the cars' reflections are worked out against (0x8264EA40, set per view by
// sub_820C5BA0 from the eye) and for which way the light glows' billboards
// face (sub_8209C800).
//
// The cars are drawn with whatever +144 holds when they draw: Race::DrawStep
// draws the cars (sub_820C5BA0) before its own camera updates (sub_820F5F48).
// On a logic pass the Update message has already rebuilt pass 0 for the
// frame, but on a render-only pass the Update is skipped, so
// CarTransforms_RebuildCamera makes that call before Race::DrawStep.
//
// Wheel spin and steering still change once a tick.

#include "subtick/car_transforms.h"

#include "subtick/frame_pacer.h"
#include "subtick/game_messages.h"

#include <rex/cvar.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <vector>

REXCVAR_DECLARE(bool, subtick);

namespace subtick {

namespace {

constexpr uint32_t kWorldMatrix = 16;  // car+16
// A move between ticks above this is a cut (not blended).
constexpr float kCut = 8.0f;

constexpr uint32_t kMainCamera = 0x826AA8E0;  // camera pass 0
constexpr uint32_t kCameraState = 400;        // eye, target, up, roll
constexpr int kCameraFloats = 13;             // eye xyzw, target xyzw, up xyzw, roll
constexpr int kMaxCameraCalls = 16;           // CameraPass::Update calls on pass 0 a pass

struct Pose {
  float prev[16] = {};
  float cur[16] = {};
  uint64_t tick = ~0ull;  // the game tick that last recorded it
};

std::map<uint32_t, Pose> g_poses;  // by car address

struct CameraPose {
  float prev[kCameraFloats] = {};
  float cur[kCameraFloats] = {};
  uint64_t tick = ~0ull;
};

std::vector<CameraPose> g_camera;  // by the call's order within the pass
uint64_t g_camera_pass = ~0ull;    // the pass the calls are being counted in
int g_camera_call = 0;

// The blend written into pass 0 this frame, kept there until the next pass
// starts, and the tick's values it replaced.
bool g_camera_held = false;
float g_camera_raw[kCameraFloats];
float g_camera_blend[kCameraFloats];

void ReadFloats(uint8_t* base, uint32_t address, float* out, int n) {
  for (int i = 0; i < n; i++) {
    out[i] = guest::LoadFloat(base, address + 4 * i);
  }
}

void WriteFloats(uint8_t* base, uint32_t address, const float* in, int n) {
  for (int i = 0; i < n; i++) {
    guest::StoreFloat(base, address + 4 * i, in[i]);
  }
}

float RowLength(const float* row) {
  return std::sqrt(row[0] * row[0] + row[1] * row[1] + row[2] * row[2]);
}

float Distance3(const float* a, const float* b) {
  float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Gives a lerped row back the length the two rows lerp to.
void KeepLength(const float* a, const float* b, float t, float* out) {
  float length = RowLength(a) + (RowLength(b) - RowLength(a)) * t;
  float current = RowLength(out);
  if (current > 0 && length > 0) {
    for (int j = 0; j < 3; j++) {
      out[j] *= length / current;
    }
  }
}

void Blend(const float* a, const float* b, float t, float* out) {
  for (int i = 0; i < 16; i++) {
    out[i] = a[i] + (b[i] - a[i]) * t;
  }
  for (int row = 0; row < 3; row++) {
    KeepLength(a + 4 * row, b + 4 * row, t, out + 4 * row);
  }
}

bool CarsEnabled() {
  return REXCVAR_GET(subtick);
}

bool CameraEnabled() {
  return REXCVAR_GET(subtick) && GameMessages_InRace();
}

// The frame's progress through the tick, fixed for the whole pass.
float Progress() {
  return std::clamp(GetFramePacer().FrameProgress(), 0.0f, 1.0f);
}

// Takes `now` as the car's pose for `tick`.
void TakePose(Pose& pose, bool fresh, uint64_t tick, const float* now) {
  bool consecutive = !fresh && pose.tick + 1 == tick;
  bool cut = consecutive && Distance3(now + 12, pose.cur + 12) > kCut;
  if (consecutive && !cut) {
    std::copy(pose.cur, pose.cur + 16, pose.prev);
  } else {
    std::copy(now, now + 16, pose.prev);  // no blending across a gap or a cut
  }
  std::copy(now, now + 16, pose.cur);
  pose.tick = tick;
}

// Before a CameraPass::Update call on pass 0: records the tick's values on a
// logic pass, and writes the blend if the call is given the tick's values
// (or the blend already written this frame, which stands for them).
void CameraBegin(uint8_t* base) {
  uint64_t pass = GameMessages_PassNumber();
  if (pass != g_camera_pass) {
    g_camera_pass = pass;
    g_camera_call = 0;
  }
  int call = g_camera_call++;
  if (call >= kMaxCameraCalls) {
    return;
  }
  if (int(g_camera.size()) <= call) {
    g_camera.resize(call + 1);
  }
  CameraPose& pose = g_camera[call];
  uint64_t tick = GetFramePacer().TicksReleased();
  float now[kCameraFloats];
  ReadFloats(base, kMainCamera + kCameraState, now, kCameraFloats);
  if (g_camera_held && std::memcmp(now, g_camera_blend, sizeof(now)) == 0) {
    std::copy(g_camera_raw, g_camera_raw + kCameraFloats, now);
  }
  if (!GameMessages_IsRenderOnlyPass() && pose.tick != tick) {
    bool consecutive = pose.tick + 1 == tick;
    bool cut = consecutive &&
               (Distance3(now, pose.cur) > kCut || Distance3(now + 4, pose.cur + 4) > kCut);
    if (consecutive && !cut) {
      std::copy(pose.cur, pose.cur + kCameraFloats, pose.prev);
    } else {
      std::copy(now, now + kCameraFloats, pose.prev);
    }
    std::copy(now, now + kCameraFloats, pose.cur);
    pose.tick = tick;
  }
  if (pose.tick != tick || std::memcmp(now, pose.cur, sizeof(now)) != 0) {
    return;  // not the tick's values: leave this call alone
  }
  float t = Progress();
  float out[kCameraFloats];
  for (int i = 0; i < kCameraFloats; i++) {
    out[i] = pose.prev[i] + (pose.cur[i] - pose.prev[i]) * t;
  }
  KeepLength(pose.prev + 8, pose.cur + 8, t, out + 8);  // the up vector
  WriteFloats(base, kMainCamera + kCameraState, out, kCameraFloats);
  std::copy(now, now + kCameraFloats, g_camera_raw);
  std::copy(out, out + kCameraFloats, g_camera_blend);
  g_camera_held = true;
}

}  // namespace

void CarTransforms_Record(uint8_t* base, uint32_t car) {
  if (!CarsEnabled() || GameMessages_IsRenderOnlyPass()) {
    return;
  }
  uint64_t tick = GetFramePacer().TicksReleased();
  auto [it, fresh] = g_poses.try_emplace(car);
  Pose& pose = it->second;
  if (pose.tick == tick) {
    return;  // already recorded this tick (another view's draw, or before the blend)
  }
  float now[16];
  ReadFloats(base, car + kWorldMatrix, now, 16);
  TakePose(pose, fresh, tick, now);
}

std::vector<CarTransformRestore> CarTransforms_BlendAll(uint8_t* base) {
  std::vector<CarTransformRestore> restores;
  if (!CarsEnabled()) {
    return restores;
  }
  uint64_t tick = GetFramePacer().TicksReleased();
  if (!GameMessages_IsRenderOnlyPass()) {
    // Cars seen last tick whose matrix has already moved on hold the new
    // pose: record it now, so this frame is blended too.
    for (auto& [car, pose] : g_poses) {
      if (pose.tick + 1 != tick) {
        continue;
      }
      float now[16];
      ReadFloats(base, car + kWorldMatrix, now, 16);
      if (std::memcmp(now, pose.cur, sizeof(now)) != 0) {
        TakePose(pose, false, tick, now);
      }
    }
  }
  float t = Progress();
  restores.reserve(g_poses.size());
  for (const auto& [car, pose] : g_poses) {
    if (pose.tick != tick) {
      continue;  // not recorded this tick: stale, or not yet moved on
    }
    CarTransformRestore restore;
    restore.address = car + kWorldMatrix;
    ReadFloats(base, restore.address, restore.saved, 16);
    Blend(pose.prev, pose.cur, t, restore.written);
    WriteFloats(base, restore.address, restore.written, 16);
    restores.push_back(restore);
  }
  return restores;
}

CarTransformRestore CarTransforms_DrawBegin(uint8_t* base, uint32_t car) {
  CarTransformRestore restore;
  if (!CarsEnabled()) {
    return restore;
  }
  auto it = g_poses.find(car);
  if (it == g_poses.end() || it->second.tick != GetFramePacer().TicksReleased()) {
    return restore;
  }
  const Pose& pose = it->second;
  float blended[16];
  Blend(pose.prev, pose.cur, Progress(), blended);
  float now[16];
  ReadFloats(base, car + kWorldMatrix, now, 16);
  if (std::memcmp(now, blended, sizeof(now)) == 0) {
    return restore;  // already blended before Race::DrawStep
  }
  restore.address = car + kWorldMatrix;
  std::copy(now, now + 16, restore.saved);
  std::copy(blended, blended + 16, restore.written);
  WriteFloats(base, restore.address, blended, 16);
  return restore;
}

void CarTransforms_DrawEnd(uint8_t* base, const CarTransformRestore& restore) {
  if (restore.address == 0) {
    return;
  }
  float now[16];
  ReadFloats(base, restore.address, now, 16);
  if (std::memcmp(now, restore.written, sizeof(now)) == 0) {
    WriteFloats(base, restore.address, restore.saved, 16);
  }
}

void CarTransforms_RestoreAll(uint8_t* base, const std::vector<CarTransformRestore>& restores) {
  for (const CarTransformRestore& restore : restores) {
    float now[16];
    ReadFloats(base, restore.address, now, 16);
    if (std::memcmp(now, restore.written, sizeof(now)) == 0) {
      WriteFloats(base, restore.address, restore.saved, 16);
    }  // otherwise the game wrote a new pose during the draw: keep it
  }
}

void CarTransforms_EndFrame(uint8_t* base) {
  if (!g_camera_held) {
    return;
  }
  g_camera_held = false;
  float now[kCameraFloats];
  ReadFloats(base, kMainCamera + kCameraState, now, kCameraFloats);
  if (std::memcmp(now, g_camera_blend, sizeof(now)) == 0) {
    WriteFloats(base, kMainCamera + kCameraState, g_camera_raw, kCameraFloats);
  }
}

void CarTransforms_RebuildCamera(PPCContext& ctx, uint8_t* base) {
  if (!CameraEnabled() || !GameMessages_IsRenderOnlyPass()) {
    return;
  }
  uint64_t r3 = ctx.r3.u64;
  uint64_t lr = ctx.lr;
  ctx.r3.u64 = kMainCamera;
  sub_820A3B98(ctx, base);  // through the wrapper below: built from the blend
  ctx.r3.u64 = r3;
  ctx.lr = lr;
}

bool CarTransforms_CameraEnabled() {
  return CameraEnabled();
}

void CarTransforms_CameraBegin(uint8_t* base) {
  CameraBegin(base);
}

}  // namespace subtick

// CameraPass::Update: r3 = the camera pass. Builds its view from its eye,
// target, up and roll. On pass 0 in a race, built from the blend between
// ticks (above); the blend stays in the pass for the rest of the frame.
extern "C" REX_FUNC(sub_820A3B98) {
  REX_FUNC_PROLOGUE();
  if (ctx.r3.u32 != 0x826AA8E0 || !subtick::CarTransforms_CameraEnabled()) {
    __imp__sub_820A3B98(ctx, base);
    return;
  }
  subtick::CarTransforms_CameraBegin(base);
  __imp__sub_820A3B98(ctx, base);
}
