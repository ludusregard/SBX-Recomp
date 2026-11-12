// The camera modes as our own C++ (--native_camera_modes, on by default):
// the camera animations' clear and a rival logo's constructor.

#include "game/race_hud.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_camera_modes, true, "SBX",
                    "Our own C++ for the camera animations' clear and the rival logo's "
                    "constructor instead of the recompiled functions");

namespace {

// The camera animation playing, if any.
constexpr uint32_t kCameraAnimation = 0x826C7C78;

bool On() { return REXCVAR_GET(native_camera_modes); }

}  // namespace

// CameraAnims::Clear: no camera animation. Returns r3.
extern "C" REX_FUNC(sub_8214E6E8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8214E6E8(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  *guest::At<guest::u32_be>(kCameraAnimation) = 0;
}

// RivalLogo::RivalLogo: r3 the logo.
extern "C" REX_FUNC(sub_8220F5D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8220F5D0(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  game::RivalLogo* logo = guest::At<game::RivalLogo>(ctx.r3.u32);
  logo->movie_key = 0;
  logo->unknown_0 = -1;
  logo->flags = 2;
  logo->unknown_12 = 0;
  logo->unknown_16 = 0;
  g.Call(sub_8220F490, logo->unknown_20);
}
