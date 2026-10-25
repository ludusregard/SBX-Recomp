// Voice chat (Xbox-only), skipped at its thread. Ported from ITC-Recomp
// (src/hooks/voice.cpp, B5 in its docs/rexglue-removal.md): the voice
// library's worker (sub_825613F8; ITC sub_8257B5D8, the same code) calls
// Voice::Poll (sub_82560D50) every 10 ms, which in play reads a notification
// into a flag only never-called code reads, sets up an audio voice nothing
// else uses, and finds no talkers. The engine itself is still made by
// Voice::Init (the game calls it without null checks).
//
// --voice_thread=true runs the original worker.

#include "guest.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(voice_thread, false, "SBX",
                    "Run the voice chat library's 10 ms worker (Xbox-only; it finds no "
                    "talkers on PC)");

// The thread's entry: r3 = the engine. Signals the event Voice::Init waits on
// (+1304, "started") and ends; nothing waits on the thread until the engine's
// release, which finds it ended.
extern "C" REX_FUNC(sub_825613F8) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(voice_thread)) {
    __imp__sub_825613F8(ctx, base);
    return;
  }
  const uint32_t engine = ctx.r3.u32;
  // The original's frame (stwu r1,-112(r1)) and the return address of its
  // SetEvent call (bl 0x8259F898 at 0x8256146C).
  guest_call::InFrame(ctx, base, 112, 0x82561470, [&](uint32_t) {
    ctx.r3.u64 = guest::LoadU32(base, engine + 1304);
    sub_8259F898(ctx, base);  // SetEvent
  });
  ctx.r3.u64 = 0;
}
