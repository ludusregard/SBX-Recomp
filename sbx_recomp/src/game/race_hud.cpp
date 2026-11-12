// The race HUD's movie as our own C++ (--native_race_hud, on by default):
// RaceHud::RunMovie, which picks the HUD's UI movie by its state and runs
// or draws it, and the rival logo's draw.
//
// The movies' own calls stay recompiled. RivalLogo::Draw keeps subtick's
// override (subtick/per_frame_logic.cpp), which calls ours below in place
// of the recompiled function.

#include "game/race_hud.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_hud, true, "SBX",
                    "Our own C++ for RaceHud::RunMovie (which UI movie the race HUD runs) "
                    "instead of the recompiled function; the movies stay recompiled");

namespace {

using game::RivalLogo;
using guest::At;
using guest::Guest;
using guest::u32_be;

struct UiMovie;

struct RaceHud {
  enum State : uint32_t {
    kMain = 0,
    kTimer = 1,
    kOverlayA = 2,
    kState3 = 3,
    kOverlayB = 4,
  };

  uint8_t unknown_0[448];
  u32_be state;
  uint8_t unknown_452[4];
  u32_be main_movie_runs;  // Else the main movie is only drawn.
  uint8_t unknown_460[144];
  guest::ptr<UiMovie> main_movie;
  uint8_t unknown_608[16];
  guest::ptr<UiMovie> timer_movie;
  uint8_t unknown_628[4];
  guest::ptr<UiMovie> overlay_movie;
};
GUEST_OFFSET(RaceHud, state, 448);
GUEST_OFFSET(RaceHud, main_movie, 604);
GUEST_OFFSET(RaceHud, overlay_movie, 632);

constexpr uint32_t kMovieTable = 0x826B59AC;

}  // namespace

// RaceHud::RunMovie: r3 = the HUD, r4 passed on to UiMovie::Run (0 to
// advance, else draw only). Returns what the movie's call returns.
extern "C" REX_FUNC(sub_82211F50) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_race_hud)) {
    __imp__sub_82211F50(ctx, base);
    return;
  }
  Guest g(ctx, base);
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  switch (hud->state) {
    case RaceHud::kMain:
      ctx.r3.u64 = hud->main_movie.address;
      if (hud->main_movie_runs != 0) {
        sub_8211E378(ctx, base);  // UiMovie::Run
      } else {
        sub_8211D440(ctx, base);  // UiMovie::Draw
      }
      return;
    case RaceHud::kTimer:
      sub_82210690(ctx, base);  // The race timer's movie.
      return;
    case RaceHud::kOverlayA:
    case RaceHud::kOverlayB:
      ctx.r3.u64 = hud->overlay_movie.address;
      sub_8211E378(ctx, base);  // UiMovie::Run
      return;
    case RaceHud::kState3:
      sub_822106E0(ctx, base);
      return;
    default:
      return;
  }
}

// RivalLogo::Draw: r3 the logo. Runs a shown logo's movie; when the movie
// is over it's finished and the logo marked so.
void hooks::NativeRaceHud_RivalLogoDraw(PPCContext& ctx, uint8_t* base) {
  if (!REXCVAR_GET(native_race_hud)) {
    __imp__sub_8220F7B0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  RivalLogo* logo = At<RivalLogo>(ctx.r3.u32);
  if ((logo->flags & RivalLogo::kShown) == 0) {
    return;
  }
  // The movie table is searched by a key passed by address.
  guest::StackFrame frame(ctx, 112);
  u32_be* key = At<u32_be>(frame.Local(80));
  *key = logo->movie_key;
  const uint32_t movie = *At<u32_be>(g.Call(sub_8211B5C0, kMovieTable, key));
  const bool running = g.Call(sub_8211E378, movie, 0u) != 0;  // UiMovie::Run
  if (!running) {
    g.Call(sub_8211E0E0, movie, logo->movie_key);  // UiMovie::Finish
    logo->flags |= RivalLogo::kFinished;
  }
}
