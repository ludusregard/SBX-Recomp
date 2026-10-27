// Voice chat (Xbox-only), skipped at its thread. Ported from ITC-Recomp
// (src/hooks/voice.cpp, B5 in its docs/rexglue-removal.md): the voice
// library's worker (sub_825613F8; ITC sub_8257B5D8, the same code) calls
// Voice::Poll (sub_82560D50) every 10 ms, which in play reads a notification
// into a flag only never-called code reads, sets up an audio voice nothing
// else uses, and finds no talkers. The engine itself is still made by
// Voice::Init (the game calls it without null checks).

#include "guest.h"
#include "hooks/guest_call.h"

extern "C" REX_FUNC(sub_825613F8) {
  REX_FUNC_PROLOGUE();

  const uint32_t engine = ctx.r3.u32;

  // Signal the "started" event without running the voice worker.
  guest_call::InFrame(ctx, base, 112, 0x82561470, [&](uint32_t) {
    ctx.r3.u64 = guest::LoadU32(base, engine + 1304);
    sub_8259F898(ctx, base);  // SetEvent
  });

  ctx.r3.u64 = 0;
}
