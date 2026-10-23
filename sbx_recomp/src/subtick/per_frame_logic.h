// Game logic the game runs from Draw (or during Update along with drawing),
// kept to once a tick with --subtick. See per_frame_logic.cpp.

#pragma once

#include "guest.h"

namespace subtick {

// Main thread, just before Race::DrawStep on every pass in a race (after
// pass 0 is built for the frame): records the race overlays (the marker
// above opponents, the light trails) if they were updated on the latest
// logic pass.
void PerFrameLogic_RecordRaceOverlays(PPCContext& ctx, uint8_t* base);

}  // namespace subtick
