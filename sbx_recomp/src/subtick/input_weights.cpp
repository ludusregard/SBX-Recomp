// The input a tick's physics uses, from the samples taken over the last tick's
// interval (--subtick_input_sensitivity, 0.0 to 1.0).
//
// The samples are weighted as a whole (all five axes together, so a steering
// flick and a brake blip count the same way). Two things add weight:
//   - the newest sample, the tick's own record (F): weight L;
//   - the size of a sample's change from the one before it, relative to the
//     biggest change in the interval: weight P, from 0 to 1. A quick steering
//     flick or brake blip gets its weight from this.
// The first sample is last tick's and gets no weight. At sensitivity s the
// weight of a sample is (1 - s) L + s (L + P) / 2, normalized to sum to 1:
//   0: the tick's own record only, as the game reads it;
//   1: half newest, half the biggest changes.
// The result is a weighted average of the samples' axes.

#include "subtick/input.h"

#include <rex/cvar.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

REXCVAR_DEFINE_DOUBLE(subtick_input_sensitivity, 0.5, "SBX",
                      "With --subtick_input: how much the input's biggest changes between ticks "
                      "count toward a tick's input, from 0.0 (the tick's own input only) to 1.0 "
                      "(half the newest input, half the biggest changes)");

namespace subtick {

ControlSample Input_Aggregate(const std::vector<ControlSample>& samples) {
  if (samples.empty()) {
    return ControlSample{};
  }
  const size_t last = samples.size() - 1;
  if (last == 0) {
    return samples[0];
  }
  const double s = std::clamp(REXCVAR_GET(subtick_input_sensitivity), 0.0, 1.0);

  // Each sample's change from the one before it, over all axes, with each axis
  // scaled to the 16-bit range.
  std::vector<double> change(samples.size(), 0.0);
  double peak = 0;
  for (size_t i = 1; i < samples.size(); i++) {
    double sum = 0;
    for (int k = 0; k < 5; k++) {
      sum += std::abs(double(samples[i].axes[k]) - double(samples[i - 1].axes[k])) / 32767.0;
    }
    change[i] = sum;
    peak = std::max(peak, sum);
  }

  std::vector<double> weight(samples.size(), 0.0);
  double total = 0;
  for (size_t i = 1; i < samples.size(); i++) {
    double newest = i == last ? 1.0 : 0.0;
    double biggest = peak > 0 ? change[i] / peak : 0.0;
    weight[i] = (1 - s) * newest + s * (newest + biggest) / 2;
    total += weight[i];
  }
  if (total <= 0) {
    return samples[last];
  }

  ControlSample out;
  out.at = samples[last].at;
  out.presence = samples[last].presence;  // the presence bits are flags, not blended
  for (int k = 0; k < 5; k++) {
    double value = 0;
    for (size_t i = 1; i < samples.size(); i++) {
      value += weight[i] / total * double(samples[i].axes[k]);
    }
    out.axes[k] = int16_t(std::clamp(std::lround(value), -32768L, 32767L));
  }
  return out;
}

}  // namespace subtick
