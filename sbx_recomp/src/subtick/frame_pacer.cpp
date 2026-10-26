#include "subtick/frame_pacer.h"

#include <rex/cvar.h>

#include <algorithm>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

REXCVAR_DEFINE_INT32(max_fps, 0, "SBX",
                     "With --subtick: frame rate cap in races; 0 means unlimited");

namespace subtick {

namespace {

#ifdef _WIN32
// Windows rounds sleeps up to the system timer (often 15.6 ms), far too
// coarse for a 144 fps cap. Sleep on a high-resolution waitable timer until
// a millisecond before, then spin the rest.
void SleepUntil(FramePacer::Clock::time_point until) {
  constexpr auto kSpin = std::chrono::milliseconds(1);
  thread_local HANDLE timer = CreateWaitableTimerExW(
      nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
  auto sleep = until - FramePacer::Clock::now() - kSpin;
  if (timer && sleep > FramePacer::Clock::duration::zero()) {
    LARGE_INTEGER due;
    // Relative, in 100 ns units.
    due.QuadPart = -std::chrono::duration_cast<std::chrono::nanoseconds>(sleep).count() / 100;
    if (SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0)) {
      WaitForSingleObject(timer, INFINITE);
    }
  }
  while (FramePacer::Clock::now() < until) {
    YieldProcessor();
  }
}
#else
void SleepUntil(FramePacer::Clock::time_point until) {
  std::this_thread::sleep_until(until);
}
#endif

}  // namespace

void FramePacer::OnFrameStarted() {
  last_frame_ = Clock::now();
  frame_progress_ = TickProgress();
}

bool FramePacer::WaitForNextFrame() {
  // A tick that's already due goes first. The frame slot counts from the
  // last frame's start, so after a frame longer than the cap's period it
  // lies in the past and could otherwise win over an overdue tick, holding
  // ticks back a frame at a time on slow machines.
  if (Clock::now() >= next_tick_) {
    return false;
  }
  Clock::time_point next_frame = last_frame_ + FramePeriod();
  if (next_frame >= next_tick_) {
    SleepUntil(next_tick_);
    return false;
  }
  SleepUntil(next_frame);
  return true;
}

void FramePacer::WaitForNextTick() {
  SleepUntil(next_tick_);
}

void FramePacer::OnTickReleased() {
  Clock::time_point now = Clock::now();
  // Stay on the 30 Hz grid, unless we've fallen a whole tick behind (a
  // loading screen, say): then restart the grid from now rather than rush
  // ticks to catch up.
  Clock::time_point next = next_tick_ + kTickPeriod;
  if (next < now) {
    next = now + kTickPeriod;
  }
  // Nor run ahead of it: ticks released without waiting (--skip_cutscenes's
  // fast boot released ~1,100 at once) would otherwise put the next one
  // that many periods away, and the game sat 36 s on the menu.
  if (next > now + kTickPeriod) {
    next = now + kTickPeriod;
  }
  // Progress counts from when the tick was due, not when it was released, so
  // a late release doesn't stretch the interval.
  last_tick_ = std::min(now, next_tick_);
  ticks_released_++;
  next_tick_ = next;
}

float FramePacer::TickProgress() const {
  auto elapsed = Clock::now() - last_tick_;
  return std::clamp(float(elapsed.count()) / float(kTickPeriod.count()), 0.0f, 1.0f);
}

FramePacer::Clock::duration FramePacer::FramePeriod() const {
  int max_fps = REXCVAR_GET(max_fps);
  if (max_fps <= 0) {
    return Clock::duration::zero();
  }
  return std::chrono::duration_cast<Clock::duration>(std::chrono::seconds(1)) / max_fps;
}

FramePacer& GetFramePacer() {
  static FramePacer pacer;
  return pacer;
}

}  // namespace subtick
