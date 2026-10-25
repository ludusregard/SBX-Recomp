// The game's movie class on our own FFmpeg player (native renderer builds):
// adapters in front of src/movies/movie.h, behind --play_movies_with_ffmpeg.
// Ported from ITC-Recomp (src/hooks/movies.cpp, its docs/native-movies.md);
// each SBX function checked against ITC's in generated/ (seven exact, State
// the same code). Slot 8 (Movie::Open, the library's player made on a
// worker) is no longer called; slots 9 and 10 (the tick handler that queues
// command 28, and the listener) stay recompiled. The callers read no r3 but
// State's: the rest leave r3 as it came. The emulated GPU build keeps the
// game's movie library, which draws through it.

#include "guest.h"

#include "hooks/title_screen.h"
#include "movies/movie.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(play_movies_with_ffmpeg, true, "SBX",
                    "Play the game's movies through our own FFmpeg and draw them with the native "
                    "renderer instead of through the movie library linked into the game. Set at "
                    "launch.");

namespace mv = movies::movie;

#define SBX_MOVIE(address, ...)                     \
  extern "C" REX_FUNC(sub_##address) {              \
    REX_FUNC_PROLOGUE();                            \
    if (!REXCVAR_GET(play_movies_with_ffmpeg)) {    \
      __imp__sub_##address(ctx, base);              \
      return;                                       \
    }                                               \
    __VA_ARGS__;                                    \
  }

SBX_MOVIE(820AEAF8, mv::Play(base, ctx.r3.u32, ctx.r4.u32))       // ITC 820AEC10
SBX_MOVIE(820AE3C8, mv::Start(base, ctx.r3.u32))                  // ITC 820AE4E0
SBX_MOVIE(820AE710, mv::Stop(base, ctx.r3.u32))                   // ITC 820AE828
SBX_MOVIE(820AE780, mv::Pause(base, ctx.r3.u32))                  // ITC 820AE898
SBX_MOVIE(820AE7E0, mv::Resume(base, ctx.r3.u32))                 // ITC 820AE8F8
SBX_MOVIE(820AE968, ctx.r3.u64 = mv::GetState(base, ctx.r3.u32))  // ITC 820AEA80
SBX_MOVIE(820AE410, mv::Release(base, ctx.r3.u32))                // ITC 820AE528

#undef SBX_MOVIE

// Movie::RenderFrame (ITC sub_820AE958), on the render thread from command
// 28. --skip_intro waits for it to have returned once (hooks/title_screen.cpp).
extern "C" REX_FUNC(sub_820AE840) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(play_movies_with_ffmpeg)) {
    mv::RenderFrame(base, ctx.r3.u32);
  } else {
    __imp__sub_820AE840(ctx, base);
  }
  hooks::TitleScreen_MovieRendered();
}
