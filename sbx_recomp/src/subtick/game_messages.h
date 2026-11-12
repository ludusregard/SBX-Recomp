// The game's message queues (GameMessages::Dispatch, sub_82106478).
//
// Each main-loop pass, game logic posts messages that a dispatcher delivers
// to every registered handler. Message 0 is Update (all game logic) and
// message 1 is Draw (the draw step that records the scene). See "Subtick"
// in docs/functions.md.
//
// On a render-only pass in a race (--subtick), Update messages are consumed
// without being delivered, so the pass draws the scene without advancing
// the game. In the menus no message is delivered: their
// state tables tick on message 1 (the race draws there), and their UI movies
// are drawn again instead (game_messages.cpp).

#pragma once

#include <cstdint>

namespace subtick {

// Main thread, when a main-loop pass's drawing is done (in WaitForFrame).
void GameMessages_OnPassFinished(uint8_t* base);

// Main thread: whether the coming main-loop pass is render-only.
void GameMessages_SetRenderOnlyPass(bool render_only);
bool GameMessages_IsRenderOnlyPass();

// Main thread: whether the last logic pass was in a race (ran the race's
// draw step), where render-only passes are used.
bool GameMessages_InRace();

// Main thread: whether a message is being delivered right now.
bool GameMessages_Delivering();

// Main thread: the message handler being delivered to (its target, as
// --log_message_handlers names it), or 0 outside a delivery.
uint32_t GameMessages_HandlerTarget();

// Main thread: out of a race, and the scene's mode the same for the last
// two logic passes (the menus draw between ticks only then: a mode
// change sets up over its first ticks).
bool GameMessages_MenusSettled();

// Which main loop pass this is (counts up by one per pass).
uint64_t GameMessages_PassNumber();

// The latest logic pass's number (GameMessages_PassNumber's count).
uint64_t GameMessages_LastLogicPass();

// Any thread, for diagnostics: the current pass's number and type as of its
// start, and whether the caller is the main thread.
void GameMessages_PassFromAnyThread(uint64_t& number, bool& render_only, bool& main_thread);


}  // namespace subtick
