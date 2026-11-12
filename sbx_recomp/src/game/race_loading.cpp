// The race loading's course set-up as our own C++ (--native_race_loading,
// on by default): its begin and its course-ready step.

#include "game/game_modes.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_loading, true, "SBX",
                    "Our own C++ for the course set-up's begin and course-ready steps instead "
                    "of the recompiled functions");

namespace {

using guest::u32_be;

// The course set-up: a state machine, then its own words.
struct CourseSetup {
  game::StateMachine machine;
  u32_be unknown_9608;
  uint8_t unknown_9612[4];
  u32_be unknown_9616[2];
  uint8_t unknown_9624[4];
  u32_be unknown_9628[3];
  u32_be loading_course;  // Cleared once the course is ready.
};
GUEST_OFFSET(CourseSetup, loading_course, 9640);

bool On() { return REXCVAR_GET(native_race_loading); }

}  // namespace

// CourseSetup::OnCourseReady: r3 the set-up. Returns it.
extern "C" REX_FUNC(sub_8214F450) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8214F450(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  guest::At<CourseSetup>(ctx.r3.u32)->loading_course = 0;
}

// CourseSetup::Begin: r3 the set-up. Returns it.
extern "C" REX_FUNC(sub_8214F460) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8214F460(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  CourseSetup* setup = guest::At<CourseSetup>(ctx.r3.u32);
  setup->unknown_9616[0] = 0;
  setup->unknown_9616[1] = 0;
  setup->unknown_9608 = 0;
  for (u32_be& word : setup->unknown_9628) {
    word = 0;
  }
}
