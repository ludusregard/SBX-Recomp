// Finds game state that advances on render-only passes (--find_per_frame_state).
// Ported from ITC-Recomp (src/debug/frame_state_diff.cpp), with a Windows
// memory map.
//
// A render-only pass should leave game state alone; anything that counts
// frames there runs too fast with --subtick (ITC found its race start
// countdown this way). Guest memory is compared before and after a sampled
// render-only pass, and words that step like a timer (a float by +-1/30 or
// +-1/60, an integer by +-1) are logged and, once seen in two samples,
// watched. On later render-only passes each watched word is checked around
// every main loop tick handler and every delivered message, to name the code
// that advances it.
//
// Samples: one render-only pass every --find_per_frame_state_interval_ms (0:
// every one; slow). (It found the play time counter; the race-exit fades,
// subtick/per_frame_logic.cpp, were found by comparing all of memory over a
// race's end, a one-off since removed.)

#pragma once

#include <cstdint>

namespace debug {

// Main thread, as a main loop pass is about to start (after the frame wait).
void FrameStateDiff_OnPassStart(uint8_t* base, bool render_only, bool in_race);

// Main thread, around a tick handler or a delivered message (by its target),
// on render-only passes.
void FrameStateDiff_Before(uint8_t* base);
void FrameStateDiff_After(uint8_t* base, const char* kind, uint32_t id);

}  // namespace debug
