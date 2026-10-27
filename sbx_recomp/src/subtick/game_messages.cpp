#include "subtick/game_messages.h"

#include "debug/frame_state_diff.h"
#include "guest.h"
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
#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

REXCVAR_DECLARE(bool, subtick);
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

constexpr uint32_t kDelegateInvoke = 0x8216F0F8;
constexpr uint32_t kMemberDelegateInvoke = 0x8222E468;

enum MessageId : uint32_t {
  kUpdate = 0,
  kDraw = 1,
};

bool g_render_only_pass = false;
std::thread::id g_main_thread{};  // the thread passes are paced on
bool g_race_draw_step_seen = false;  // during the current logic pass
bool g_in_race = false;              // as of the last logic pass
uint64_t g_passes = 0;
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
void Deliver(PPCContext& ctx, uint8_t* base, uint32_t dispatcher, uint32_t message) {
  uint32_t handler_count = guest::LoadU32(base, dispatcher + kHandlerCountOffset);
  for (uint32_t i = 0; i < handler_count; i++) {
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
    if (REXCVAR_GET(log_message_handlers) && !g_render_only_pass) {
      g_pass_handlers.emplace_back(message, target);
    }
    ctx.r3.u64 = object;
    ctx.r4.u64 = dispatcher;
    ctx.lr = kHandlerCallReturn;
    debug::FrameStateDiff_Before(base);
    REX_CALL_INDIRECT_FUNC(method);
    debug::FrameStateDiff_After(base, "message handler", target);
  }
}

void DispatchMessages(PPCContext& ctx, uint8_t* base, uint32_t dispatcher) {
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
    if (guest::LoadU32(base, message) == kUpdate && g_render_only_pass &&
        std::this_thread::get_id() == g_main_thread) {
      continue;  // render-only: the game doesn't advance
    }
    Deliver(ctx, base, dispatcher, message);
  }
}

}  // namespace

void GameMessages_OnPassFinished() {
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
}

void GameMessages_SetRenderOnlyPass(bool render_only) {
  g_main_thread = std::this_thread::get_id();
  g_passes++;
  g_render_only_pass = render_only;
  g_pass_any_thread.store(g_passes * 2 + (render_only ? 1 : 0), std::memory_order_relaxed);
}

bool GameMessages_IsRenderOnlyPass() {
  return g_render_only_pass;
}

bool GameMessages_InRace() {
  return g_in_race;
}

uint64_t GameMessages_PassNumber() {
  return g_passes;
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
