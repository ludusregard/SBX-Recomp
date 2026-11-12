// Frame pacing for --subtick: game logic keeps its fixed 30 Hz tick, and in
// races extra frames are drawn between ticks.
//
// Each pass, the game posts an Update and a Draw message (game_messages.h),
// then waits for the render thread in Renderer::WaitForFrame, swaps buffers
// and has the render thread draw the new recording. After that wait, the
// pacer decides the coming pass: when the next 30 Hz tick is due it runs
// normally (a logic pass); otherwise it waits for the next frame slot
// (--max_fps) and the coming pass is render-only: Update messages are
// skipped, so it draws without advancing the game. Everything else in the
// pass runs as the game has it. The menus draw frames between their ticks
// too, once a mode change has settled and while no menu waits to close
// (game_messages.h).
//
// The present's 2-vblank wait is dropped (the pacer holds the rate instead).

#include "debug/frame_state_diff.h"
#include "guest.h"
#include "hooks/title_screen.h"
#include "subtick/car_transforms.h"
#include "subtick/frame_pacer.h"
#include "subtick/game_messages.h"
#include "subtick/input.h"

#ifdef SBX_VULKAN_RENDERER
#include "native/renderer.h"
#endif

#include <rex/cvar.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(subtick, true, "SBX",
                    "Draw frames between the game's 30 Hz ticks (--max_fps), in races and the "
                    "menus, with the cars' controls fed input sampled each frame "
                    "(--subtick_input); game logic stays at 30 Hz");

namespace {

// The video output object passed to the vblank wait: its graphics device.
constexpr uint32_t kVideoOutputDeviceOffset = 140;

#ifdef SBX_VULKAN_RENDERER
// Camera pass 0's view-projection (docs/functions.md, "Known data": the
// camera pass array at 0x826AA8E0, view-projection at +0x90).
constexpr uint32_t kMainCameraViewProjection = 0x826AA8E0 + 0x90;

// The camera the list just recorded was drawn with, for the native
// renderer's own passes (--gbuffer). In a race only (the pass array holds the
// race's camera then); with --subtick it's this frame's blended camera
// (CarTransforms_RebuildCamera), still in place until CarTransforms_EndFrame.
native::ListCamera CaptureListCamera(uint8_t* base) {
  native::ListCamera camera;
  if (!subtick::GameMessages_InRace()) {
    return camera;
  }
  for (uint32_t i = 0; i < 16; i++) {
    uint32_t word = guest::LoadU32(base, kMainCameraViewProjection + 4 * i);
    std::memcpy(&camera.view_projection[i], &word, 4);
  }
  camera.valid = true;
  return camera;
}
#endif

// Main thread, once per main-loop pass, after the game's own wait for the
// frame in flight: picks the coming pass.
void PaceNextPass(PPCContext& ctx, uint8_t* base) {
  subtick::CarTransforms_EndFrame(base);  // the last frame's camera blend
  subtick::FramePacer& pacer = subtick::GetFramePacer();
  bool finished_logic_pass = !subtick::GameMessages_IsRenderOnlyPass();
  subtick::GameMessages_OnPassFinished(base);
  if (finished_logic_pass) {
    subtick::Input_OnLogicPassFinished();
  }
  bool render_only;
  if (subtick::GameMessages_InRace()) {
    render_only = pacer.WaitForNextFrame();  // false once the next tick is due
  } else if (hooks::TitleScreen_SkipTickWait()) {
    // --skip_cutscenes is running the boot fast (hooks/title_screen.h).
    render_only = false;
  } else if (subtick::GameMessages_MenusSettled()) {
    render_only = pacer.WaitForNextFrame();
  } else {
    // Menus: the game's 30 Hz.
    pacer.WaitForNextTick();
    render_only = false;
  }
  if (!render_only) {
    pacer.OnTickReleased();
  }
  pacer.OnFrameStarted();
  subtick::GameMessages_SetRenderOnlyPass(render_only);
  debug::FrameStateDiff_OnPassStart(base, render_only, subtick::GameMessages_InRace());
  if (render_only) {
    subtick::Input_OnRenderOnlyPass(ctx, base, pacer.TickProgress());
  }
}

}  // namespace

// Renderer::WaitForFrame: r3 = renderer. A tick handler on the main thread.
// With the native renderer, the pass's command list is handed over first
// (whether a logic pass recorded it).
extern "C" REX_FUNC(sub_82092E60) {
  REX_FUNC_PROLOGUE();
#ifdef SBX_VULKAN_RENDERER
  native::Renderer_OnListHandedOver(!subtick::GameMessages_IsRenderOnlyPass(),
                                    CaptureListCamera(base));
#endif
  __imp__sub_82092E60(ctx, base);
  if (!REXCVAR_GET(subtick)) {
#ifdef SBX_VULKAN_RENDERER
    subtick::GameMessages_OnPassFinished(base);  // tells when a race ends
#endif
    // Turned off on a render-only pass (from F4, mid-race): every pass is a
    // logic pass from here. Left set, the Update message stayed skipped and
    // the game stood still. The camera blend of the last frame is put back.
    if (subtick::GameMessages_IsRenderOnlyPass()) {
      subtick::CarTransforms_EndFrame(base);
      subtick::GameMessages_SetRenderOnlyPass(false);
    }
    return;
  }
  uint32_t r1 = ctx.r1.u32;
  PaceNextPass(ctx, base);
  ctx.r1.u32 = r1;
}

// Renderer::DrawFrame (render thread): r3 = renderer. With the native
// renderer, each frame's replay is framed for it (Renderer_OnReplayStart,
// Renderer_EndFrame), as ITC-Recomp's render thread loop does.
#ifdef SBX_VULKAN_RENDERER
extern "C" REX_FUNC(sub_82091FB0) {
  REX_FUNC_PROLOGUE();
  native::Renderer_OnReplayStart();
  __imp__sub_82091FB0(ctx, base);
  native::Renderer_EndFrame();
}
#endif

// VideoOutput::WaitForVblank: r3 = the video output. The native renderer gets
// no vblanks, so this keeps the graphics device call that precedes the wait
// and drops the wait itself, which held each frame for the game's 2-vblank
// interval.
extern "C" REX_FUNC(sub_82573638) {
  REX_FUNC_PROLOGUE();
  bool skip_wait = hooks::TitleScreen_SkipTickWait();
  ctx.r3.u64 = guest::LoadU32(base, ctx.r3.u32 + kVideoOutputDeviceOffset);
  sub_8229DD58(ctx, base);
  // Without --subtick the pacer isn't running (PaceNextPass), so hold the
  // game's 30 Hz here instead of the 2-vblank wait. Without it every pass ran
  // at once, as fast as the CPU allowed.
  if (!REXCVAR_GET(subtick) && !skip_wait) {
    subtick::FramePacer& pacer = subtick::GetFramePacer();
    pacer.WaitForNextTick();
    pacer.OnTickReleased();
  }
}
