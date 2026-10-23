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
// pass runs as the game has it. Menus stay at the game's 30 fps.
//
// The present's 2-vblank wait is dropped (the pacer holds the rate instead).

#include "guest.h"
#include "subtick/car_transforms.h"
#include "subtick/frame_pacer.h"
#include "subtick/game_messages.h"
#include "subtick/input.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(subtick, false, "SBX",
                    "Experimental: in races, draw frames between the game's 30 Hz ticks "
                    "(--max_fps), with the cars' controls fed input sampled each frame "
                    "(--subtick_input); game logic stays at 30 Hz");

namespace {

// The video output object passed to the vblank wait: its graphics device.
constexpr uint32_t kVideoOutputDeviceOffset = 140;

// Main thread, once per main-loop pass, after the game's own wait for the
// frame in flight: picks the coming pass.
void PaceNextPass(PPCContext& ctx, uint8_t* base) {
  subtick::CarTransforms_EndFrame(base);  // the last frame's camera blend
  subtick::FramePacer& pacer = subtick::GetFramePacer();
  bool finished_logic_pass = !subtick::GameMessages_IsRenderOnlyPass();
  subtick::GameMessages_OnPassFinished();
  if (finished_logic_pass) {
    subtick::Input_OnLogicPassFinished();
  }
  bool render_only;
  if (subtick::GameMessages_InRace()) {
    render_only = pacer.WaitForNextFrame();  // false once the next tick is due
  } else {
    pacer.WaitForNextTick();
    render_only = false;
  }
  if (!render_only) {
    pacer.OnTickReleased();
  }
  pacer.OnFrameStarted();
  subtick::GameMessages_SetRenderOnlyPass(render_only);
  if (render_only) {
    subtick::Input_OnRenderOnlyPass(ctx, base, pacer.TickProgress());
  }
}

}  // namespace

// Renderer::WaitForFrame: r3 = renderer. A tick handler on the main thread.
extern "C" REX_FUNC(sub_82092E60) {
  REX_FUNC_PROLOGUE();
  __imp__sub_82092E60(ctx, base);
  if (!REXCVAR_GET(subtick)) {
    return;
  }
  uint32_t r1 = ctx.r1.u32;
  PaceNextPass(ctx, base);
  ctx.r1.u32 = r1;
}

// VideoOutput::WaitForVblank: r3 = the video output. With --subtick, keeps
// the graphics device call that precedes the wait and drops the wait itself,
// which held each frame for the game's 2-vblank interval.
extern "C" REX_FUNC(sub_82573638) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(subtick)) {
    __imp__sub_82573638(ctx, base);
    return;
  }
  ctx.r3.u64 = guest::LoadU32(base, ctx.r3.u32 + kVideoOutputDeviceOffset);
  sub_8229DD58(ctx, base);
}
