// Autosave (--autosave; src/hooks/autosave.cpp): the game saved when the
// player is back at the garage's top menu after racing, or after a shop or
// car menu.

#pragma once

#include "guest.h"

#include <cstdint>

namespace hooks {

// Main thread, from StateTable::Dispatch's override (src/hooks/title_screen.cpp):
// whether autosave wants each dispatch, and the dispatch just made (`object`
// took message `id` in state `before`; ctx.r3 is its result, left as is).
bool Autosave_Watching();
void Autosave_OnDispatch(PPCContext& ctx, uint8_t* base, uint32_t object, uint32_t id,
                         uint32_t before);

// Main thread. The garage top's load step (sub_8218C240, src/hooks/post_race.cpp)
// with its handler object: runs on every entry to the garage.
void Autosave_OnGarageTopLoadStep(uint32_t garage_top);

// Main thread. A race's draw step ran (Race::DrawStep, src/subtick/game_messages.cpp).
void Autosave_OnRaceDrawn();

}  // namespace hooks
