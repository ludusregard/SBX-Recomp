// Input sampled between ticks for the cars' two physics steps a tick (--subtick,
// --subtick_input). See input.cpp and "Subtick" in docs/functions.md.

#pragma once

#include "guest.h"

#include <cstdint>
#include <vector>

namespace subtick {

// The analog part of a car's control record, as the record builder
// (sub_8215DBF8) works it out from the action mappers.
struct ControlSample {
  float at = 0;  // when it was taken, from 0 (the last tick) to 1 (this tick)
  int16_t axes[5] = {};  // record +4, +6, +8, +10, +12
  uint32_t presence = 0;  // record word 0's bits 0x4, 0x8 and 0x8000
};

// Offsets of the five axes in a control record, and the word 0 bits that
// say the first, second and fourth are non-zero.
constexpr uint32_t kAxisOffsets[5] = {4, 6, 8, 10, 12};
constexpr uint32_t kPresenceBits = 0x4 | 0x8 | 0x8000;

// Main thread, at the start of a render-only pass: reads the pads and the
// action mappers' analog values again (their digital state is put back, so
// presses and repeats still count once a tick) and takes a sample for each
// player. `progress` is how far the pass is between the last tick and the
// next.
void Input_OnRenderOnlyPass(PPCContext& ctx, uint8_t* base, float progress);

// Main thread, after a logic pass: the last sample carries over to the
// next tick as its first.
void Input_OnLogicPassFinished();

// The one input a tick's physics uses, weighted from its samples (oldest
// first; the first is last tick's and gets no weight). See input_weights.cpp.
ControlSample Input_Aggregate(const std::vector<ControlSample>& samples);

// Reads a control record's axes and presence bits into a sample, and writes
// a sample's into a record (the rest of the record is left alone).
ControlSample Input_ReadSample(uint8_t* base, uint32_t record);
void Input_WriteSample(uint8_t* base, uint32_t record, const ControlSample& sample);

}  // namespace subtick
