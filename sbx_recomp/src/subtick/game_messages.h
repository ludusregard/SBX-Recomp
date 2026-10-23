// The game's message queues (GameMessages::Dispatch, sub_82106478).
//
// Each main-loop pass, game logic posts messages that a dispatcher delivers
// to every registered handler. Message 0 is Update (all game logic) and
// message 1 is Draw (the draw step that records the scene). See "Subtick"
// in docs/functions.md.
//
// On a render-only pass (--subtick, races only), Update messages are
// consumed without being delivered, so the pass draws the scene without
// advancing the game. Menus record their UI and act on input during Update,
// so they keep the game's 30 fps.

#pragma once

#include <cstdint>

namespace subtick {

// Main thread, when a main-loop pass's drawing is done (in WaitForFrame).
void GameMessages_OnPassFinished();

// Main thread: whether the coming main-loop pass is render-only.
void GameMessages_SetRenderOnlyPass(bool render_only);
bool GameMessages_IsRenderOnlyPass();

// Main thread: whether the last logic pass was in a race (ran the race's
// draw step), where render-only passes are used.
bool GameMessages_InRace();

// Which main loop pass this is (counts up by one per pass).
uint64_t GameMessages_PassNumber();


}  // namespace subtick
