// The career's records as our own C++ (--native_career, on by default):
// the play time's mark and the win streaks' getters.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_career, true, "SBX",
                    "Our own C++ for the career's play time mark and win streak getters "
                    "instead of the recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u16_be;
using guest::u32_be;

// The game's play clock, in its writable data.
constexpr uint32_t kPlayClock = 0x826B5870;

struct PlayTime {
  uint8_t unknown_0[6088];
  guest::f32_be mark;  // The play clock when last marked.
};

// A day's block of the career's records.
struct DayBlock {
  uint8_t unknown_0[4828];
  u16_be win_streak;
  u16_be best_streak;
  u32_be unknown_4832;
};

struct Stats {
  uint8_t unknown_0[5970];
  u16_be win_streak;
};

bool On() { return REXCVAR_GET(native_career); }

}  // namespace

#define SBX_CAREER_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();              \
  if (!On()) {                      \
    __imp__##name(ctx, base);       \
    return;                         \
  }                                 \
  Guest g(ctx, base);

// PlayTime::Mark: r3 the play time; the play clock noted. Returns it.
extern "C" REX_FUNC(sub_82111598) {
  SBX_CAREER_FALLBACK(sub_82111598)
  At<PlayTime>(ctx.r3.u32)->mark = *At<guest::f32_be>(kPlayClock);
}

extern "C" REX_FUNC(sub_821104A8) {  // DayBlock::WinStreak
  SBX_CAREER_FALLBACK(sub_821104A8)
  ctx.r3.u64 = At<DayBlock>(ctx.r3.u32)->win_streak;
}

extern "C" REX_FUNC(sub_821104B0) {  // DayBlock::BestStreak
  SBX_CAREER_FALLBACK(sub_821104B0)
  ctx.r3.u64 = At<DayBlock>(ctx.r3.u32)->best_streak;
}

extern "C" REX_FUNC(sub_821104C8) {  // DayBlock::Word4832
  SBX_CAREER_FALLBACK(sub_821104C8)
  ctx.r3.u64 = At<DayBlock>(ctx.r3.u32)->unknown_4832;
}

extern "C" REX_FUNC(sub_82110A80) {  // Stats::WinStreak
  SBX_CAREER_FALLBACK(sub_82110A80)
  ctx.r3.u64 = At<Stats>(ctx.r3.u32)->win_streak;
}
