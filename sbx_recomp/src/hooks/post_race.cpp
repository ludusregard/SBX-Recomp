// The post-race check (sub_8218C240): r3 = the race object.
//
// After a race is loaded, the game counts checks in a global at 0x826C7EC4 and,
// once the count reaches its limit, moves the race object to its next state
// (+9604): 9, or 4 when +9612 is 0. The original limit is 60. On fast hardware
// that leaves the screen black after quest races (Gliniak's Xenia patch,
// xenia-project/game-compatibility#651).
//
// --post_race_check_cycles is that limit. 60 is the original; the default 1
// moves on at the first check. Values in between find the lowest limit that
// doesn't hang.
//
// With --subtick, the check runs on logic passes only (once per 30 Hz tick, as
// on the Xbox); render-only passes skip it (--per_tick_post_race_check). If
// it runs from Draw, render-only passes would otherwise count it at the frame
// rate and could force the next state on a pass whose Update is skipped,
// which may be how the music's cue is lost after the tutorial race.
//
// The whole function is overridden: the generated body is copied here with
// its control flow written out. If codegen's output for it changes, check it
// again: findstr /n "DEFINE_REX_FUNC(sub_8218C240)" generated\default\*.cpp

#include "guest.h"
#include "subtick/game_messages.h"

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DECLARE(bool, subtick);

REXCVAR_DEFINE_INT32(post_race_check_cycles, 1, "SBX",
                     "Post-race check: how many checks to count before the race moves to "
                     "its next screen (the original is 60; 1 fixes the black screen after "
                     "quest races)");
REXCVAR_DEFINE_BOOL(per_tick_post_race_check, true, "SBX",
                    "With --subtick, the post-race check runs once per tick (logic passes "
                    "only), as on the Xbox");

namespace {

constexpr uint32_t kCurrentRace = 0x826C7EC0;  // The race object, as sub_8218C0F8/C140 take it.
constexpr uint32_t kCheckCount = 0x826C7EC4;   // Checks counted so far.

// Offsets in the race object.
constexpr uint32_t kRaceLoaded = 9608;      // Set once the race has loaded.
constexpr uint32_t kNextState = 9604;       // The state the race moves to.
constexpr uint32_t kSomeFlag = 9612;        // Picks the next state: 9 when set, 4 when clear.

// sub_8211C068: a check, given r3; nonzero when it passes.
bool PassesCheck(PPCContext& ctx, uint8_t* base, uint32_t which) {
  ctx.r3.u64 = which;
  sub_8211C068(ctx, base);
  return ctx.r3.u32 != 0;
}

// The loading checks: all of them must pass before the race counts as loaded.
bool RaceLoaded(PPCContext& ctx, uint8_t* base) {
  if (!PassesCheck(ctx, base, 0) || !PassesCheck(ctx, base, 9) || !PassesCheck(ctx, base, 10)) {
    return false;
  }
  uint32_t race = guest::LoadU32(base, kCurrentRace);
  ctx.r3.u64 = race;
  sub_8218C0F8(ctx, base);
  if (ctx.r3.u32 == 0) {
    return false;
  }
  ctx.r3.u64 = race;
  sub_8218C140(ctx, base);
  return ctx.r3.u32 != 0;
}

}  // namespace

extern "C" REX_FUNC(sub_8218C240) {
  REX_FUNC_PROLOGUE();
  uint32_t race = ctx.r3.u32;

  uint64_t pass = 0;
  bool render_only = false;
  bool main_thread = false;
  subtick::GameMessages_PassFromAnyThread(pass, render_only, main_thread);
  render_only = render_only && main_thread && REXCVAR_GET(subtick);
  if (render_only && REXCVAR_GET(per_tick_post_race_check)) {
    return;
  }

  if (guest::LoadU32(base, race + kRaceLoaded) == 0 && RaceLoaded(ctx, base)) {
    guest::StoreU32(base, race + kRaceLoaded, 1);
  }

  // Count one check. The checks come in blocks of 15: in every other block (the
  // first, third, ...) the game makes a call (sub_821611F0) with the arguments
  // below before the count goes up.
  uint32_t checks = guest::LoadU32(base, kCheckCount);
  if ((checks / 15) % 2 == 0) {
    ctx.r3.u64 = 100;
    ctx.r4.u64 = 30;
    ctx.r5.u64 = 0;
    ctx.r6.u64 = 0x8200A128;
    sub_821611F0(ctx, base);
    checks = guest::LoadU32(base, kCheckCount);
  }
  ++checks;
  guest::StoreU32(base, kCheckCount, checks);

  if (guest::LoadU32(base, race + kRaceLoaded) == 0) {
    return;
  }
  uint32_t limit = uint32_t(REXCVAR_GET(post_race_check_cycles));
  if (checks < limit) {
    return;
  }

  guest::StoreU32(base, kCheckCount, 0);
  uint32_t next_state = guest::LoadU32(base, race + kSomeFlag) != 0 ? 9 : 4;
  guest::StoreU32(base, race + kNextState, next_state);
}
