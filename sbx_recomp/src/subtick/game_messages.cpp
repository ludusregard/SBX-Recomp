#include "subtick/game_messages.h"

#include "debug/frame_state_diff.h"
#include "guest.h"
#include "hooks/autosave.h"
#include "hooks/title_screen.h"
#include "subtick/car_transforms.h"
#include "subtick/per_frame_logic.h"

#ifdef SBX_VULKAN_RENDERER
#include "native/textures.h"
#endif

#include <fmt/format.h>
#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <map>
#include <cstring>
#include <string>
#include <thread>
#include <utility>
#include <vector>

REXCVAR_DECLARE(bool, subtick);
REXCVAR_DECLARE(bool, redraw_rival_logo);
REXCVAR_DEFINE_BOOL(log_message_handlers, false, "SBX",
                    "Diagnostic: log the Update and Draw message handlers a logic pass "
                    "delivers to whenever they change (entering a screen: finds its draw step, "
                    "as Race::DrawStep is the race's)");

namespace subtick {

namespace {

// The dispatcher object (r3 of sub_82106478).
constexpr uint32_t kHandlersOffset = 0;  // array of handler pointers
constexpr uint32_t kHandlerCountOffset = 200;
constexpr uint32_t kQueueOffset = 204;  // ring of messages
constexpr uint32_t kQueueCountOffset = 804;
constexpr uint32_t kQueueReadIndexOffset = 808;
constexpr uint32_t kQueueCapacity = 50;
constexpr uint32_t kMessageBytes = 12;  // id and two arguments

// Return addresses of the dispatcher's two calls, as the original sets them.
constexpr uint32_t kPickHandlerReturn = 0x82106500;
constexpr uint32_t kHandlerCallReturn = 0x82106518;

// The rival intro cutscene's Update handler (per_frame_logic.cpp): the one
// Update render-only passes still deliver, to redraw its logo.
constexpr uint32_t kRivalCutsceneHandler = 0x82157948;

constexpr uint32_t kDelegateInvoke = 0x8216F0F8;
constexpr uint32_t kMemberDelegateInvoke = 0x8222E468;

enum MessageId : uint32_t {
  kUpdate = 0,
  kDraw = 1,
};

bool g_render_only_pass = false;
bool g_delivering = false;           // main thread, during a message's delivery
uint32_t g_handler_target = 0;       // the handler being delivered to (0 outside delivery)
uint64_t g_ui_redrawn_pass = ~0ull;  // the render-only pass the menus' UI was redrawn on
// The menus' logic passes in a row with the same scene mode (Scene::Draw's
// +64), out of a race: render-only passes wait for 2 after any change.
uint32_t g_scene_mode = 0;             // as the latest Scene::Draw saw it
uint32_t g_settled_mode = 0;           // as of the last logic pass
uint32_t g_menu_settled_passes = 0;
constexpr uint32_t kMenuSettledPasses = 2;

// The menus' state tables (StateTable, docs/menus.md): entries of 48 bytes
// from +0 (key: state +0, message id +4), the count at +9600, the state at
// +9604. A menu closing waits in a state whose only entry for it is message
// 3 (the garage's state 8: its "closed" step). With frames between ticks
// that wait sometimes never ended (garage to course select or tuning shop,
// John's tests 2026-11-11, until the flag for frames between ticks in the menus was turned off), so while
// any handler waits so, the menus run at 30 Hz, as for a mode change. A
// handler kept in such a state for over 10 s is taken as resting there.
constexpr uint32_t kStateEntryBytes = 48;
constexpr uint32_t kStateCount = 9600;
constexpr uint32_t kStateCurrent = 9604;
constexpr uint32_t kMaxStateEntries = 200;
constexpr uint32_t kClosedMessage = 3;
constexpr uint32_t kRestingPasses = 300;
struct HandlerState {
  uint32_t state = ~0u;
  uint32_t passes = 0;  // logic passes in it
};
std::map<uint32_t, HandlerState> g_handler_states;  // by handler object
std::vector<uint32_t> g_pass_handler_objects;       // delivered to this logic pass
bool g_menu_waiting = false;                        // as of the last logic pass
std::thread::id g_main_thread{};  // the thread passes are paced on
bool g_race_draw_step_seen = false;  // during the current logic pass
bool g_in_race = false;              // as of the last logic pass
uint64_t g_passes = 0;
uint64_t g_logic_pass = 0;  // the latest logic pass's number
// For other threads (the native renderer's diagnostics): the pass as of its
// start, packed as number * 2 + render-only.
std::atomic<uint64_t> g_pass_any_thread{0};
// --log_message_handlers: (message, target) delivered this logic pass, and
// the set last logged.
std::vector<std::pair<uint32_t, uint32_t>> g_pass_handlers, g_logged_handlers;

void LogHandlersIfChanged() {
  std::sort(g_pass_handlers.begin(), g_pass_handlers.end());
  g_pass_handlers.erase(std::unique(g_pass_handlers.begin(), g_pass_handlers.end()),
                        g_pass_handlers.end());
  if (!g_pass_handlers.empty() && g_pass_handlers != g_logged_handlers) {
    std::string update, draw;
    for (auto [message, target] : g_pass_handlers) {
      std::string& list = message == kDraw ? draw : update;
      list += fmt::format(" {:08X}", target);
    }
    REXLOG_INFO("message handlers (pass {}): Update:{}; Draw:{}", g_passes,
                update.empty() ? " none" : update, draw.empty() ? " none" : draw);
    g_logged_handlers = g_pass_handlers;
  }
  g_pass_handlers.clear();
}

// Delivers one message to each handler.
// only_target, when set, delivers to that handler alone.
void Deliver(PPCContext& ctx, uint8_t* base, uint32_t dispatcher, uint32_t message,
             uint32_t only_target = 0) {
  uint32_t handler_count = guest::LoadU32(base, dispatcher + kHandlerCountOffset);
  const bool note_objects = !g_render_only_pass && !g_in_race &&
                            std::this_thread::get_id() == g_main_thread;
  for (uint32_t i = 0; i < handler_count; i++) {
    if (note_objects) {
      g_pass_handler_objects.push_back(guest::LoadU32(base, dispatcher + kHandlersOffset + 4 * i));
    }
    // sub_82106410 picks the object that handles this message for handler
    // i; its first virtual method takes it.
    ctx.r3.u64 = guest::LoadU32(base, dispatcher + kHandlersOffset + 4 * i);
    ctx.r4.u64 = message;
    ctx.lr = kPickHandlerReturn;
    sub_82106410(ctx, base);
    uint32_t object = guest::LoadU32(base, ctx.r3.u32);
    uint32_t method = guest::LoadU32(base, guest::LoadU32(base, object));
    // Delegates (Delegate::Invoke, MemberDelegate::Invoke) keep their real
    // target at +8: named by it for --find_per_frame_state.
    uint32_t target = (method == kDelegateInvoke || method == kMemberDelegateInvoke)
                          ? guest::LoadU32(base, object + 8)
                          : method;
    if (only_target != 0 && target != only_target) {
      continue;
    }
    if (REXCVAR_GET(log_message_handlers) && !g_render_only_pass) {
      g_pass_handlers.emplace_back(guest::LoadU32(base, message), target);
    }
    ctx.r3.u64 = object;
    ctx.r4.u64 = dispatcher;
    ctx.lr = kHandlerCallReturn;
    debug::FrameStateDiff_Before(base);
    const uint32_t outer_target = g_handler_target;
    g_handler_target = target;
    REX_CALL_INDIRECT_FUNC(method);
    g_handler_target = outer_target;
    debug::FrameStateDiff_After(base, "message handler", target);
  }
}

// A render-only pass in the menus. The menus' state
// tables use the messages differently from the race's (0 their update and
// UI movies, 1 their tick: set-up, loading, timers, sounds; the race draws on
// 1), so none is delivered. The pass's own Update and Draw messages are
// consumed, as the logic pass after it posts its own; any other message
// (an event a tick handler posted) stays queued for that logic pass. The UI
// movies the last logic pass's messages drew are drawn again in their place
// (per_frame_logic.cpp).
void DispatchMenuRenderOnly(PPCContext& ctx, uint8_t* base, uint32_t dispatcher) {
  std::vector<std::array<uint32_t, 3>> kept;
  while (guest::LoadU32(base, dispatcher + kQueueCountOffset) != 0) {
    uint32_t count = guest::LoadU32(base, dispatcher + kQueueCountOffset);
    uint32_t read = guest::LoadU32(base, dispatcher + kQueueReadIndexOffset);
    guest::StoreU32(base, dispatcher + kQueueCountOffset, count - 1);
    guest::StoreU32(base, dispatcher + kQueueReadIndexOffset, read + 1);
    uint32_t slot = dispatcher + kQueueOffset + (read % kQueueCapacity) * kMessageBytes;
    std::array<uint32_t, 3> message{};
    for (uint32_t k = 0; k < 3; k++) {
      message[k] = guest::LoadU32(base, slot + 4 * k);
    }
    if (message[0] != kUpdate && message[0] != kDraw) {
      kept.push_back(message);
    }
  }
  for (const auto& message : kept) {  // back in the queue, in order
    uint32_t count = guest::LoadU32(base, dispatcher + kQueueCountOffset);
    uint32_t read = guest::LoadU32(base, dispatcher + kQueueReadIndexOffset);
    uint32_t slot = dispatcher + kQueueOffset + ((read + count) % kQueueCapacity) * kMessageBytes;
    for (uint32_t k = 0; k < 3; k++) {
      guest::StoreU32(base, slot + 4 * k, message[k]);
    }
    guest::StoreU32(base, dispatcher + kQueueCountOffset, count + 1);
  }
  if (g_ui_redrawn_pass != g_passes) {
    g_ui_redrawn_pass = g_passes;
    // The redraw calls what the logic pass's messages called, and those can
    // post messages (a UI movie's animation ending, a menu stepping on):
    // posted again, the next logic pass took them a second time, and a
    // garage sub-menu's opening could wait on them for good (John's test,
    // 2026-11-11: garage to course select or tuning shop hung until
    // frames between ticks in the menus were turned off). The queue is put
    // back as it was.
    constexpr uint32_t kQueueBytes = kQueueReadIndexOffset + 4 - kQueueOffset;
    std::array<uint8_t, kQueueBytes> queue;
    std::memcpy(queue.data(), base + dispatcher + kQueueOffset, kQueueBytes);
    const uint32_t count = guest::LoadU32(base, dispatcher + kQueueCountOffset);
    PerFrameLogic_RedrawMenuUiMovies(ctx, base);
    const int32_t posted = int32_t(guest::LoadU32(base, dispatcher + kQueueCountOffset) - count);
    if (posted != 0 && REXCVAR_GET(log_message_handlers)) {
      REXLOG_INFO("message handlers (pass {}): the menus' redraw changed the queue by {} "
                  "message(s); put back",
                  g_passes, posted);
    }
    std::memcpy(base + dispatcher + kQueueOffset, queue.data(), kQueueBytes);
  }
}

// Whether a handler waits in a closing state (see kClosedMessage).
bool WaitsForClosedMessage(uint8_t* base, uint32_t object, uint32_t state) {
  const uint32_t count = std::min(guest::LoadU32(base, object + kStateCount), kMaxStateEntries);
  bool closed = false;
  for (uint32_t i = 0; i < count; i++) {
    const uint32_t entry = object + i * kStateEntryBytes;
    if (guest::LoadU32(base, entry) != state) {
      continue;
    }
    const uint32_t message = guest::LoadU32(base, entry + 4);
    if (message == kUpdate || message == kDraw) {
      return false;
    }
    closed = closed || message == kClosedMessage;
  }
  return closed;
}

void NoteMenuWaits(uint8_t* base) {
  std::sort(g_pass_handler_objects.begin(), g_pass_handler_objects.end());
  g_pass_handler_objects.erase(
      std::unique(g_pass_handler_objects.begin(), g_pass_handler_objects.end()),
      g_pass_handler_objects.end());
  bool waiting = false;
  uint32_t waiting_object = 0, waiting_state = 0;
  for (uint32_t object : g_pass_handler_objects) {
    if (object == 0) {
      continue;
    }
    HandlerState& handler = g_handler_states[object];
    const uint32_t state = guest::LoadU32(base, object + kStateCurrent);
    if (state != handler.state) {
      handler.state = state;
      handler.passes = 0;
    } else if (handler.passes < kRestingPasses) {
      handler.passes++;
    }
    if (handler.passes < kRestingPasses && WaitsForClosedMessage(base, object, state)) {
      waiting = true;
      waiting_object = object;
      waiting_state = state;
    }
  }
  if (waiting != g_menu_waiting && REXCVAR_GET(log_message_handlers)) {
    if (waiting) {
      REXLOG_INFO("message handlers (pass {}): handler {:08X} waits in state {} for message 3; "
                  "the menus run at 30 Hz until it comes",
                  g_passes, waiting_object, waiting_state);
    } else {
      REXLOG_INFO("message handlers (pass {}): no handler waits for message 3", g_passes);
    }
  }
  g_menu_waiting = waiting;
  g_pass_handler_objects.clear();
}

void DispatchMessages(PPCContext& ctx, uint8_t* base, uint32_t dispatcher) {
  const bool main_thread = std::this_thread::get_id() == g_main_thread;
  if (g_render_only_pass && main_thread && !g_in_race) {
    DispatchMenuRenderOnly(ctx, base, dispatcher);
    return;
  }
  // The original's frame (stwu r1,-144(r1)), with the message copied to +80.
  guest::StackFrame frame(ctx, 144);
  uint32_t message = frame.Local(80);
  while (guest::LoadU32(base, dispatcher + kQueueCountOffset) != 0) {
    // Pop the oldest message into the local copy.
    uint32_t count = guest::LoadU32(base, dispatcher + kQueueCountOffset);
    uint32_t read = guest::LoadU32(base, dispatcher + kQueueReadIndexOffset);
    guest::StoreU32(base, dispatcher + kQueueCountOffset, count - 1);
    guest::StoreU32(base, dispatcher + kQueueReadIndexOffset, read + 1);
    uint32_t slot = dispatcher + kQueueOffset + (read % kQueueCapacity) * kMessageBytes;
    for (uint32_t k = 0; k < kMessageBytes; k += 4) {
      guest::StoreU32(base, message + k, guest::LoadU32(base, slot + k));
    }
    if (guest::LoadU32(base, message) == kUpdate && g_render_only_pass && main_thread) {
      // Render-only: the game doesn't advance. The rival intro cutscene's
      // handler still runs, to redraw its logo (per_frame_logic.cpp).
      if (REXCVAR_GET(redraw_rival_logo)) {
        Deliver(ctx, base, dispatcher, message, kRivalCutsceneHandler);
      }
      continue;
    }
    const bool outer = g_delivering;  // a dispatch from inside a handler
    g_delivering = outer || main_thread;
    Deliver(ctx, base, dispatcher, message);
    g_delivering = outer;
  }
}

}  // namespace

void GameMessages_OnPassFinished(uint8_t* base) {
  if (g_render_only_pass) {
    return;
  }
  if (REXCVAR_GET(log_message_handlers)) {
    LogHandlersIfChanged();
  }
  // The pass that just ran was a logic pass: it decides whether we're in a
  // race.
#ifdef SBX_VULKAN_RENDERER
  if (g_in_race && !g_race_draw_step_seen) {
    native::TextureCache_RaceLeft();  // frees the race's textures
  }
#endif
  g_in_race = g_race_draw_step_seen;
  g_race_draw_step_seen = false;
  if (g_in_race) {
    g_pass_handler_objects.clear();
    g_menu_waiting = false;
  } else {
    NoteMenuWaits(base);
  }
  if (g_in_race || g_scene_mode != g_settled_mode || g_menu_waiting) {
    g_menu_settled_passes = 0;
  } else if (g_menu_settled_passes < kMenuSettledPasses) {
    g_menu_settled_passes++;
  }
  g_settled_mode = g_scene_mode;
}

void GameMessages_SetRenderOnlyPass(bool render_only) {
  g_main_thread = std::this_thread::get_id();
  g_passes++;
  g_render_only_pass = render_only;
  if (!render_only) {
    g_logic_pass = g_passes;
  }
  g_pass_any_thread.store(g_passes * 2 + (render_only ? 1 : 0), std::memory_order_relaxed);
}

bool GameMessages_IsRenderOnlyPass() {
  return g_render_only_pass;
}

bool GameMessages_Delivering() {
  return g_delivering;
}

uint32_t GameMessages_HandlerTarget() {
  return g_handler_target;
}

bool GameMessages_MenusSettled() {
  return !g_in_race && g_menu_settled_passes >= kMenuSettledPasses;
}

bool GameMessages_InRace() {
  return g_in_race;
}

uint64_t GameMessages_PassNumber() {
  return g_passes;
}

uint64_t GameMessages_LastLogicPass() {
  return g_logic_pass;
}

void GameMessages_PassFromAnyThread(uint64_t& number, bool& render_only, bool& main_thread) {
  uint64_t packed = g_pass_any_thread.load(std::memory_order_relaxed);
  number = packed >> 1;
  render_only = packed & 1;
  main_thread = std::this_thread::get_id() == g_main_thread;
}

}  // namespace subtick

// GameMessages::Dispatch: r3 = the dispatcher. Delivers each queued message
// to every handler. Replaced (same steps as the original) whenever its
// handlers need to be known or Update skipped; otherwise the original runs.
extern "C" REX_FUNC(sub_82106478) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(subtick) && !REXCVAR_GET(log_message_handlers)) {
    __imp__sub_82106478(ctx, base);
    return;
  }
  subtick::DispatchMessages(ctx, base, ctx.r3.u32);
}

// Scene::Draw: r3 = the scene (as Scene::Update's). Calls the current mode
// object's (+64) draw, its vtable slot 5. Hooked for --log_message_handlers:
// logs the mode object and that draw whenever they change.
extern "C" REX_FUNC(sub_82098E68) {
  REX_FUNC_PROLOGUE();
  subtick::g_scene_mode = ctx.r3.u32 ? guest::LoadU32(base, ctx.r3.u32 + 64) : 0;
  if (REXCVAR_GET(log_message_handlers)) {
    static uint32_t last_mode = 0, last_draw = 0;
    uint32_t scene = ctx.r3.u32;
    uint32_t mode = scene ? guest::LoadU32(base, scene + 64) : 0;
    uint32_t vtable = mode ? guest::LoadU32(base, mode) : 0;
    // Only a vtable in the image's data is read through.
    uint32_t draw = (vtable >= 0x82000000u && vtable < 0x82A00000u)
                        ? guest::LoadU32(base, vtable + 5 * 4)
                        : 0;
    if (mode != last_mode || draw != last_draw) {
      REXLOG_INFO("message handlers: scene {:08X} mode {:08X} (vtable {:08X}), its draw {:08X}",
                  scene, mode, vtable, draw);
      last_mode = mode;
      last_draw = draw;
    }
  }
  __imp__sub_82098E68(ctx, base);
}

// Race::DrawStep: noted to tell races from menus.
extern "C" REX_FUNC(sub_82159540) {
  REX_FUNC_PROLOGUE();
  subtick::g_race_draw_step_seen = true;
  hooks::TitleScreen_OnRaceDrawn();  // ends --skip_cutscenes's fast menus
  hooks::Autosave_OnRaceDrawn();     // a day of racing, for --autosave
  // The cars' matrices are blended between ticks for the step, so the camera
  // built from them moves with the cars (car_transforms.cpp).
  // The camera passes' view-projections for this frame first (render-only
  // passes skip the Update that builds them: the main camera, the mirror), then the race overlays recorded with it
  // (per_frame_logic.cpp), then the cars blended for the step.
  subtick::CarTransforms_RebuildCamera(ctx, base);
  subtick::PerFrameLogic_RecordRaceOverlays(ctx, base);
  auto restores = subtick::CarTransforms_BlendAll(base);
  subtick::CarTransforms_DrawStepBegin();
  __imp__sub_82159540(ctx, base);
  subtick::CarTransforms_DrawStepEnd();
  subtick::CarTransforms_RestoreAll(base, restores);
}
