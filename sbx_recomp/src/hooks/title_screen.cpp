// The boot up to the title screen, and the title screen's idle wait before
// the attract race: --skip_intro and --attract_after_s for automated runs,
// --log_boot_modes to see the sequence. Ported from ITC-Recomp
// (src/hooks/title_screen.cpp, its docs/title-screen.md); addresses through
// docs/itc_to_sbx_functions.tsv, the globals by the title update's code.
//
// The title screen's update (sub_8221FC48; ITC sub_822221A0) counts idle
// ticks in +36 of the object at [0x8280DE44] (ITC 0x82814E34) and, at 600
// (20 s at 30 Hz), sets the object's state (+4) to 5, which loads an
// attract-mode race. The front end's state objects are at [0x8280DE3C],
// [0x8280DE40] and [0x8280DE44] (ITC 0x82814E2C-34), created when the boot
// movie ends.
//
// --skip_intro: a boot movie reports finished as soon as the render thread
// has started it, and the main loop runs without its 30 Hz wait
// (subtick/renderer_hooks.cpp) through the title screen's intro (SBX: the
// Genki logos and the warning, played by the title screen while it counts
// idle ticks), to 10 ticks short of its attract wait; the count is then put
// back to 0. ITC kept the fast loop until the first race, which also ran the
// title's 20 s idle wait at once and started the attract race straight away
// (and, with --subtick here, its load hung).
// Failing the movie file's open instead shows the dirty disc error (ITC).

#include "hooks/title_screen.h"

#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <atomic>
#include <format>
#include <string>

REXCVAR_DEFINE_BOOL(log_boot_modes, false, "SBX",
                    "Log the front end's state objects as they change, and the boot movie's "
                    "wait");
REXCVAR_DEFINE_BOOL(skip_intro, false, "SBX",
                    "Skip the boot movie, and run the logos and the title screen's intro "
                    "without the 30 Hz wait (until the title screen waits for start)");
REXCVAR_DEFINE_INT32(attract_after_s, 0, "SBX",
                     "Testing: seconds the title screen waits before the attract race "
                     "(0 = the game's 20)");

namespace {

constexpr uint32_t kFrontEndPointers[] = {0x8280DE3C, 0x8280DE40, 0x8280DE44};
constexpr uint32_t kTitleScreenPointer = 0x8280DE44;
constexpr uint32_t kIdleTicksOffset = 36;
constexpr int32_t kAttractTicks = 600;
constexpr int32_t kTicksPerSecond = 30;
// The title ticks --skip_intro runs at once (its intro, short of the attract
// wait).
constexpr int32_t kFastForwardTicks = kAttractTicks - 10;

std::atomic<bool> g_first_race_started{false};
// The title screen has started counting idle ticks ("press start" is up):
// --skip_intro's fast boot ends there.
std::atomic<bool> g_title_idle{false};
bool g_in_movie_wait = false;
// Set by the render thread once it has started the boot movie (see
// sub_820AE840).
std::atomic<bool> g_movie_started{false};

}  // namespace

namespace hooks {

bool TitleScreen_SkipTickWait() {
  return REXCVAR_GET(skip_intro) && !g_title_idle.load(std::memory_order_relaxed) &&
         !g_first_race_started.load(std::memory_order_relaxed);
}

void TitleScreen_OnRaceDrawn() {
  g_first_race_started.store(true, std::memory_order_relaxed);
}

void TitleScreen_MovieRendered() {
  g_movie_started.store(true, std::memory_order_release);
}

}  // namespace hooks

// TitleScreen::Update (ITC sub_822221A0, order 0.99; its 600 and +36 checked
// in generated/). With --attract_after_s, after a tick that counted, the
// counter jumps to one short of the game's limit once it reaches the wait,
// so the next tick starts the attract race.
extern "C" REX_FUNC(sub_8221FC48) {
  REX_FUNC_PROLOGUE();
  int32_t after = REXCVAR_GET(attract_after_s) * kTicksPerSecond;
  uint32_t title = guest::LoadU32(base, kTitleScreenPointer);
  int32_t before = title ? int32_t(guest::LoadU32(base, title + kIdleTicksOffset)) : 0;
  __imp__sub_8221FC48(ctx, base);
  if (!title || guest::LoadU32(base, kTitleScreenPointer) != title) {
    return;
  }
  int32_t now = int32_t(guest::LoadU32(base, title + kIdleTicksOffset));
  // --skip_intro: SBX's title screen plays its intro (the Genki logos, the
  // warning) while it counts idle ticks, and takes start only after it. So
  // the fast boot runs the title screen until just short of its attract
  // wait (the intro is shorter than that), then puts the count back to 0:
  // the menu is ready and the attract race is still the game's 20 s away.
  if (REXCVAR_GET(skip_intro) && !g_title_idle.load(std::memory_order_relaxed) &&
      now > before && now >= kFastForwardTicks) {
    guest::StoreU32(base, title + kIdleTicksOffset, 0);
    now = 0;
    g_title_idle.store(true, std::memory_order_relaxed);
    if (REXCVAR_GET(log_boot_modes)) {
      REXLOG_INFO("boot modes: title screen ran {} ticks, --skip_intro's fast boot ends",
                  kFastForwardTicks);
    }
  }
  if (after <= 0 || hooks::TitleScreen_SkipTickWait()) {
    return;  // (not during --skip_intro's fast-forward)
  }
  if (now == before + 1 && now >= after && now < kAttractTicks - 1) {
    guest::StoreU32(base, title + kIdleTicksOffset, uint32_t(kAttractTicks - 1));
  }
}

// Scene::Update (ITC sub_820988C8, exact): r3 = the scene. Hooked for
// --log_boot_modes: logs the front end's state objects as they change.
extern "C" REX_FUNC(sub_82098DF0) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(log_boot_modes)) {
    static std::string last;
    std::string now;
    for (uint32_t pointer : kFrontEndPointers) {
      uint32_t object = guest::LoadU32(base, pointer);
      now += std::format(" [{:08X}]={:08X}:", pointer, object);
      for (uint32_t i = 0; object && i < 24; i += 4) {
        now += std::format(" {}", int32_t(guest::LoadU32(base, object + i)));
      }
    }
    if (now != last) {
      REXLOG_INFO("boot modes:{}", now);
      last = now;
    }
  }
  __imp__sub_82098DF0(ctx, base);
}

// Messages::PickHandler (sub_82106410; subtick/game_messages.cpp calls it
// for each handler set): r3 a handler set, r4 the message (its id at +0).
// Looks up the set's state (+9604) and the message in its table
// (sub_82106380) and moves to the entry's next state (+28) if it has one.
// Hooked for --log_boot_modes: logs each state change, to map the boot's
// states (for a boot straight to the menu).
extern "C" REX_FUNC(sub_82106410) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(log_boot_modes)) {
    __imp__sub_82106410(ctx, base);
    return;
  }
  uint32_t set = ctx.r3.u32;
  uint32_t message = guest::LoadU32(base, ctx.r4.u32);
  uint32_t before = guest::LoadU32(base, set + 9604);
  __imp__sub_82106410(ctx, base);
  uint32_t after = guest::LoadU32(base, set + 9604);
  if (after != before) {
    REXLOG_INFO("boot modes: handler set {:08X} state {} -> {} on message {}", set, before,
                after, message);
  }
}

// Boot::WaitForMovie (ITC sub_82107C58, exact): the boot's movie step, run
// every tick while the logo movie plays. Ends the step when the movie is
// skippable and a button is pressed, or when it has finished
// (Movie::IsFinished returns 1) and this+9608 is 1. With --skip_intro the
// movie reports finished as soon as the render thread has started it.
extern "C" REX_FUNC(sub_82107D80) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(log_boot_modes)) {
    REXLOG_INFO("boot modes: movie wait, this {:08X} +9608 {}", ctx.r3.u32,
                guest::LoadU32(base, ctx.r3.u32 + 9608));
  }
  g_in_movie_wait = REXCVAR_GET(skip_intro);
  __imp__sub_82107D80(ctx, base);
  g_in_movie_wait = false;
}

// Movie::IsFinished (ITC sub_820A4450, exact): 1 when the movie has finished.
extern "C" REX_FUNC(sub_820A4678) {
  REX_FUNC_PROLOGUE();
  if (g_in_movie_wait && g_movie_started.load(std::memory_order_acquire)) {
    ctx.r3.u64 = 1;
    return;
  }
  __imp__sub_820A4678(ctx, base);
}

// Movie::RenderFrame (ITC sub_820AE958, exact), the render thread's command
// that draws the boot movie. The library's waits in the movie player for the
// first decoded frame; if the movie step ended the movie before then, the
// main thread would wait on the render thread in Boot::EndMovie while the
// decoder never delivers (ITC: about 1 boot in 6 stalled that way). So
// --skip_intro waits for this. With the native renderer, the hook is in
// hooks/movies.cpp (the movie on our own player).
#ifndef SBX_NATIVE_RENDERER
extern "C" REX_FUNC(sub_820AE840) {
  REX_FUNC_PROLOGUE();
  __imp__sub_820AE840(ctx, base);
  hooks::TitleScreen_MovieRendered();
}
#endif
