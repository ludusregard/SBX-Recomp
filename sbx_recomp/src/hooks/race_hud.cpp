// The race HUD's movie as our own C++ (--native_race_hud, on by default):
// RaceHud::RunMovie (sub_82211F50), which picks the HUD's UI movie by its
// state and runs or draws it.
//
// Written from SBX's generated code. The movies' calls stay recompiled.
// The rival logo's draw (sub_8220F7B0) keeps subtick's override
// (subtick/per_frame_logic.cpp), which calls ours below in place of the
// recompiled function.
//
// The HUD: +448 the state, +456 whether movie +604 runs (else it's only
// drawn), +604, +624 and +632 its movies.

#include "guest.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_hud, true, "SBX",
                    "Our own C++ for RaceHud::RunMovie (which UI movie the race HUD runs) "
                    "instead of the recompiled function; the movies stay recompiled");

// RaceHud::RunMovie: r3 = the HUD, r4 passed on to UiMovie::Run (0 to
// advance, else draw only). The original's calls are all tail calls, so each
// returns what its callee leaves.
//   0     movie +604: UiMovie::Run (sub_8211E378) when +456 is set, else
//         UiMovie::Draw (sub_8211D440)
//   1     sub_82210690 (the race timer's movie, +624)
//   2, 4  movie +632: UiMovie::Run
//   3     sub_822106E0
//   5 on  nothing
extern "C" REX_FUNC(sub_82211F50) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_race_hud)) {
    __imp__sub_82211F50(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32;
  switch (guest::LoadU32(base, hud + 448)) {
    case 0:
      ctx.r3.u64 = guest::LoadU32(base, hud + 604);
      if (guest::LoadU32(base, hud + 456) != 0) {
        sub_8211E378(ctx, base);
      } else {
        sub_8211D440(ctx, base);
      }
      return;
    case 1:
      sub_82210690(ctx, base);
      return;
    case 2:
    case 4:
      ctx.r3.u64 = guest::LoadU32(base, hud + 632);
      sub_8211E378(ctx, base);
      return;
    case 3:
      sub_822106E0(ctx, base);
      return;
    default:
      return;
  }
}

// RivalLogo::Draw: r3 the logo's entry (+4 its flags, +8 its movie's key).
// With flag 8 set, the movie looked up in the movie table (sub_8211B5C0,
// [0x826B59AC], the key in the original's 112-byte frame at +80) and run
// (UiMovie::Run, r4 = 0); when that returns 0 (it's over), UiMovie::Finish
// (sub_8211E0E0, r4 = the key) and flag 16 set. Called from subtick's
// override (subtick/per_frame_logic.cpp), which keeps the logo's entry for
// its render-only passes.
void hooks::NativeRaceHud_RivalLogoDraw(PPCContext& ctx, uint8_t* base) {
  if (!REXCVAR_GET(native_race_hud)) {
    __imp__sub_8220F7B0(ctx, base);
    return;
  }
  const uint32_t entry = ctx.r3.u32;
  if ((guest::LoadU32(base, entry + 4) & 8) == 0) {
    return;
  }
  guest::StackFrame frame(ctx, 112);
  const uint32_t key = frame.Local(80);
  guest::StoreU32(base, key, guest::LoadU32(base, entry + 8));
  ctx.r3.u64 = 0x826B59AC;  // the movie table
  ctx.r4.u64 = key;
  sub_8211B5C0(ctx, base);
  const uint32_t movie = guest::LoadU32(base, ctx.r3.u32);
  ctx.r3.u64 = movie;
  ctx.r4.u64 = 0;
  sub_8211E378(ctx, base);
  if (ctx.r3.u32 != 0) {
    return;
  }
  ctx.r3.u64 = movie;
  ctx.r4.u64 = guest::LoadU32(base, entry + 8);
  sub_8211E0E0(ctx, base);
  guest::StoreU32(base, entry + 4, guest::LoadU32(base, entry + 4) | 16);
}
