// The cameras as our own C++ (--native_cameras, on by default): the camera
// passes' set-up, a pass's enable and the viewport's update.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_cameras, true, "SBX",
                    "Our own C++ for the camera passes' set-up, a pass's enable and the "
                    "viewport's update instead of the recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u32_be;

// The 16 words kept just before the camera passes (0x826AA8E0), all but
// the second cleared by CameraPasses::Init.
constexpr uint32_t kCameraPassWords = 0x826AA8A0;

// A camera pass (592 bytes): the game's views, pass 0 the main camera.
struct CameraPass {
  uint8_t unknown_0[588];
  u32_be enabled;
};
GUEST_SIZE(CameraPass, 592);

bool On() { return REXCVAR_GET(native_cameras); }

}  // namespace

// CameraPasses::Init: its words cleared, then the passes set up
// (recompiled).
extern "C" REX_FUNC(sub_8209FAB0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8209FAB0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  u32_be* words = At<u32_be>(kCameraPassWords);
  for (int i = 0; i < 16; i++) {
    if (i != 1) {
      words[i] = 0;
    }
  }
  sub_8209FA58(ctx, base);
}

// CameraPass::Enable: r3 the pass. Returns it.
extern "C" REX_FUNC(sub_820A0B50) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A0B50(ctx, base);
    return;
  }
  Guest g(ctx, base);
  At<CameraPass>(ctx.r3.u32)->enabled = 1;
}

// Viewport::Update: r3 the viewport (recompiled update).
extern "C" REX_FUNC(sub_820AADF8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820AADF8(ctx, base);
    return;
  }
  sub_820AA508(ctx, base);
}
