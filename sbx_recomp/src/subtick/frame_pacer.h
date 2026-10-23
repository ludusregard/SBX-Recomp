// Decides when frames are drawn and when game logic may tick (--subtick).
//
// Game logic advances a fixed 1/30 s per tick (see "Fixed time step" in
// docs/functions.md), so ticks are released on a steady 30 Hz clock. Between
// ticks, races draw as many frames as --max_fps allows. Pure host code: it
// knows nothing about guest state.

#pragma once

#include <chrono>

namespace subtick {

class FramePacer {
 public:
  using Clock = std::chrono::steady_clock;

  static constexpr int kTicksPerSecond = 30;
  static constexpr Clock::duration kTickPeriod =
      std::chrono::duration_cast<Clock::duration>(std::chrono::seconds(1)) / kTicksPerSecond;

  // Call just before a frame's pass starts.
  void OnFrameStarted();

  // Sleeps until the next frame may start, honoring the frame cap.
  // Returns false instead if the next tick is due first.
  bool WaitForNextFrame();

  // Sleeps until the next tick is due.
  void WaitForNextTick();

  // Call when releasing game logic for its next tick. Schedules the tick
  // after it.
  void OnTickReleased();

  // How many game ticks have been released so far.
  uint64_t TicksReleased() const { return ticks_released_; }

  // How far the current moment is between the last tick and the next, from
  // 0 to 1.
  float TickProgress() const;

  // TickProgress() as it was when the current frame's pass started
  // (OnFrameStarted). Everything blended for one frame uses this, so the
  // cars and the camera agree even when the pass runs for several ms (a
  // logic pass builds its camera early in Update and draws the cars ~10 ms
  // later).
  float FrameProgress() const { return frame_progress_; }

 private:
  Clock::duration FramePeriod() const;

  Clock::time_point next_tick_{};
  Clock::time_point last_tick_{};
  Clock::time_point last_frame_{};
  uint64_t ticks_released_ = 0;
  float frame_progress_ = 0;
};

FramePacer& GetFramePacer();

}  // namespace subtick
