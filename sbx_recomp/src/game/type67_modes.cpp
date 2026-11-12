// The front end's type 6 and 7 screens and the garage stage as our own
// C++ (--native_type67_modes, on by default): the garage's pose and
// camera set-ups, the dialogs' movies opened, the controller options'
// buffers freed, and the screens' state steps.

#include "game/game_modes.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_type67_modes, true, "SBX",
                    "Our own C++ for the type 6 and 7 screens' and garage stage's small steps "
                    "instead of the recompiled functions");

namespace {

using game::StateMachine;
using game::Vector;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kMemRelease = 0x8000;

// The garage stage's pose: a value and three points.
struct GaragePose {
  f32_be unknown_0;
  uint8_t unknown_4[12];
  Vector points[3];
  u32_be unknown_64;
};
GUEST_OFFSET(GaragePose, points, 16);

struct CameraPreset {
  uint8_t unknown_0[4];
  f32_be unknown_4[3];
  Vector points[3];
};

struct GarageCamera {
  u32_be unknown_0;
  uint8_t unknown_4[288];
  u32_be presets;  // The camera presets' table.
  uint8_t unknown_296[4096];
  u32_be unknown_4392;
};
GUEST_OFFSET(GarageCamera, presets, 292);
GUEST_OFFSET(GarageCamera, unknown_4392, 4392);
constexpr uint32_t kGarageCameraPresets = 0x82609388;

// The controller options' two buffers.
constexpr uint32_t kControllerOptionBuffers = 0x826C7F34;

// The dialogs' four movie slots and their movies.
constexpr uint32_t kDialogMovieSlots = 16;
constexpr uint32_t kDialogMovies = 0x581E;

// The screens' states.
constexpr uint32_t kControllerOptionsOpenState = 4;
constexpr uint32_t kScreenWaitState = 9;

// A screen: its state machine, then its own fields.
struct Type6Screen {
  StateMachine machine;
  uint8_t unknown_9608[32];
  u32_be buffers[3];  // Freed when the screen's left.
  uint8_t unknown_9652[104];
  u32_be dialog_shown;
};
GUEST_OFFSET(Type6Screen, buffers, 9640);
GUEST_OFFSET(Type6Screen, dialog_shown, 9756);

struct Type7Screen {
  StateMachine machine;
  uint8_t unknown_9608[36];
  u32_be buffers[3];
  uint8_t unknown_9656[104];
  u32_be dialog_shown;
};
GUEST_OFFSET(Type7Screen, buffers, 9644);
GUEST_OFFSET(Type7Screen, dialog_shown, 9760);

struct PanelScreen {
  uint8_t unknown_0[11672];
  u32_be unknown_11672;
  uint8_t unknown_11676[4];
  u32_be unknown_11680;
};

bool On() { return REXCVAR_GET(native_type67_modes); }

void FreeBuffer(Guest& g, u32_be& buffer) {
  g.Call(sub_82292D70, buffer, 0u, kMemRelease);  // VirtualFree
}

void SetPoint(Vector& v, float y) {
  v = Vector{0.0f, y, 0.0f, 1.0f};
}

}  // namespace

#define SBX_TYPE67_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();              \
  if (!On()) {                      \
    __imp__##name(ctx, base);       \
    return;                         \
  }                                 \
  Guest g(ctx, base);

// GarageStage::ResetPose: r3 the pose. Returns it.
extern "C" REX_FUNC(sub_82178328) {
  SBX_TYPE67_FALLBACK(sub_82178328)
  GaragePose* pose = At<GaragePose>(ctx.r3.u32);
  pose->unknown_0 = 0.0f;
  SetPoint(pose->points[0], 0.0f);
  SetPoint(pose->points[1], 0.0f);
  SetPoint(pose->points[2], -0.704335f);
  pose->unknown_64 = 0;
}

// CameraPreset::CameraPreset: r3 the preset. Returns it.
extern "C" REX_FUNC(sub_82179E58) {
  SBX_TYPE67_FALLBACK(sub_82179E58)
  CameraPreset* preset = At<CameraPreset>(ctx.r3.u32);
  for (f32_be& v : preset->unknown_4) {
    v = 0.0f;
  }
  for (Vector& point : preset->points) {
    SetPoint(point, 0.0f);
  }
}

// GarageCamera::Init: r3 the camera.
extern "C" REX_FUNC(sub_8217E808) {
  SBX_TYPE67_FALLBACK(sub_8217E808)
  GarageCamera* camera = At<GarageCamera>(ctx.r3.u32);
  g.Call(sub_8217B770, camera);
  camera->unknown_4392 = 0;
  camera->presets = kGarageCameraPresets;
  camera->unknown_0 = 0;
  g.Call(sub_8217E1C0, camera);
}

// Dialog::OpenMovies: the dialog's four UI movies opened.
extern "C" REX_FUNC(sub_82184340) {
  SBX_TYPE67_FALLBACK(sub_82184340)
  for (uint32_t i = 0; i < 4; i++) {
    g.Call(sub_8211E9C8, kDialogMovieSlots + i, kDialogMovies + i);  // UiMovies::Open
  }
}

// ControllerOptions::OnLeft: its two buffers freed.
extern "C" REX_FUNC(sub_821B9708) {
  SBX_TYPE67_FALLBACK(sub_821B9708)
  u32_be* buffers = At<u32_be>(kControllerOptionBuffers);
  FreeBuffer(g, buffers[1]);
  FreeBuffer(g, buffers[0]);
  buffers[1] = 0;
  buffers[0] = 0;
}

// ControllerOptions::Open and ScreenMachine::Wait: r3 the machine, put in
// its state. Return r3.
extern "C" REX_FUNC(sub_821BABA8) {
  SBX_TYPE67_FALLBACK(sub_821BABA8)
  At<StateMachine>(ctx.r3.u32)->state = kControllerOptionsOpenState;
}

extern "C" REX_FUNC(sub_82274630) {
  SBX_TYPE67_FALLBACK(sub_82274630)
  At<StateMachine>(ctx.r3.u32)->state = kScreenWaitState;
}

// Type6Dialog::Shown and Type7Dialog::Shown: r3 the screen. Return r3.
extern "C" REX_FUNC(sub_8222FA58) {
  SBX_TYPE67_FALLBACK(sub_8222FA58)
  At<Type6Screen>(ctx.r3.u32)->dialog_shown = 0;
}

extern "C" REX_FUNC(sub_8223F558) {
  SBX_TYPE67_FALLBACK(sub_8223F558)
  At<Type7Screen>(ctx.r3.u32)->dialog_shown = 0;
}

// Type6Screen0::OnLeft and Type7Screen0::OnLeft: r3 the screen; its
// buffers freed.
extern "C" REX_FUNC(sub_82231C80) {
  SBX_TYPE67_FALLBACK(sub_82231C80)
  Type6Screen* screen = At<Type6Screen>(ctx.r3.u32);
  for (u32_be& buffer : screen->buffers) {
    FreeBuffer(g, buffer);
  }
  for (u32_be& buffer : screen->buffers) {
    buffer = 0;
  }
}

extern "C" REX_FUNC(sub_82241948) {
  SBX_TYPE67_FALLBACK(sub_82241948)
  Type7Screen* screen = At<Type7Screen>(ctx.r3.u32);
  for (u32_be& buffer : screen->buffers) {
    FreeBuffer(g, buffer);
  }
  for (u32_be& buffer : screen->buffers) {
    buffer = 0;
  }
}

// PanelScreen::InitWords: r3 the screen. Returns it.
extern "C" REX_FUNC(sub_8223DDA8) {
  SBX_TYPE67_FALLBACK(sub_8223DDA8)
  PanelScreen* screen = At<PanelScreen>(ctx.r3.u32);
  screen->unknown_11672 = 4;
  screen->unknown_11680 = 9;
}
