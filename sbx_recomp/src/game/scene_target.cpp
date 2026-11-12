// The scene target as our own C++ (--native_scene_target, on by default):
// its bound targets recorded.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scene_target, true, "SBX",
                    "Our own C++ for the scene target's bound targets recorded instead of the "
                    "recompiled function");

namespace {

// Holds the scene targets' recorder.
constexpr uint32_t kTargetRecorder = 0x8265CD6C;

}  // namespace

// SceneTarget::RecordBoundTargets: r3 the target, r4 passed on. The
// recorder records it (recompiled).
extern "C" REX_FUNC(sub_8209B178) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_scene_target)) {
    __imp__sub_8209B178(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  g.Call(sub_8209AF68, *guest::At<guest::u32_be>(kTargetRecorder), ctx.r3.u32, ctx.r4.u32);
}
