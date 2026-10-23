// Tick rate experiments (--tick_rate_60, --tick_rate_half_step): see
// tick_rate.cpp.

#pragma once

namespace rex::memory {
class Memory;
}

namespace tick_rate {

// Halves the fixed step lengths, for --tick_rate_half_step. Call once the XEX
// is loaded, before the game runs.
void PatchStepConstants(rex::memory::Memory* memory);

}  // namespace tick_rate
