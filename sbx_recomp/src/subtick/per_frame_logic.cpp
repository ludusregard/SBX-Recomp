// Game logic that the game runs from its Draw message (or during Update
// along with drawing), counting frames.
//
// On the 360 every Draw was a 30 Hz tick, so this logic counts ticks; with
// --subtick's render-only passes it would run several times a tick. Each
// case here keeps it to once a tick without changing what's drawn. The
// list follows what ITC-Recomp found in the same game (Import Tuner
// Challenge); more may turn up (anything that runs too fast or flickers
// only above 30 fps).

#include "guest.h"
#include "subtick/car_transforms.h"
#include "subtick/frame_pacer.h"
#include "subtick/game_messages.h"
#include "subtick/per_frame_logic.h"

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
REXCVAR_DECLARE(bool, tick_rate_60);

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
// the call runs from the object's current state, which is then put back.
class ReplayFromLogicPass {
 public:
  ReplayFromLogicPass(uint32_t bytes) : bytes_(bytes) {}

  template <typename Call>
  void Run(uint8_t* base, uint32_t object, Call&& call) {
    uint64_t pass = subtick::GameMessages_PassNumber();
    if (pass != pass_) {
      pass_ = pass;
      occurrences_.clear();
    }
    Key key{object, occurrences_[object]++};
    uint8_t* state = base + object;
    if (!RenderOnly()) {
      States& states = states_[key];
      states.before.assign(state, state + bytes_);
      call();
      states.after.assign(state, state + bytes_);
      return;
    }
    auto it = states_.find(key);
    if (it == states_.end() || it->second.after.size() != bytes_) {
      CallFromCurrentState(state, call);
      return;
    }
    if (std::memcmp(state, it->second.after.data(), bytes_) != 0) {
      CallFromCurrentState(state, call);
      return;
    }
    std::memcpy(state, it->second.before.data(), bytes_);
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
  };
  uint32_t bytes_;
  uint64_t pass_ = ~0ull;
  std::unordered_map<Key, States, KeyHash> states_;
  std::unordered_map<uint32_t, int> occurrences_;
  std::vector<uint8_t> current_;
};

// UI movies' state: the furthest field ITC-Recomp found used, ~+0x96C8.
ReplayFromLogicPass g_ui_movies(0xA000);
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
  __imp__sub_820F1038(ctx, base);
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
// fade is still drawn every frame, at the tick's alpha. With --tick_rate_60
// the play time's one is taken back every other call, so it counts real time.
namespace {
constexpr uint32_t kFadeOutCounts = 0x82694DBC;  // 2 layers
constexpr uint32_t kFadeInCounts = 0x82694DC4;   // 2 layers
constexpr uint32_t kFrameCountdown = 0x825F180C;
constexpr std::array<uint32_t, 5> kPerTickCounts = {
    kFadeOutCounts, kFadeOutCounts + 4, kFadeInCounts, kFadeInCounts + 4, kFrameCountdown};
}  // namespace

extern "C" REX_FUNC(sub_82107A38) {
  REX_FUNC_PROLOGUE();
  static bool take_back = false;
  bool undo = false;
  bool render_only = RenderOnly();
  if (render_only) {
    undo = true;
  } else if (REXCVAR_GET(tick_rate_60)) {
    undo = take_back;
    take_back = !take_back;
  }
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
    __imp__sub_82585C40(ctx, base);
    return;
  }
  uint32_t node = ctx.r3.u32, track = ctx.r4.u32;
  uint32_t property = guest::LoadU32(base, track);
  uint32_t descriptor = kTrackProperties + 12 * property;
  bool adds = property < kTrackPropertyCount && guest::LoadU8(base, descriptor + 8) == 0;
  uint32_t type = property < kTrackPropertyCount ? guest::LoadU32(base, descriptor + 4) : 3;
  if (!adds || type >= 3) {
    __imp__sub_82585C40(ctx, base);
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
  __imp__sub_82585C40(ctx, base);
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
  if (ctx.r4.u32 != 0 || !REXCVAR_GET(subtick)) {
    __imp__sub_8211D4B8(ctx, base);
  } else {
    g_ui_movies.Run(base, ctx.r3.u32, [&] { __imp__sub_8211D4B8(ctx, base); });
  }
  if (tracks) {
    if (!RenderOnly()) {
      NotePutBackTargets(base, *g_added);
    }
    g_update_depth--;
  }
  g_added = added_before;
}

// Car::DrawUpdate (r3 = the car - 4288), every pass: first lowers the boost
// value (car+704, set again each tick) by 1.0, then poses and draws the car.
// On render-only passes the boost is replayed from the logic pass, so the
// turbo gauge doesn't fall to rest between ticks.
extern "C" REX_FUNC(sub_820DA930) {
  REX_FUNC_PROLOGUE();
  // The car's world matrix is recorded as the tick's pose on logic passes
  // (car_transforms.cpp), and blended before Race::DrawStep.
  uint32_t object = ctx.r3.u32;
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
void SaveOverlays(uint8_t* base, uint32_t list) {
  g_overlay_list = list;
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
  SaveOverlays(base, list);
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
  SaveOverlays(base, ctx.r3.u32);
}

namespace subtick {

void PerFrameLogic_RecordRaceOverlays(PPCContext& ctx, uint8_t* base) {
  // Updated (or, paused, recorded) on the latest tick, or the one before:
  // a logic pass whose update comes after its draw step still draws what the
  // frames before it drew.
  uint64_t tick = GetFramePacer().TicksReleased();
  if (!REXCVAR_GET(subtick) || g_overlay_list == 0 ||
      (g_overlay_tick != tick && g_overlay_tick + 1 != tick)) {
    return;
  }
  // The other camera passes as the overlays' update saw them (Draw reuses
  // them for other views); pass 0 as built for this frame.
  uint8_t* cameras = base + kCameraPasses;
  std::vector<uint8_t> current;
  bool swap = g_overlay_cameras.size() == kCameraPassesBytes;
  if (swap) {
    current.assign(cameras + kCameraPassBytes, cameras + kCameraPassesBytes);
    std::memcpy(cameras + kCameraPassBytes, g_overlay_cameras.data() + kCameraPassBytes,
                kCameraPassesBytes - kCameraPassBytes);
  }
  uint64_t r3 = ctx.r3.u64;
  uint64_t lr = ctx.lr;
  {
    guest::StackFrame frame(ctx, 112);
    ForEachOverlay(ctx, base, g_overlay_list, kOverlayRecordSlot);
  }
  ctx.r3.u64 = r3;
  ctx.lr = lr;
  if (swap) {
    std::memcpy(cameras + kCameraPassBytes, current.data(), current.size());
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
