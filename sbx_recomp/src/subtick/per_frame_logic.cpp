// Game logic that the game runs from its Draw message (or during Update
// along with drawing), counting frames.
//
// On the 360 every Draw was a 30 Hz tick, so this logic counts ticks; with
// --subtick's render-only passes it would run several times a tick. Each
// case here keeps it to once a tick without changing what's drawn. The
// list follows what ITC-Recomp found in the same game (Import Tuner
// Challenge); more may turn up (anything that runs too fast or flickers
// only above 30 fps).

#include "game/ui_movies.h"
#include "guest.h"
#include "hooks/native_calls.h"
#include "subtick/car_transforms.h"
#include "subtick/frame_pacer.h"
#include "subtick/game_messages.h"
#include "subtick/per_frame_logic.h"

#include <fmt/format.h>
#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

REXCVAR_DECLARE(bool, subtick);

REXCVAR_DEFINE_BOOL(log_race_overlays, false, "SBX",
                    "Diagnostic: log the race overlays' updates and recordings whenever what "
                    "happens to them changes (a few passes either side; 2,000 lines at most): "
                    "which handler ran on each logic pass and whether each pass recorded them");

REXCVAR_DEFINE_BOOL(redraw_rival_logo, true, "SBX",
                    "Draw the rival's logo in the rival intro cutscene on render-only passes "
                    "too (the cutscene still steps once a tick)");

namespace {

bool RenderOnly() {
  return subtick::GameMessages_IsRenderOnlyPass();
}

// Calls whose animation state and drawing are one: on a render-only pass,
// replay the call from the state its object had before the logic pass's same
// call, then put back the state after it. It draws what the logic pass drew,
// and the state advances once a tick. Calls are matched by object and by
// order within the pass (an object can be drawn once per view). If the
// object changed since the logic pass, or the logic pass had no such call,
// the call runs from the object's current state, which is then put back
// (or, with `always_from_logic_pass`, from the logic pass's state before its
// call all the same, if there was such a call).
class ReplayFromLogicPass {
 public:
  explicit ReplayFromLogicPass(uint32_t bytes, bool always_from_logic_pass = false)
      : bytes_(bytes), always_from_logic_pass_(always_from_logic_pass) {}

  // For the next Run on a render-only pass: a call the latest logic pass
  // didn't make (by object and order) is skipped instead of run.
  void SkipUnmatched(bool skip) { skip_unmatched_ = skip; }

  template <typename Call>
  void Run(uint8_t* base, uint32_t object, Call&& call) {
    Run(base, object, call, [] {});
  }

  // `prepare` runs on a render-only pass once the logic pass's state before
  // its call is back in place, just before the call.
  template <typename Call, typename Prepare>
  void Run(uint8_t* base, uint32_t object, Call&& call, Prepare&& prepare) {
    uint64_t pass = subtick::GameMessages_PassNumber();
    if (pass != pass_) {
      pass_ = pass;
      occurrences_.clear();
    }
    Key key{object, occurrences_[object]++};
    uint8_t* state = base + object;
    const bool skip_unmatched = skip_unmatched_;
    skip_unmatched_ = false;
    if (!RenderOnly()) {
      States& states = states_[key];
      states.before.assign(state, state + bytes_);
      call();
      states.after.assign(state, state + bytes_);
      states.logic_pass = pass;
      return;
    }
    auto it = states_.find(key);
    if (skip_unmatched && (it == states_.end() ||
                           it->second.logic_pass != subtick::GameMessages_LastLogicPass())) {
      return;
    }
    if (it == states_.end() || it->second.after.size() != bytes_) {
      CallFromCurrentState(state, call);
      return;
    }
    if (std::memcmp(state, it->second.after.data(), bytes_) != 0) {
      if (always_from_logic_pass_) {
        current_.assign(state, state + bytes_);
        std::memcpy(state, it->second.before.data(), bytes_);
        prepare();
        call();
        std::memcpy(state, current_.data(), bytes_);
        return;
      }
      CallFromCurrentState(state, call);
      return;
    }
    std::memcpy(state, it->second.before.data(), bytes_);
    prepare();
    call();
    std::memcpy(state, it->second.after.data(), bytes_);
  }

 private:
  template <typename Call>
  void CallFromCurrentState(uint8_t* state, Call&& call) {
    current_.assign(state, state + bytes_);
    call();
    std::memcpy(state, current_.data(), bytes_);
  }

  struct Key {
    uint32_t object;
    int occurrence;
    bool operator==(const Key&) const = default;
  };
  struct KeyHash {
    size_t operator()(const Key& k) const {
      return (size_t(k.object) << 8) ^ size_t(k.occurrence);
    }
  };
  struct States {
    std::vector<uint8_t> before, after;
    uint64_t logic_pass = ~0ull;
  };
  uint32_t bytes_;
  bool always_from_logic_pass_;
  bool skip_unmatched_ = false;
  uint64_t pass_ = ~0ull;
  std::unordered_map<Key, States, KeyHash> states_;
  std::unordered_map<uint32_t, int> occurrences_;
  std::vector<uint8_t> current_;
};

// UI movies' state: the furthest field ITC-Recomp found used, ~+0x96C8.
ReplayFromLogicPass g_ui_movies(0xA000);
// Draw-only UI movie updates: nothing to put back, matched only to skip
// those the logic pass didn't make.
ReplayFromLogicPass g_ui_movie_draws(0);
// The car's boost value (car+704), which the draw update lowers.
ReplayFromLogicPass g_boost(4);

constexpr uint32_t kPlayTimeFrames = 0x826B5870;
constexpr uint32_t kMinimapBlinkFrames = 0x826B5784;
constexpr uint32_t kMinimapBlinkPhase = 0x826B5788;
// The camera passes: 21 of 592 bytes.
constexpr uint32_t kCameraPasses = 0x826AA8E0;
constexpr uint32_t kCameraPassesBytes = 21 * 592;
// Car::DrawUpdate's r3 is the car - 4288; the boost is at car+704.
constexpr uint32_t kDrawUpdateBoostOffset = 4288 + 704;
// Car::DrawUpdate's r3 is the car - 4288.
constexpr uint32_t kDrawUpdateCarOffset = 4288;

std::vector<uint8_t> g_overlay_cameras;  // as of the logic pass's overlays update
constexpr uint32_t kCameraPassBytes = 592;  // pass 0, built for each frame

// The race overlays (sub_8215BE00): the list updated on the latest logic
// pass, and that pass's tick.
constexpr uint32_t kOverlayCount = 25;
constexpr uint32_t kOverlayUpdateSlot = 16;  // vtable offset of slot 4
constexpr uint32_t kOverlayRecordSlot = 20;  // vtable offset of slot 5
uint32_t g_overlay_list = 0;
uint64_t g_overlay_tick = ~0ull;
bool g_overlay_paused = false;  // last saved by the paused recording, not by an update

void ForEachOverlay(PPCContext& ctx, uint8_t* base, uint32_t list, uint32_t slot) {
  for (uint32_t i = 0; i < kOverlayCount; i++) {
    uint32_t overlay = guest::LoadU32(base, list + 4 * i);
    if (overlay == 0) {
      continue;
    }
    ctx.r3.u64 = overlay;
    REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, overlay) + slot));
  }
}


}  // namespace

// Blink timers (r3 = a timer set, from the car's draw): four frame counters
// that drive the flashing sprites above cars. Skipped on render-only passes.
extern "C" REX_FUNC(sub_820DC5C8) {
  REX_FUNC_PROLOGUE();
  if (RenderOnly()) {
    return;
  }
  __imp__sub_820DC5C8(ctx, base);
}

// TextureAnimations::Advance (from the draw step part sub_820F5F48): steps
// every flipbook animation (the flashing building lights, road signs).
// Skipped on render-only passes.
extern "C" REX_FUNC(sub_820F1038) {
  REX_FUNC_PROLOGUE();
  if (RenderOnly()) {
    return;
  }
  hooks::NativeCourseScenery_AdvanceTextureAnimations(ctx, base);
}

// Minimap::Draw: after drawing, steps the blink clock at 0x826B5784 (wraps
// after 4, then steps the phase at 0x826B5788). On a render-only pass both
// are put back after the call, so it draws with the logic pass's phase.
extern "C" REX_FUNC(sub_820F5610) {
  REX_FUNC_PROLOGUE();
  if (!RenderOnly()) {
    __imp__sub_820F5610(ctx, base);
    return;
  }
  uint32_t frames = guest::LoadU32(base, kMinimapBlinkFrames);
  uint32_t phase = guest::LoadU32(base, kMinimapBlinkPhase);
  __imp__sub_820F5610(ctx, base);
  guest::StoreU32(base, kMinimapBlinkFrames, frames);
  guest::StoreU32(base, kMinimapBlinkPhase, phase);
}

// sub_82107A38, the game's per-frame logic, from Draw, every pass (on the
// 360, once a 30 Hz tick). Besides drawing it steps game state by one each
// call, which with --subtick's render-only passes ran once a frame:
// - the play time frame counter at 0x826B5870 (sub_82110928 turns it into
//   hours, minutes and seconds at 30 a second);
// - the screen fades (Fade::Update, sub_8209D250): two layers, each with a
//   fade-out count at 0x82694DBC + 4i and a fade-in count at 0x82694DC4 +
//   4i, down by one a call, the fade quad's alpha from them. The race's
//   forced exit waits for the fades (sub_82166940 posts message 49, leaving
//   the race, once all four are 0): at 60 fps it came twice as soon, so the
//   race-end music's step (bgm_tmp_08) was cut short;
// - a countdown at 0x825F180C, down by one a call;
// - the action mappers' update (sub_8210B140, subtick/input.cpp).
// On render-only passes each of these is put back after the call (only a
// step of one: a fade the call starts is kept), so they count ticks; the
// fade is still drawn every frame, at the tick's alpha.
namespace {
constexpr uint32_t kFadeOutCounts = 0x82694DBC;  // 2 layers
constexpr uint32_t kFadeInCounts = 0x82694DC4;   // 2 layers
constexpr uint32_t kFrameCountdown = 0x825F180C;
constexpr std::array<uint32_t, 5> kPerTickCounts = {
    kFadeOutCounts, kFadeOutCounts + 4, kFadeInCounts, kFadeInCounts + 4, kFrameCountdown};
}  // namespace

extern "C" REX_FUNC(sub_82107A38) {
  REX_FUNC_PROLOGUE();
  bool render_only = RenderOnly();
  bool undo = render_only;
  if (undo) {
    guest::StoreU32(base, kPlayTimeFrames, guest::LoadU32(base, kPlayTimeFrames) - 1);
  }
  std::array<uint32_t, kPerTickCounts.size()> before{};
  if (render_only) {
    for (size_t i = 0; i < kPerTickCounts.size(); i++) {
      before[i] = guest::LoadU32(base, kPerTickCounts[i]);
    }
  }
  __imp__sub_82107A38(ctx, base);
  if (render_only) {
    for (size_t i = 0; i < kPerTickCounts.size(); i++) {
      if (guest::LoadU32(base, kPerTickCounts[i]) == before[i] - 1) {
        guest::StoreU32(base, kPerTickCounts[i], before[i]);
      }
    }
  }
}

// UI movies' adding animation tracks, with --subtick (ported from ITC-Recomp,
// its --per_tick_ui_movie_tracks; its docs/pause-menu-fix.md, "The sub-menus'
// arrow"). A track writes one value into its node, outside the bytes the UI
// movie replay restores; a track whose property descriptor's byte +8 is 0
// adds its value instead of setting it. Most nodes are put back to the
// movie's rest pose at the end of the update (UiMovie::RestorePose,
// sub_82119D50), so their adds start again from it every update; skipping
// those on render-only passes would draw the rest pose there. Adds on values
// the update doesn't put back were applied again on every render-only update
// (ITC: at 144 fps the pause sub-menus' arrow slid ~5x as far). So on a logic
// pass each adding track's target is noted as put back or not (its value at
// the end of the update is the one before the track), and on render-only
// passes only adds on targets not seen put back are skipped.
//
// sub_82585C40 (ITC sub_825A0548, the same code but for float constants): r3
// = the node, r4 = the track, whose word +0 is a property id. The property's
// descriptor (from sub_82585560) is entry id of a 12-byte table at
// 0x82973088 (ITC 0x82970C28), filled on first use (byte 0x82973080 set;
// ITC 0x82970C20): +0 the node offset, +4 the type (0 float, 1 and 2
// integer, 3 and up write nothing), byte +8 set (non-zero) or add (0), byte
// +9 indirect. The one word written is node + offset, plus, when indirect,
// 16 * ([node + 112] + 2 * ([track + 12] + 4)).
namespace {

constexpr uint32_t kTrackPropertiesReady = 0x82973080;  // ITC 0x82970C20
constexpr uint32_t kTrackProperties = 0x82973088;       // ITC 0x82970C28
constexpr uint32_t kTrackPropertyCount = 64;

struct AddedTarget {
  uint32_t target, before;
};
// The adding tracks' targets of each UI movie update in progress (updates
// can nest), with their values before the track; null outside an update.
std::deque<std::vector<AddedTarget>> g_added_by_depth;
size_t g_update_depth = 0;
std::vector<AddedTarget>* g_added = nullptr;
// Targets of adding tracks the last logic pass's update put back.
std::unordered_set<uint32_t> g_put_back_targets;

uint32_t TrackTarget(uint8_t* base, uint32_t node, uint32_t track, uint32_t descriptor) {
  uint32_t target = node + guest::LoadU32(base, descriptor);
  if (guest::LoadU8(base, descriptor + 9) != 0) {
    target += 16 * (guest::LoadU32(base, node + 112) + 2 * (guest::LoadU32(base, track + 12) + 4));
  }
  return target;
}

// At the end of a logic pass's update: a target is put back when it holds
// its value from before its first track in the update (walked from the end,
// so that track decides).
void NotePutBackTargets(uint8_t* base, const std::vector<AddedTarget>& added) {
  for (auto it = added.rbegin(); it != added.rend(); ++it) {
    if (guest::LoadU32(base, it->target) == it->before) {
      g_put_back_targets.insert(it->target);
    } else {
      g_put_back_targets.erase(it->target);
    }
  }
}

}  // namespace

extern "C" REX_FUNC(sub_82585C40) {
  REX_FUNC_PROLOGUE();
  if (g_added == nullptr || guest::LoadU8(base, kTrackPropertiesReady) == 0) {
    hooks::NativeUiMovies_ApplyTrack(ctx, base);
    return;
  }
  uint32_t node = ctx.r3.u32, track = ctx.r4.u32;
  uint32_t property = guest::LoadU32(base, track);
  uint32_t descriptor = kTrackProperties + 12 * property;
  bool adds = property < kTrackPropertyCount && guest::LoadU8(base, descriptor + 8) == 0;
  uint32_t type = property < kTrackPropertyCount ? guest::LoadU32(base, descriptor + 4) : 3;
  if (!adds || type >= 3) {
    hooks::NativeUiMovies_ApplyTrack(ctx, base);
    return;
  }
  uint32_t target = TrackTarget(base, node, track, descriptor);
  if (RenderOnly()) {
    if (!g_put_back_targets.contains(target)) {
      return;  // an add the update wouldn't undo: once a tick
    }
  } else {
    g_added->push_back({target, guest::LoadU32(base, target)});
  }
  hooks::NativeUiMovies_ApplyTrack(ctx, base);
}

// A UI movie drawn on a render-only pass from its state
// before the logic pass's update, with its running animations moved on by
// the part of a step the frame is through the tick (the update steps their
// time by kUiMovieStep a tick, in the 60ths of a second its keys use).
// Clamped to the animation, as the update clamps a queued one; a looping
// one isn't wrapped early.
namespace {
constexpr float kUiMovieStep = 2.0f;  // UiMovie::Update's step (0x820021A8)

void MoveOn(game::AnimationSlot* slot, float amount) {
  if (slot->hold != 0) {
    return;
  }
  float time = slot->time;
  time = slot->backwards != 0 ? time - amount : time + amount;
  const float length = slot->animation->length;
  slot->time = time < 0.0f ? 0.0f : time > length ? length : time;
}

void BlendUiMovie(uint8_t* base, uint32_t movie_address) {
  game::UiMovie* movie = guest::At<game::UiMovie>(movie_address);
  const float amount = kUiMovieStep * subtick::GetFramePacer().FrameProgress();
  for (int32_t i = 0; i < movie->queue.count; i++) {
    game::AnimationSlot* slot = movie->queue.ItemAtPosition(uint32_t(i));
    MoveOn(slot, amount);
    if (slot->with_next == 0) {
      break;  // the update steps no further
    }
  }
  for (int32_t i = 0; i < movie->looping.count; i++) {
    MoveOn(movie->looping.ItemAtPosition(uint32_t(i)), amount);
  }
}

// In the menus: the drawing the last logic pass did while delivering its
// messages (the menus draw there), in order, to draw again on render-only
// passes (PerFrameLogic_RedrawMenuUiMovies): the top-level UI movie updates,
// the garage stage's steps (the car on its turntable and the camera around
// it) and the parking area's scene.
struct MenuDrawCall {
  enum Kind { kUiMovie, kGarageStage, kParkingArea };
  Kind kind;
  uint32_t r3, r4, r5;
};
std::vector<MenuDrawCall> g_menu_draw_calls;
uint64_t g_menu_draw_pass = ~0ull;
bool g_menu_redrawing = false;  // during PerFrameLogic_RedrawMenuUiMovies

// Notes a drawing call made by the logic pass's messages in the menus.
void NoteMenuDrawCall(MenuDrawCall call) {
  const uint64_t pass = subtick::GameMessages_PassNumber();
  if (pass != g_menu_draw_pass) {
    g_menu_draw_pass = pass;
    g_menu_draw_calls.clear();
  }
  g_menu_draw_calls.push_back(call);
}

void RedrawParkingArea(PPCContext& ctx, uint8_t* base, uint32_t area);

bool NotingMenuDrawCalls() {
  return REXCVAR_GET(subtick) && !RenderOnly() &&
         subtick::GameMessages_Delivering() && !subtick::GameMessages_InRace();
}
}  // namespace

void subtick::PerFrameLogic_RedrawMenuUiMovies(PPCContext& ctx, uint8_t* base) {
  if (g_menu_draw_pass != subtick::GameMessages_LastLogicPass()) {
    return;  // the logic pass's messages drew nothing
  }
  const uint64_t r3 = ctx.r3.u64, r4 = ctx.r4.u64, r5 = ctx.r5.u64;
  g_menu_redrawing = true;
  for (const MenuDrawCall& call : g_menu_draw_calls) {
    ctx.r3.u64 = call.r3;
    ctx.r4.u64 = call.r4;
    ctx.r5.u64 = call.r5;
    // The wrappers below: replayed from the logic pass.
    if (call.kind == MenuDrawCall::kUiMovie) {
      sub_8211D4B8(ctx, base);
    } else if (call.kind == MenuDrawCall::kGarageStage) {
      sub_8217DD48(ctx, base);
    } else {
      RedrawParkingArea(ctx, base, call.r3);
    }
  }
  g_menu_redrawing = false;
  ctx.r3.u64 = r3;
  ctx.r4.u64 = r4;
  ctx.r5.u64 = r5;
}

// AudioManager::PlayCue (sub_821196D0; from its vtable's 2D and 3D cue
// slots, sub_82119808 and sub_821197D8: the vtable is 0x82008318, the
// AudioManager's first, so MenuSound::Play's slot 10 is sub_82119808). The
// drawing the menus redraw between ticks can start sounds the logic pass's
// call already started (the garage car's door and hood opening): none is
// started while redrawing.
extern "C" REX_FUNC(sub_821196D0) {
  REX_FUNC_PROLOGUE();
  if (g_menu_redrawing) {
    ctx.r3.u64 = 0;
    return;
  }
  __imp__sub_821196D0(ctx, base);
}

// UI movie update: r3 = the movie, r4 = 0 advances a frame and draws, 1
// only draws. Advancing is replayed from the logic pass on render-only
// passes (the HUD; the race start countdown is a 240-frame movie). Each
// update's adding tracks are noted (above).
extern "C" REX_FUNC(sub_8211D4B8) {
  REX_FUNC_PROLOGUE();
  bool tracks = REXCVAR_GET(subtick);
  std::vector<AddedTarget>* added_before = g_added;
  if (tracks) {
    if (g_update_depth == g_added_by_depth.size()) {
      g_added_by_depth.emplace_back();
    }
    g_added = &g_added_by_depth[g_update_depth++];
    g_added->clear();
  }
  if (tracks && g_update_depth == 1 && NotingMenuDrawCalls()) {
    NoteMenuDrawCall({MenuDrawCall::kUiMovie, ctx.r3.u32, ctx.r4.u32, 0});
  }
  // In a race, a render-only pass draws only the UI movies the tick's logic
  // pass drew: the HUD drew again for a frame after the race's result had
  // taken it down (John's tests, 2026-11-11).
  const bool race = subtick::GameMessages_InRace();
  if (!REXCVAR_GET(subtick)) {
    hooks::NativeUiMovies_Update(ctx, base);
  } else if (ctx.r4.u32 != 0) {
    g_ui_movie_draws.SkipUnmatched(race);
    g_ui_movie_draws.Run(base, ctx.r3.u32, [&] { hooks::NativeUiMovies_Update(ctx, base); });
  } else {
    const uint32_t movie = ctx.r3.u32;
    g_ui_movies.SkipUnmatched(race);
    g_ui_movies.Run(
        base, movie, [&] { hooks::NativeUiMovies_Update(ctx, base); },
        [&] { BlendUiMovie(base, movie); });
  }
  if (tracks) {
    if (!RenderOnly()) {
      NotePutBackTargets(base, *g_added);
    }
    g_update_depth--;
  }
  g_added = added_before;
}

// The garage stage's step (sub_8217DD48): r3 the stage's holder (+0 the
// stage, 6208 bytes), r5 whether to keep the camera's pitch in range. Moves
// the camera around the car and turns the car, then poses and draws the car
// on its stage. The garage and its shops call it from their messages, once a
// tick. It's called again on render-only passes in the menus
// (PerFrameLogic_RedrawMenuUiMovies), from the stage as it was before the
// logic pass's call, and the stage is put back after: the camera passes it
// updates and the car are blended between ticks (car_transforms.cpp).
namespace {
constexpr uint32_t kGarageStageBytes = 6208;
ReplayFromLogicPass g_garage_stage(kGarageStageBytes, true);

bool GarageStageBlended() {
  return REXCVAR_GET(subtick) &&
         !subtick::GameMessages_InRace();
}
}  // namespace

extern "C" REX_FUNC(sub_8217DD48) {
  REX_FUNC_PROLOGUE();
  const uint32_t stage = guest::LoadU32(base, ctx.r3.u32);
  if (!GarageStageBlended() || stage == 0) {
    __imp__sub_8217DD48(ctx, base);
    return;
  }
  if (g_update_depth == 0 && NotingMenuDrawCalls()) {
    NoteMenuDrawCall({MenuDrawCall::kGarageStage, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32});
  }
  subtick::CarTransforms_MenuStageBegin();
  g_garage_stage.Run(base, stage, [&] { __imp__sub_8217DD48(ctx, base); });
  subtick::CarTransforms_MenuStageEnd();
}

// The parking area's scene (sub_82225DE0, r3 the area, [0x8280DE60]): sets
// its camera (from the area's +592 eye and +608 target, set once a tick) and
// draws its scenery. Its handler (sub_8222C840, message 0) then draws the
// cars on it: the draw object's +5004 set, sub_82225918, sub_820C93A8 and
// sub_820C5BA0 (the draw object, 0). In the menus the scene and
// the cars are drawn again on render-only passes
// (PerFrameLogic_RedrawMenuUiMovies), the camera blended between ticks as
// the garage stage's is. Nothing in it steps once a tick (the scenery's
// animations are kept to once a tick by ModelAnimations::Apply below).
namespace {
constexpr uint32_t kDrawObject = 0x826B2B24;  // holds the draw object
constexpr uint32_t kDrawObjectCarsShown = 5004;

void RedrawParkingArea(PPCContext& ctx, uint8_t* base, uint32_t area) {
  ctx.r3.u64 = area;
  sub_82225DE0(ctx, base);  // the wrapper below
  const uint32_t draw = guest::LoadU32(base, kDrawObject);
  if (draw == 0) {
    return;
  }
  guest::StoreU32(base, draw + kDrawObjectCarsShown, 1);
  sub_82225918(ctx, base);
  sub_820C93A8(ctx, base);
  ctx.r3.u64 = draw;
  ctx.r4.u64 = 0;
  sub_820C5BA0(ctx, base);
}
}  // namespace

extern "C" REX_FUNC(sub_82225DE0) {
  REX_FUNC_PROLOGUE();
  if (!GarageStageBlended()) {
    __imp__sub_82225DE0(ctx, base);
    return;
  }
  if (g_update_depth == 0 && NotingMenuDrawCalls()) {
    NoteMenuDrawCall({MenuDrawCall::kParkingArea, ctx.r3.u32, 0, 0});
  }
  subtick::CarTransforms_MenuStageBegin();
  __imp__sub_82225DE0(ctx, base);
  subtick::CarTransforms_MenuStageEnd();
}

// Scrolling text (the garage's ticker): a UI movie keeps a record per
// scrolling text, found by the text's id in its node map (movie+39884,
// sub_8211B4E0): +4 set, +8 a frame count, +12 the frames to wait before
// scrolling, +16 the speed. TickerText::Offset (sub_8211B9D0, r3 the movie,
// r4 the text's draw: +0 its x, +8 its width, +120 its id) returns the x it
// is drawn at, from the frame count; TickerText::Step (sub_8211BB10, r3 the
// movie, r4 the id) counts a frame. The movie's queued text draw
// (sub_8211CF58) steps it after every draw that isn't draw-only, so with
// the menus' UI movies drawn again between ticks it scrolled once a frame.
// On render-only passes the step is skipped, and the x is taken between the
// last frame count's and the one before by the frame's progress through the
// tick (not across the text starting over: a jump of more than 64).
namespace {
constexpr uint32_t kMovieNodeMap = 39884;
constexpr uint32_t kTickerTextId = 120;
constexpr uint32_t kTickerFrames = 8;
constexpr float kTickerJump = 64.0f;

uint32_t TickerRecord(PPCContext& ctx, uint8_t* base, uint32_t movie, uint32_t id) {
  const uint64_t r3 = ctx.r3.u64, r4 = ctx.r4.u64, lr = ctx.lr;
  uint32_t slot;
  {
    guest::StackFrame frame(ctx, 112);
    const uint32_t key = frame.Local(80);
    guest::StoreU32(base, key, id);
    ctx.r3.u64 = movie + kMovieNodeMap;
    ctx.r4.u64 = key;
    sub_8211B4E0(ctx, base);
    slot = ctx.r3.u32;
  }
  ctx.r3.u64 = r3;
  ctx.r4.u64 = r4;
  ctx.lr = lr;
  return slot ? guest::LoadU32(base, slot) : 0;
}
}  // namespace

extern "C" REX_FUNC(sub_8211BB10) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(subtick) && RenderOnly()) {
    return;  // counted once a tick, on logic passes
  }
  __imp__sub_8211BB10(ctx, base);
}

extern "C" REX_FUNC(sub_8211B9D0) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(subtick) || !RenderOnly()) {
    __imp__sub_8211B9D0(ctx, base);
    return;
  }
  const uint64_t r3 = ctx.r3.u64, r4 = ctx.r4.u64;
  const uint32_t record =
      TickerRecord(ctx, base, ctx.r3.u32, guest::LoadU32(base, ctx.r4.u32 + kTickerTextId));
  if (record == 0) {
    __imp__sub_8211B9D0(ctx, base);
    return;
  }
  const uint32_t frames = guest::LoadU32(base, record + kTickerFrames);
  __imp__sub_8211B9D0(ctx, base);
  const double now = ctx.f1.f64;
  if (frames == 0) {
    return;
  }
  // The x one frame count earlier: what the logic pass drew.
  guest::StoreU32(base, record + kTickerFrames, frames - 1);
  ctx.r3.u64 = r3;
  ctx.r4.u64 = r4;
  __imp__sub_8211B9D0(ctx, base);
  const double before = ctx.f1.f64;
  guest::StoreU32(base, record + kTickerFrames, frames);
  const float t = std::clamp(subtick::GetFramePacer().FrameProgress(), 0.0f, 1.0f);
  ctx.f1.f64 = std::fabs(now - before) > kTickerJump ? now : before + (now - before) * t;
}

// A scene model's animations (sub_8257A460, r3 the model; from
// Scenery::Draw, every pass): two lists, at +304 (16-byte entries, the time
// at +12) and +364 (32-byte entries, the time at +16); each list: +0 set,
// +4 its count's address, +8 the clips (80 bytes each: start +72, end +76),
// +16 the entries. Each animation is applied at its time, then the time
// steps one frame (wrapping at the clip's end): the course's moving
// textures (scrolling signs and the like). On a render-only pass each is
// applied at the logic pass's time plus the frame's progress through the
// tick (held at the clip's end), and the times are put back after: they
// step once a tick, and move smoothly between. A model the logic pass
// didn't step, or that changed since, is applied at its time as it is.
namespace {
struct AnimationListLayout {
  uint32_t offset, entry_bytes, time;
};
constexpr AnimationListLayout kModelAnimationLists[] = {{304, 16, 12}, {364, 32, 16}};
constexpr uint32_t kAnimationListSet = 0;
constexpr uint32_t kAnimationListCount = 4;
constexpr uint32_t kAnimationListClips = 8;
constexpr uint32_t kAnimationListEntries = 16;
constexpr uint32_t kAnimationClipBytes = 80;
constexpr uint32_t kAnimationClipEnd = 76;

// The addresses of a model's animation times, and their clips' ends.
void ModelAnimationTimes(uint8_t* base, uint32_t model, std::vector<uint32_t>& times,
                         std::vector<float>& ends) {
  times.clear();
  ends.clear();
  for (const AnimationListLayout& layout : kModelAnimationLists) {
    const uint32_t list = model + layout.offset;
    if (guest::LoadU32(base, list + kAnimationListSet) == 0) {
      continue;
    }
    const uint32_t count = guest::LoadU32(base, guest::LoadU32(base, list + kAnimationListCount));
    const uint32_t entries = guest::LoadU32(base, list + kAnimationListEntries);
    const uint32_t clips = guest::LoadU32(base, list + kAnimationListClips);
    for (uint32_t i = 0; i < count && i < 4096; i++) {
      times.push_back(entries + i * layout.entry_bytes + layout.time);
      ends.push_back(guest::LoadFloat(base, clips + i * kAnimationClipBytes + kAnimationClipEnd));
    }
  }
}

struct ModelAnimationTicks {
  uint64_t tick = ~0ull;
  std::vector<float> before, after;
};
std::unordered_map<uint32_t, ModelAnimationTicks> g_model_animations;
std::vector<uint32_t> g_animation_times;
std::vector<float> g_animation_ends;

std::vector<float> ReadTimes(uint8_t* base, const std::vector<uint32_t>& times) {
  std::vector<float> values(times.size());
  for (size_t i = 0; i < times.size(); i++) {
    values[i] = guest::LoadFloat(base, times[i]);
  }
  return values;
}

void WriteTimes(uint8_t* base, const std::vector<uint32_t>& times, const std::vector<float>& values) {
  for (size_t i = 0; i < times.size(); i++) {
    guest::StoreFloat(base, times[i], values[i]);
  }
}
}  // namespace

extern "C" REX_FUNC(sub_8257A460) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(subtick)) {
    __imp__sub_8257A460(ctx, base);
    return;
  }
  const uint32_t model = ctx.r3.u32;
  const uint64_t tick = subtick::GetFramePacer().TicksReleased();
  ModelAnimationTimes(base, model, g_animation_times, g_animation_ends);
  std::vector<float> now = ReadTimes(base, g_animation_times);
  if (!RenderOnly()) {
    ModelAnimationTicks& ticks = g_model_animations[model];
    ticks.before = now;
    __imp__sub_8257A460(ctx, base);
    ticks.after = ReadTimes(base, g_animation_times);
    ticks.tick = tick;
    return;
  }
  auto it = g_model_animations.find(model);
  if (it != g_model_animations.end() && it->second.tick == tick && it->second.after == now) {
    const float t = std::clamp(subtick::GetFramePacer().FrameProgress(), 0.0f, 1.0f);
    std::vector<float> at = it->second.before;
    for (size_t i = 0; i < at.size(); i++) {
      at[i] = std::min(at[i] + t, std::max(at[i], g_animation_ends[i]));
    }
    WriteTimes(base, g_animation_times, at);
  }
  __imp__sub_8257A460(ctx, base);
  WriteTimes(base, g_animation_times, now);
}

// The car's draw flag (Car::DrawUpdate's r3 + 4296; 0 and the call draws
// nothing). Update sets it once a tick (Car::TickUpdate, sub_8215A788), and
// the cars' draw (sub_820C5BA0) changes it between views: the player's car in
// cockpit view is drawn twice a pass, and after the first call the flag is 0.
// In a race the cars' draw sets it again for each view on every pass, but in
// replays it doesn't, so only logic passes (after Update) drew the cockpit:
// it flickered in replays, paused too (--log_cockpit_draw: on logic passes
// the player's two calls saw 1 then 0, on render-only passes 0 and 0). On
// render-only passes each call now gets the flag the logic pass's same call
// (by car and order within the pass) had. In races that's what the draw sets
// anyway, so nothing changes there.
REXCVAR_DEFINE_BOOL(per_tick_car_draw_flag, true, "SBX",
                    "On render-only passes, give each car draw the draw flag the logic pass's "
                    "same call had (the cockpit flickered in replays)");

namespace {
constexpr uint32_t kDrawFlagOffset = 4296;  // from Car::DrawUpdate's r3

class HoldDrawFlag {
 public:
  void Apply(uint8_t* base, uint32_t object) {
    uint64_t pass = subtick::GameMessages_PassNumber();
    if (pass != pass_) {
      pass_ = pass;
      occurrences_.clear();
    }
    size_t occurrence = size_t(occurrences_[object]++);
    uint32_t address = object + kDrawFlagOffset;
    if (!RenderOnly()) {
      std::vector<uint32_t>& values = logic_[object];
      if (occurrence == 0) {
        values.clear();
      }
      values.push_back(guest::LoadU32(base, address));
      return;
    }
    auto it = logic_.find(object);
    if (it == logic_.end() || occurrence >= it->second.size()) {
      return;  // not drawn this way by the logic pass: left as it is
    }
    guest::StoreU32(base, address, it->second[occurrence]);
  }

 private:
  uint64_t pass_ = ~0ull;
  std::unordered_map<uint32_t, int> occurrences_;
  std::unordered_map<uint32_t, std::vector<uint32_t>> logic_;
};
HoldDrawFlag g_draw_flag;
}  // namespace

// --log_cockpit_draw (diagnostic, read only): one line per
// Car::DrawUpdate call with what decides how the car is drawn: its draw and
// interior flags (+4296, +4300), the hide flag (+5028), the parts word
// (+5084), the draw mode (+32), the viewport's view mode ([[0x826B29C8]] +
// 1860), which of the 21 camera passes are active (+588) and the type (+584)
// of passes 0 and 3. Turn it on from the overlay during the replay: it logs
// the next 300 calls.
REXCVAR_DEFINE_BOOL(log_cockpit_draw, false, "SBX",
                    "Diagnostic: log each car draw's flags and the active camera passes on "
                    "logic and render-only passes (the next 300 calls; turn on during a replay)");

namespace {
constexpr uint32_t kViewports = 0x826B29C8;  // [..] + 0: viewport 0
uint32_t g_cockpit_log_lines = 0;

bool GuestPointer(uint32_t address) {
  return address >= 0x40000000 && address < 0x90000000;
}

void LogCockpitDraw(uint8_t* base, uint32_t object) {
  if (!REXCVAR_GET(log_cockpit_draw) || g_cockpit_log_lines >= 300) {
    return;
  }
  uint32_t viewport = guest::LoadU32(base, kViewports);
  int64_t view = GuestPointer(viewport) ? int64_t(guest::LoadU32(base, viewport + 1860)) : -1;
  uint32_t active = 0;
  for (uint32_t i = 0; i < 21; i++) {
    if (guest::LoadU32(base, kCameraPasses + i * 592 + 588) != 0) {
      active |= 1u << i;
    }
  }
  REXLOG_INFO("cockpit draw (pass {} {}): object {:08X} draw {} interior {} hidden {} parts {:08X} "
              "mode {} view {} active passes {:06X} type0 {} type3 {}",
              subtick::GameMessages_PassNumber(), RenderOnly() ? "render" : "logic", object,
              guest::LoadU32(base, object + 4296), guest::LoadU32(base, object + 4300),
              guest::LoadU32(base, object + 5028), guest::LoadU32(base, object + 5084),
              guest::LoadU32(base, object + 32), view, active,
              guest::LoadU32(base, kCameraPasses + 584),
              guest::LoadU32(base, kCameraPasses + 3 * 592 + 584));
  g_cockpit_log_lines++;
}
}  // namespace

// Car::DrawUpdate (r3 = the car - 4288), every pass: first lowers the boost
// value (car+704, set again each tick) by 1.0, then poses and draws the car.
// On render-only passes the boost is replayed from the logic pass, so the
// turbo gauge doesn't fall to rest between ticks.
extern "C" REX_FUNC(sub_820DA930) {
  REX_FUNC_PROLOGUE();
  // The car's world matrix is recorded as the tick's pose on logic passes
  // (car_transforms.cpp), and blended before Race::DrawStep.
  uint32_t object = ctx.r3.u32;
  if (REXCVAR_GET(subtick) && REXCVAR_GET(per_tick_car_draw_flag)) {
    g_draw_flag.Apply(base, object);
  }
  LogCockpitDraw(base, object);
  subtick::CarTransforms_Record(base, object + kDrawUpdateCarOffset);
  subtick::CarTransformRestore pose =
      subtick::CarTransforms_DrawBegin(base, object + kDrawUpdateCarOffset);
  if (!REXCVAR_GET(subtick)) {
    __imp__sub_820DA930(ctx, base);
  } else {
    g_boost.Run(base, object + kDrawUpdateBoostOffset, [&] { __imp__sub_820DA930(ctx, base); });
  }
  subtick::CarTransforms_DrawEnd(base, pose);
}

// Race overlays (r3 = a list of 25 objects; from the Update handler
// sub_8215C0A0, during Update): updates each (vtable slot 4), then records
// its drawing (slot 5): the marker above opponent cars, tire smoke and the
// light trails behind head and tail lights, world-space sprites recorded
// with a camera pass's view-projection (+144) as it is when they record.
// While paused the overlays get the Update handler sub_8215C0A8 instead,
// which only records them (sub_8215BE80, below).
//
// With --subtick, the update runs here (logic passes only) and the recording
// is moved to just before Race::DrawStep, on every pass in a race
// (PerFrameLogic_RecordRaceOverlays), once pass 0 holds the frame's blended
// camera (game_messages.cpp). Recorded during Update, render-only passes
// (which skip Update) had to reuse the logic pass's cameras, so the sprites
// were drawn with an older camera than the world and shook against it.
namespace {
// --log_race_overlays: each line has a pattern (its message with the numbers
// taken out), kept per kind of pass (logic, render-only) and kind of line
// (an update, the paused recording, the draw step). A line is logged when
// its pattern differs from the last of its kinds, with the 4 lines before
// it and the 4 after; a race's steady run logs nothing.
int g_overlay_log_lines = 2000;
std::deque<std::string> g_overlay_log_before;  // the latest lines not logged
std::map<std::string, std::string> g_overlay_log_patterns;
int g_overlay_log_after = 0;

std::string OverlayLogPattern(const std::string& line) {
  std::string pattern;
  for (char c : line) {
    if (c < '0' || c > '9') {
      pattern += c;
    } else if (pattern.empty() || pattern.back() != '#') {
      pattern += '#';
    }
  }
  return pattern;
}

template <typename... Args>
void LogOverlays(fmt::format_string<Args...> format, Args&&... args) {
  if (!REXCVAR_GET(log_race_overlays) || g_overlay_log_lines <= 0) {
    return;
  }
  const char* pass_kind = RenderOnly() ? "render-only" : "logic";
  std::string message = fmt::format(format, std::forward<Args>(args)...);
  std::string line = fmt::format("race overlays: pass {} ({}), tick {}: {}",
                                 subtick::GameMessages_PassNumber(), pass_kind,
                                 subtick::GetFramePacer().TicksReleased(), message);
  const std::string kind = std::string(pass_kind) + " " + message.substr(0, message.find(' '));
  std::string& last = g_overlay_log_patterns[kind];
  std::string pattern = OverlayLogPattern(message);
  if (pattern != last) {
    last = pattern;
    for (const std::string& before : g_overlay_log_before) {
      REXLOG_INFO("{}", before);
      g_overlay_log_lines--;
    }
    g_overlay_log_before.clear();
    g_overlay_log_after = 5;  // this line and the 4 after
  }
  if (g_overlay_log_after > 0) {
    g_overlay_log_after--;
    REXLOG_INFO("{}", line);
    g_overlay_log_lines--;
    return;
  }
  g_overlay_log_before.push_back(std::move(line));
  if (g_overlay_log_before.size() > 4) {
    g_overlay_log_before.pop_front();
  }
}

void SaveOverlays(uint8_t* base, uint32_t list, bool paused) {
  g_overlay_list = list;
  g_overlay_paused = paused;
  g_overlay_tick = subtick::GetFramePacer().TicksReleased();
  uint8_t* cameras = base + kCameraPasses;
  g_overlay_cameras.assign(cameras, cameras + kCameraPassesBytes);
}
}  // namespace

extern "C" REX_FUNC(sub_8215BE00) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(subtick)) {
    __imp__sub_8215BE00(ctx, base);
    return;
  }
  if (RenderOnly()) {
    return;  // not expected (Update isn't delivered); recorded before the draw
  }
  uint32_t list = ctx.r3.u32;
  SaveOverlays(base, list, false);
  LogOverlays("updated (RaceOverlays::Run), list {:08X}", list);
  // The original's frame (stwu r1,-112(r1)).
  guest::StackFrame frame(ctx, 112);
  ForEachOverlay(ctx, base, list, kOverlayUpdateSlot);
}

// The race overlays' recording alone (r3 = the list; slot 5 of each), from
// the paused Update handler sub_8215C0A8 (ITC's 8215E158). Delivered with
// Update, so only on logic passes: the overlays were drawn on logic frames
// and not between them (the marker and tire smoke flickering while paused).
// With --subtick in a race the recording is moved to before Race::DrawStep,
// on every pass, as for RaceOverlays::Run.
extern "C" REX_FUNC(sub_8215BE80) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(subtick) || !subtick::GameMessages_InRace() || RenderOnly()) {
    __imp__sub_8215BE80(ctx, base);
    return;
  }
  SaveOverlays(base, ctx.r3.u32, true);
  LogOverlays("recorded while paused (RaceOverlays::Record), list {:08X}", ctx.r3.u32);
}

namespace subtick {

void PerFrameLogic_RecordRaceOverlays(PPCContext& ctx, uint8_t* base) {
  // Updated (or, paused, recorded) on the latest tick. A logic pass's draw
  // step can come before its update, so on a logic pass the tick before
  // counts too: it draws what the frames before it drew, as the game does.
  // Render-only passes after it draw only what that pass updated: once the
  // overlays stop being updated (the race over, the rival's marker and its
  // gauge gone), the frames between ticks stop drawing them at once.
  // The tick before counts only after an update: after the paused
  // recording (the results menu) the logic pass that resumes a free run drew
  // the destroyed health gauge again for one tick.
  uint64_t tick = GetFramePacer().TicksReleased();
  const bool latest = g_overlay_tick == tick;
  const bool before_update = !RenderOnly() && !g_overlay_paused && g_overlay_tick + 1 == tick;
  if (!REXCVAR_GET(subtick) || g_overlay_list == 0 || !(latest || before_update)) {
    LogOverlays("draw step: not recorded (last updated on tick {})", g_overlay_tick);
    return;
  }
  LogOverlays("draw step: recorded (updated on tick {})", g_overlay_tick);
  // The other camera passes as the overlays' update saw them (Draw reuses
  // them for other views), except those holding this frame's blend: pass 0,
  // and any other pass built from the blend this frame (the rear-view mirror,
  // rebuilt by CarTransforms_RebuildCamera on a render-only pass). Swapping
  // the mirror back to the logic pass's camera drew its rival markers, smoke
  // and trails once a tick against the blended cars (the mirror's judder).
  uint8_t* cameras = base + kCameraPasses;
  std::vector<uint32_t> swapped;  // the passes given the update's camera
  std::vector<uint8_t> current;   // what they held, to put back
  if (g_overlay_cameras.size() == kCameraPassesBytes) {
    for (uint32_t offset = kCameraPassBytes; offset < kCameraPassesBytes; offset += kCameraPassBytes) {
      if (CarTransforms_HoldsFrameCamera(base, kCameraPasses + offset)) {
        continue;
      }
      swapped.push_back(offset);
      current.insert(current.end(), cameras + offset, cameras + offset + kCameraPassBytes);
      std::memcpy(cameras + offset, g_overlay_cameras.data() + offset, kCameraPassBytes);
    }
  }
  uint64_t r3 = ctx.r3.u64;
  uint64_t lr = ctx.lr;
  {
    guest::StackFrame frame(ctx, 112);
    ForEachOverlay(ctx, base, g_overlay_list, kOverlayRecordSlot);
  }
  ctx.r3.u64 = r3;
  ctx.lr = lr;
  for (size_t i = 0; i < swapped.size(); i++) {
    std::memcpy(cameras + swapped[i], current.data() + i * kCameraPassBytes, kCameraPassBytes);
  }
}

}  // namespace subtick

// World-space sprites (ported from ITC-Recomp, "World-space sprites" in its
// docs/functions.md): a sprite's four corners (x, y, z, pad; 16 floats at
// +80) are written by game logic once a tick, and its recorder writes them
// as vertices drawn through the camera pass's view-projection (r4 + 144).
// With the camera blended, corners left at the tick's values ran ahead of
// their car early in each tick and caught up by its end (the light trails'
// small shake). Each recorder blends the corners between their last two
// ticks by the frame's progress while it records, then puts them back. A
// corner that moved more than 10 units in a tick is a new sprite.
namespace {
constexpr uint32_t kSpriteCountOffset = 0x109F0;
constexpr uint32_t kSpriteCornersOffset = 80;
constexpr uint32_t kSpriteStride = 64;
constexpr uint32_t kMaxSprites = 500;
constexpr float kMaxSpriteStep = 10.0f;

bool BlendSprites() {
  return REXCVAR_GET(subtick) && subtick::GameMessages_InRace();
}

float SpriteProgress() {
  return std::clamp(subtick::GetFramePacer().FrameProgress(), 0.0f, 1.0f);
}

// Sprite lists, matched by list and by order in the pass (the list is
// shared: each overlay fills it and records it in turn).
struct SpriteList {
  std::vector<float> before, now;
};
std::map<std::pair<uint32_t, int>, SpriteList> g_sprite_lists;
uint64_t g_sprite_pass = ~0ull;
std::map<uint32_t, int> g_sprite_occurrences;

// Single sprites, matched by sprite, caller and order in the pass.
struct SingleSprite {
  std::array<float, 16> before{}, now{};
  int ticks = 0;
};
std::map<std::tuple<uint32_t, uint32_t, int>, SingleSprite> g_single_sprites;
uint64_t g_single_sprite_pass = ~0ull;
std::map<std::pair<uint32_t, uint32_t>, int> g_single_sprite_occurrences;

// The marker above opponents: its centre, blended in its overlay's record
// (sub_8212BDA0, below); its corners aren't blended again.
struct MarkerCentre {
  std::array<float, 3> before{}, now{};
  int ticks = 0;
  uint64_t pass = ~0ull;
};
std::unordered_map<uint32_t, MarkerCentre> g_marker_centres;
bool g_in_marker_record = false;
}  // namespace

// Sprite list recorder (r3 = the list, up to 500 sprites, count at
// +0x109F0, sprite i at +80 + 64 * i; r4 = the camera pass): the light
// trails. ITC's sub_82128750.
extern "C" REX_FUNC(sub_821281B0) {
  REX_FUNC_PROLOGUE();
  if (!BlendSprites()) {
    __imp__sub_821281B0(ctx, base);
    return;
  }
  uint32_t list = ctx.r3.u32;
  uint32_t count = std::min(guest::LoadU32(base, list + kSpriteCountOffset), kMaxSprites);
  auto corner = [&](uint32_t i, uint32_t k) {
    return list + kSpriteCornersOffset + kSpriteStride * i + 4 * k;
  };
  uint64_t pass = subtick::GameMessages_PassNumber();
  if (pass != g_sprite_pass) {
    g_sprite_pass = pass;
    g_sprite_occurrences.clear();
  }
  SpriteList& sprites = g_sprite_lists[{list, g_sprite_occurrences[list]++}];
  if (!RenderOnly()) {
    sprites.before = std::move(sprites.now);
    sprites.now.resize(count * 16);
    for (uint32_t i = 0; i < count; i++) {
      for (uint32_t k = 0; k < 16; k++) {
        sprites.now[i * 16 + k] = guest::LoadFloat(base, corner(i, k));
      }
    }
  }
  float t = SpriteProgress();
  std::vector<std::pair<uint32_t, float>> saved;
  uint32_t paired = std::min<uint32_t>(
      count, uint32_t(std::min(sprites.before.size(), sprites.now.size()) / 16));
  for (uint32_t i = 0; i < paired; i++) {
    const float* a = &sprites.before[i * 16];
    const float* b = &sprites.now[i * 16];
    bool same = true, near = true;
    for (uint32_t k = 0; k < 16; k++) {
      same = same && guest::LoadFloat(base, corner(i, k)) == b[k];  // still this tick's
      near = near && std::isfinite(a[k]) && std::abs(b[k] - a[k]) <= kMaxSpriteStep;
    }
    if (!same || !near) {
      continue;
    }
    for (uint32_t k = 0; k < 16; k++) {
      if (k % 4 == 3) {
        continue;
      }
      saved.push_back({corner(i, k), b[k]});
      guest::StoreFloat(base, corner(i, k), a[k] + (b[k] - a[k]) * t);
    }
  }
  __imp__sub_821281B0(ctx, base);
  for (const auto& [address, value] : saved) {
    guest::StoreFloat(base, address, value);
  }
}

// Single sprite recorder (r3 = the sprite, corners at +80; r4 = the camera
// pass): the marker above opponent cars. ITC's sub_821281A8.
extern "C" REX_FUNC(sub_82127C08) {
  REX_FUNC_PROLOGUE();
  if (!BlendSprites() || g_in_marker_record) {
    __imp__sub_82127C08(ctx, base);
    return;
  }
  uint32_t sprite = ctx.r3.u32, caller = uint32_t(ctx.lr);
  uint64_t pass = subtick::GameMessages_PassNumber();
  if (pass != g_single_sprite_pass) {
    g_single_sprite_pass = pass;
    g_single_sprite_occurrences.clear();
  }
  int occurrence = g_single_sprite_occurrences[{sprite, caller}]++;
  std::array<float, 16> corners;
  for (uint32_t k = 0; k < 16; k++) {
    corners[k] = guest::LoadFloat(base, sprite + kSpriteCornersOffset + 4 * k);
  }
  SingleSprite& state = g_single_sprites[{sprite, caller, occurrence}];
  if (!RenderOnly()) {
    state.before = state.now;
    state.now = corners;
    state.ticks++;
  }
  bool blend = state.ticks >= 2 && corners == state.now;
  for (uint32_t k = 0; blend && k < 16; k++) {
    blend = std::isfinite(state.before[k]) &&
            std::abs(state.now[k] - state.before[k]) <= kMaxSpriteStep;
  }
  if (!blend) {
    __imp__sub_82127C08(ctx, base);
    return;
  }
  float t = SpriteProgress();
  for (uint32_t k = 0; k < 16; k++) {
    if (k % 4 != 3) {
      guest::StoreFloat(base, sprite + kSpriteCornersOffset + 4 * k,
                        state.before[k] + (state.now[k] - state.before[k]) * t);
    }
  }
  __imp__sub_82127C08(ctx, base);
  for (uint32_t k = 0; k < 16; k++) {
    guest::StoreFloat(base, sprite + kSpriteCornersOffset + 4 * k, corners[k]);
  }
}

// The marker above an opponent: its overlay's record (vtable slot 5; r3 =
// the overlay, r4 = the camera pass index; ITC's sub_8212CE40). It builds
// the sprite on the stack each call: its centre (overlay +176, +180, +184,
// set by the overlay's update once a tick from the opponent's position)
// plus axes facing the camera pass's eye (+400). With the camera blended,
// the axes change every frame, so the corners never matched the tick's and
// the single sprite recorder drew them unblended: the centre stepped once a
// tick against the blended car (the marker's jitter while racing). Here the
// centre is blended between its last two ticks by the frame's progress for
// the call, as the cars are, then put back.
extern "C" REX_FUNC(sub_8212BDA0) {
  REX_FUNC_PROLOGUE();
  if (!BlendSprites()) {
    __imp__sub_8212BDA0(ctx, base);
    return;
  }
  constexpr uint32_t kCentreOffset = 176;
  uint32_t overlay = ctx.r3.u32;
  uint32_t address = overlay + kCentreOffset;
  std::array<float, 3> centre;
  for (uint32_t k = 0; k < 3; k++) {
    centre[k] = guest::LoadFloat(base, address + 4 * k);
  }
  MarkerCentre& state = g_marker_centres[overlay];
  uint64_t pass = subtick::GameMessages_PassNumber();
  if (!RenderOnly() && state.pass != pass) {
    // Once a logic pass: a tick's centre (the same as the last while paused).
    state.pass = pass;
    state.before = state.now;
    state.now = centre;
    state.ticks++;
  }
  bool blend = state.ticks >= 2 && centre == state.now;
  for (uint32_t k = 0; blend && k < 3; k++) {
    blend = std::isfinite(state.before[k]) &&
            std::abs(state.now[k] - state.before[k]) <= kMaxSpriteStep;
  }
  if (blend) {
    float t = SpriteProgress();
    for (uint32_t k = 0; k < 3; k++) {
      guest::StoreFloat(base, address + 4 * k,
                        state.before[k] + (state.now[k] - state.before[k]) * t);
    }
  }
  g_in_marker_record = true;
  __imp__sub_8212BDA0(ctx, base);
  g_in_marker_record = false;
  if (blend) {
    for (uint32_t k = 0; k < 3; k++) {
      guest::StoreFloat(base, address + 4 * k, centre[k]);
    }
  }
}

// The rival intro cutscene (Update handler 0x82157948, a jump to sub_821573E0).
// It steps the cutscene and, deep inside (sub_82153A10), draws the rival's logo
// with sub_8220F7B0 (r3 = the logo's movie entry), which advances and draws the
// logo's UI movie. Update is skipped on render-only passes, so the logo showed on
// logic passes only (flicker). With --redraw_rival_logo, render-only passes
// deliver Update to this handler alone (game_messages.cpp): there the cutscene
// isn't stepped, only the logo is drawn, with the entry the logic pass drew. The
// UI movie replay keeps the logo advancing once a tick: the logo is a UI movie
// (the boss and rival intro screen, BUILD 00022624: the emblems and their light
// effects), and sub_8220F7B0 advances and draws it through sub_8211E378, a jump
// to the UI movie update sub_8211D4B8 (above). As ITC-Recomp's fix
// (docs/frame-interpolation.md, "The rival logo at the race start").
namespace {
bool g_in_rival_cutscene = false;
uint32_t g_rival_logo = 0;  // the entry the last logic pass drew, or 0
constexpr uint32_t kRivalLogoReturn = 0x82153BC8;  // the logo's call in sub_82153A10
}  // namespace

extern "C" REX_FUNC(sub_8220F7B0) {
  REX_FUNC_PROLOGUE();
  if (g_in_rival_cutscene) {
    g_rival_logo = ctx.r3.u32;
  }
  hooks::NativeRaceHud_RivalLogoDraw(ctx, base);
}

extern "C" REX_FUNC(sub_821573E0) {
  REX_FUNC_PROLOGUE();
  if (!RenderOnly()) {
    g_rival_logo = 0;
    g_in_rival_cutscene = true;
    __imp__sub_821573E0(ctx, base);
    g_in_rival_cutscene = false;
    return;
  }
  if (g_rival_logo == 0) {
    return;
  }
  // The original's stack frame (stwu r1,-208(r1)).
  guest::StackFrame frame(ctx, 208);
  ctx.r3.u64 = g_rival_logo;
  ctx.lr = kRivalLogoReturn;
  sub_8220F7B0(ctx, base);
}

// The cockpit dials: sub_820E3EB0 (the speedometer and tachometer) and
// sub_820E4178 (the dash's other needles, turbo and oil pressure among them),
// r3 = the dash, r4 = the car. Both are stateless: on every pass they turn the
// car's gauge values (car + 696 speed, + 700 revs, + 704 boost, + 712, + 716,
// + 720) into needle angles and draw the needles. The angle map (sub_820D32D0,
// and the tachometer's sub_820D3388) jumps at the ends of its range: below it
// the start angle, above it the end angle, but inside it
// ((value - from) / (to - from)) * (end - start), without the start added back.
// So a gauge at the end of its range that differs even slightly between the
// logic pass and a render-only pass (Car::DrawUpdate lowers the boost on every
// call, replayed only when the calls line up) swings the needle from one end of
// the dial to the other, while mid-range the same difference barely shows: the
// needles flipped between min and max at any rate above 30 Hz. On render-only
// passes the dials now read the gauge values the logic pass's same call read
// (matched by dash, car and order within the pass), and the values are put
// back after, so the needles move once a tick as on the 360 (as ITC-Recomp's
// --native_cockpit_dials keeps its gauges); the car under them is still
// blended.
REXCVAR_DEFINE_BOOL(per_tick_cockpit_dials, true, "SBX",
                    "Pose the cockpit dials' needles from the logic pass's gauge values on "
                    "render-only passes (above 30 fps they flipped between their ends)");

namespace {
constexpr std::array<uint32_t, 6> kDialGaugeOffsets = {696, 700, 704, 712, 716, 720};
using DialGauges = std::array<uint32_t, 6>;

DialGauges ReadDialGauges(uint8_t* base, uint32_t car) {
  DialGauges gauges;
  for (size_t i = 0; i < gauges.size(); i++) {
    gauges[i] = guest::LoadU32(base, car + kDialGaugeOffsets[i]);
  }
  return gauges;
}

void WriteDialGauges(uint8_t* base, uint32_t car, const DialGauges& gauges) {
  for (size_t i = 0; i < gauges.size(); i++) {
    guest::StoreU32(base, car + kDialGaugeOffsets[i], gauges[i]);
  }
}

class HoldDialGauges {
 public:
  template <typename Call>
  void Run(uint8_t* base, uint32_t dash, uint32_t car, Call&& call) {
    uint64_t pass = subtick::GameMessages_PassNumber();
    if (pass != pass_) {
      pass_ = pass;
      occurrences_.clear();
    }
    uint64_t key = (uint64_t(dash) << 32) | car;
    size_t occurrence = size_t(occurrences_[key]++);
    DialGauges now = ReadDialGauges(base, car);
    if (!RenderOnly()) {
      std::vector<DialGauges>& calls = held_[key];
      if (occurrence == 0) {
        calls.clear();
      }
      calls.push_back(now);
      call();
      return;
    }
    auto it = held_.find(key);
    if (it == held_.end() || it->second.empty()) {
      call();  // no logic pass drew these dials yet
      return;
    }
    const std::vector<DialGauges>& calls = it->second;
    WriteDialGauges(base, car, calls[std::min(occurrence, calls.size() - 1)]);
    call();
    WriteDialGauges(base, car, now);
  }

 private:
  uint64_t pass_ = ~0ull;
  std::unordered_map<uint64_t, int> occurrences_;
  std::unordered_map<uint64_t, std::vector<DialGauges>> held_;
};
HoldDialGauges g_dial_gauges;

bool HoldDials() {
  return REXCVAR_GET(subtick) && REXCVAR_GET(per_tick_cockpit_dials);
}
}  // namespace

extern "C" REX_FUNC(sub_820E3EB0) {
  REX_FUNC_PROLOGUE();
  if (!HoldDials()) {
    __imp__sub_820E3EB0(ctx, base);
    return;
  }
  g_dial_gauges.Run(base, ctx.r3.u32, ctx.r4.u32, [&] { __imp__sub_820E3EB0(ctx, base); });
}

extern "C" REX_FUNC(sub_820E4178) {
  REX_FUNC_PROLOGUE();
  if (!HoldDials()) {
    __imp__sub_820E4178(ctx, base);
    return;
  }
  g_dial_gauges.Run(base, ctx.r3.u32, ctx.r4.u32, [&] { __imp__sub_820E4178(ctx, base); });
}
