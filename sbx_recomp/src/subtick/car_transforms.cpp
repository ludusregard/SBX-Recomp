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
// Cameras. The game's views are camera passes (an array at 0x826AA8E0, 592
// bytes a pass; pass 0 is the main camera, others the rear-view mirror and
// the like). CameraPass::Update (sub_820A3B98) builds a pass's view (+16) and
// view-projection (+144 = view times projection +80) from its eye (+400),
// target (+416), up (+432) and roll (+448), several times a Draw. Every pass
// is blended the same way, keyed by its address: each call on a pass in a
// race is matched by its order within the main loop pass, the logic pass
// records what each call is given, and on every pass a call given exactly the
// tick's values builds the view from the blend instead. Calls given anything
// else are left alone. More than 8 m between ticks is a cut, not blended.
// (Only pass 0 was blended at first: the mirror then drew the blended cars
// from the tick's camera, and they juddered in it as they used to on screen.)
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
// On a logic pass the Update message has already rebuilt the passes it
// updates for the frame, but on a render-only pass the Update is skipped, so
// CarTransforms_RebuildCamera makes those calls before Race::DrawStep: one
// for each pass the last logic pass updated before its draw step.
//
// Wheel spin and steering still change once a tick.

#include "subtick/car_transforms.h"

#include "subtick/frame_pacer.h"
#include "subtick/game_messages.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <vector>

REXCVAR_DECLARE(bool, subtick);

REXCVAR_DEFINE_BOOL(log_record_matrix, false, "SBX",
                    "Log the first 40 view-projections the recorder draws with (which camera the race world uses)");
REXCVAR_DEFINE_INT32(camera_fov, 0, "SBX",
                     "Vertical field of view of the main camera, in degrees (10 to 170); 0 is the game's own");

namespace subtick {

namespace {

constexpr uint32_t kWorldMatrix = 16;  // car+16
// A move between ticks above this is a cut (not blended).
constexpr float kCut = 8.0f;

constexpr uint32_t kCameraState = 400;  // eye, target, up, roll
constexpr int kCameraFloats = 13;       // eye xyzw, target xyzw, up xyzw, roll
constexpr int kMaxCameraCalls = 16;     // CameraPass::Update calls on one camera a pass

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

// One camera pass.
struct Camera {
  std::vector<CameraPose> poses;  // by the call's order within the main loop pass
  uint64_t counted_pass = ~0ull;  // the main loop pass the calls are being counted in
  int call = 0;
  // Updated by the last logic pass before its draw step (by the Update
  // message): rebuilt before the draw step on render-only passes.
  bool updated_before_draw = false;
  // The blend written into the pass this frame, kept there until the next
  // main loop pass starts, and the tick's values it replaced.
  bool held = false;
  float raw[kCameraFloats];
  float blend[kCameraFloats];
};

std::map<uint32_t, Camera> g_cameras;  // by the camera pass's address
bool g_in_draw_step = false;           // between Race::DrawStep's start and end

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

// Before a CameraPass::Update call on a camera pass: records the tick's
// values on a logic pass, and writes the blend if the call is given the
// tick's values (or the blend already written this frame, which stands for
// them).
void CameraBegin(uint8_t* base, uint32_t address) {
  Camera& camera = g_cameras[address];
  uint64_t pass = GameMessages_PassNumber();
  if (pass != camera.counted_pass) {
    camera.counted_pass = pass;
    camera.call = 0;
    if (!GameMessages_IsRenderOnlyPass()) {
      camera.updated_before_draw = false;
    }
  }
  if (!GameMessages_IsRenderOnlyPass() && !g_in_draw_step) {
    camera.updated_before_draw = true;
  }
  int call = camera.call++;
  if (call >= kMaxCameraCalls) {
    return;
  }
  if (int(camera.poses.size()) <= call) {
    camera.poses.resize(call + 1);
  }
  CameraPose& pose = camera.poses[call];
  uint64_t tick = GetFramePacer().TicksReleased();
  float now[kCameraFloats];
  ReadFloats(base, address + kCameraState, now, kCameraFloats);
  if (camera.held && std::memcmp(now, camera.blend, sizeof(now)) == 0) {
    std::copy(camera.raw, camera.raw + kCameraFloats, now);
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
  WriteFloats(base, address + kCameraState, out, kCameraFloats);
  std::copy(now, now + kCameraFloats, camera.raw);
  std::copy(out, out + kCameraFloats, camera.blend);
  camera.held = true;
}

// --camera_fov: scales the main camera's projection (pass 0; the projection is
// at +80, row-major, so P00 is at +80 and P11 at +100). It runs after the
// game's update, because the update can rebuild the projection (a virtual call
// before the matrix multiply) and so would overwrite a change made before it.
// After a change the view-projection (+144) is rebuilt as view (+16) times the
// projection, the same product the game makes. P11 is cot(fov / 2) and P00 is
// P11 / aspect, so both take the same factor and the aspect is kept. The
// game's own values are kept while the flag is off, and when the game writes
// the projection again (anything other than what this wrote is taken as the
// game's).
constexpr uint32_t kMainCamera = 0x826AA8E0;
constexpr uint32_t kProjectionP00 = 0x50;
constexpr uint32_t kProjectionP11 = 0x64;
constexpr uint32_t kProjection = 0x50;
constexpr uint32_t kView = 0x10;
constexpr uint32_t kViewProjection = 0x90;
constexpr float kPi = 3.14159265358979f;

struct MainFov {
  bool applied = false;
  float game_p00 = 0, game_p11 = 0;
  float applied_p00 = 0, applied_p11 = 0;
};
MainFov g_main_fov;

// view-projection (+144) = view (+16) times projection (+80), as the game builds it.
void RebuildViewProjection(uint8_t* base, uint32_t address) {
  float view[16];
  float projection[16];
  for (int i = 0; i < 16; i++) {
    view[i] = guest::LoadFloat(base, address + kView + 4 * i);
    projection[i] = guest::LoadFloat(base, address + kProjection + 4 * i);
  }
  float out[16];
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      out[r * 4 + c] = view[r * 4 + 0] * projection[0 * 4 + c] +
                       view[r * 4 + 1] * projection[1 * 4 + c] +
                       view[r * 4 + 2] * projection[2 * 4 + c] +
                       view[r * 4 + 3] * projection[3 * 4 + c];
    }
  }
  for (int i = 0; i < 16; i++) {
    guest::StoreFloat(base, address + kViewProjection + 4 * i, out[i]);
  }
}

void MainCameraFov(uint8_t* base, uint32_t address) {
  if (address != kMainCamera) {
    return;
  }
  float p00 = guest::LoadFloat(base, address + kProjectionP00);
  float p11 = guest::LoadFloat(base, address + kProjectionP11);
  bool was_applied = g_main_fov.applied;
  if (g_main_fov.applied && (p00 != g_main_fov.applied_p00 || p11 != g_main_fov.applied_p11)) {
    g_main_fov.applied = false;  // the game wrote the projection since
  }
  if (!g_main_fov.applied) {
    if (!(std::isfinite(p00) && std::isfinite(p11) && p00 > 0 && p11 > 0)) {
      return;
    }
    g_main_fov.game_p00 = p00;
    g_main_fov.game_p11 = p11;
  }
  int degrees = REXCVAR_GET(camera_fov);
  float new_p00 = g_main_fov.game_p00;
  float new_p11 = g_main_fov.game_p11;
  if (degrees >= 10 && degrees <= 170) {
    float target_p11 = 1.0f / std::tan(float(degrees) * kPi / 360.0f);
    float scale = target_p11 / g_main_fov.game_p11;
    new_p00 = g_main_fov.game_p00 * scale;
    new_p11 = g_main_fov.game_p11 * scale;
    g_main_fov.applied = true;
    g_main_fov.applied_p00 = new_p00;
    g_main_fov.applied_p11 = new_p11;
  } else {
    g_main_fov.applied = false;
  }
  if (new_p00 != p00 || new_p11 != p11) {
    guest::StoreFloat(base, address + kProjectionP00, new_p00);
    guest::StoreFloat(base, address + kProjectionP11, new_p11);
  }
  if (was_applied || g_main_fov.applied) {
    RebuildViewProjection(base, address);
  }
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
  for (auto& [address, camera] : g_cameras) {
    if (!camera.held) {
      continue;
    }
    camera.held = false;
    float now[kCameraFloats];
    ReadFloats(base, address + kCameraState, now, kCameraFloats);
    if (std::memcmp(now, camera.blend, sizeof(now)) == 0) {
      WriteFloats(base, address + kCameraState, camera.raw, kCameraFloats);
    }
  }
}

void CarTransforms_RebuildCamera(PPCContext& ctx, uint8_t* base) {
  if (!CameraEnabled() || !GameMessages_IsRenderOnlyPass()) {
    return;
  }
  std::vector<uint32_t> passes;
  for (const auto& [address, camera] : g_cameras) {
    if (camera.updated_before_draw) {
      passes.push_back(address);
    }
  }
  uint64_t r3 = ctx.r3.u64;
  uint64_t lr = ctx.lr;
  for (uint32_t address : passes) {
    ctx.r3.u64 = address;
    sub_820A3B98(ctx, base);  // through the wrapper below: built from the blend
  }
  ctx.r3.u64 = r3;
  ctx.lr = lr;
}

void CarTransforms_DrawStepBegin() {
  g_in_draw_step = true;
}

void CarTransforms_DrawStepEnd() {
  g_in_draw_step = false;
}

bool CarTransforms_CameraEnabled() {
  return CameraEnabled();
}

void CarTransforms_CameraBegin(uint8_t* base, uint32_t address) {
  CameraBegin(base, address);
}

void CarTransforms_CameraFov(uint8_t* base, uint32_t address) {
  MainCameraFov(base, address);
}

}  // namespace subtick

// CameraPass::Update: r3 = the camera pass. Builds its view from its eye,
// target, up and roll. In a race, built from the blend between ticks
// (above); the blend stays in the pass for the rest of the frame.
extern "C" REX_FUNC(sub_820A3B98) {
  REX_FUNC_PROLOGUE();
  const uint32_t pass = ctx.r3.u32;
  if (subtick::CarTransforms_CameraEnabled()) {
    subtick::CarTransforms_CameraBegin(base, pass);
  }
  __imp__sub_820A3B98(ctx, base);
  subtick::CarTransforms_CameraFov(base, pass);  // --camera_fov, after the game's update (any mode)
}

// Recorder::RecordObject (sub_8257ACD0): r4 = the view-projection the object is
// drawn with. --log_record_matrix logs each new address once: whether it is a
// camera pass's view-projection (and which), the caller, and the first row.
extern "C" REX_FUNC(sub_8257ACD0) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(log_record_matrix)) {
    static std::set<uint32_t> seen;
    uint32_t matrix = ctx.r4.u32;
    if (seen.size() < 40 && seen.insert(matrix).second) {
      int pass = -1;
      uint32_t offset = matrix - 0x826AA8E0u;
      if (matrix >= 0x826AA8E0u && offset < 21 * 592 && offset % 592 == 0x90) {
        pass = int(offset / 592);
      }
      REXLOG_INFO("record matrix {:08X} (camera pass {}, from {:08X}): row0 {} {} {} {}",
                  matrix, pass, uint32_t(ctx.lr), guest::LoadFloat(base, matrix),
                  guest::LoadFloat(base, matrix + 4), guest::LoadFloat(base, matrix + 8),
                  guest::LoadFloat(base, matrix + 12));
    }
  }
  __imp__sub_8257ACD0(ctx, base);
}
