// The race HUD object as our own C++ (--native_race_hud_object, on by
// default): its gauges' and gear's nodes, the boost value, the player it
// follows, the warning and movie starts and runs, its draw, the item's
// update, the touching test and its teardown, and the integer power its
// meters use. The UI movies' node lookups (sub_8211A748), run
// (sub_8211E378) and draw stay recompiled, and so do the HUD's own set-up,
// its racers' and route ticks and RaceHud::RunMovie (native_race_hud).

#include "game/physics.h"
#include "game/race_hud.h"
#include "game/vehicle.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_hud_object, true, "SBX",
                    "Our own C++ for the race HUD object (its gauges, gear, boost, player, "
                    "warning, draw, item and teardown) instead of the recompiled functions; "
                    "the UI movies and the HUD's set-up stay recompiled");

namespace {

using game::RaceHud;
using game::RaceProgress;
using game::RaceScreens;
using game::UiNode;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_race_hud_object); }

constexpr uint32_t kRaceProgress = 0x826C7D90;  // [it] (writable)
constexpr uint32_t kTouchGroups = 0x07000000;   // The collision groups the touching test looks at.

// UiMovie::Node (sub_8211A748): the movie, the parent (0 or -1), the
// node's index.
UiNode* Node(Guest& g, uint32_t movie, int32_t parent, uint32_t index) {
  return At<UiNode>(g.Call(sub_8211A748, movie, parent, index));
}

// Racers::Vehicle (sub_820C0288) of a player.
game::Vehicle* PlayerVehicle(Guest& g, uint32_t player) {
  return At<game::Vehicle>(g.Call(sub_820C0288, player));
}

// UiMovie::Run (sub_8211E378): r4 = 1 draws only.
void RunMovie(Guest& g, uint32_t movie, uint32_t draw_only) { g.Call(sub_8211E378, movie, draw_only); }

}  // namespace

#define SBX_RACE_HUD_OBJECT_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                     \
  if (!On()) {                             \
    __imp__##name(ctx, base);              \
    return;                                \
  }                                        \
  Guest g(ctx, base);

// RaceHud::ClearWarning: r3 the HUD. Returns it.
extern "C" REX_FUNC(sub_82210028) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210028)
  At<RaceHud>(ctx.r3.u32)->warning = 0;
}

// RaceHud::StartCountdown: the countdown's movie started (sub_8211DB38).
extern "C" REX_FUNC(sub_8220FFF8) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_8220FFF8)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  hud->counting_down = 1;
  g.Call(sub_8211DB38, hud->countdown_movie, -1, 0u, 0u, 0u, 0u, 0u);
}

// RaceHud::StartMovie1, FinishMovie1, RunMovie1: the movie at +628
// started (sub_8211DC88 then sub_8211DFA0), finished (sub_8211E0E0) or
// run.
extern "C" REX_FUNC(sub_822104A0) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_822104A0)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  g.Call(sub_8211DC88, hud->movie_628, 0u, 0u, 0u, 0u, 0u);
  g.Call(sub_8211DFA0, hud->movie_628, 0u);
}

extern "C" REX_FUNC(sub_82210600) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210600)
  g.Call(sub_8211E0E0, At<RaceHud>(ctx.r3.u32)->movie_628, 0u);
}

extern "C" REX_FUNC(sub_82210880) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210880)
  RunMovie(g, At<RaceHud>(ctx.r3.u32)->movie_628, 0);
}

// RaceScreens::DrawPauseMenu and DrawAfterRaceMenu: r3 the screens; the
// menu's draw (sub_8220D848, sub_822197F0).
extern "C" REX_FUNC(sub_8220FA90) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_8220FA90)
  g.Call(sub_8220D848, At<RaceScreens>(ctx.r3.u32)->pause_menu);
}

extern "C" REX_FUNC(sub_8220FA98) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_8220FA98)
  g.Call(sub_822197F0, At<RaceScreens>(ctx.r3.u32)->after_race_menu);
}

// RaceHud::SetBoost: the boost meter updated (sub_8220FEA0), then node 5
// given the boost.
extern "C" REX_FUNC(sub_82210BA0) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210BA0)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  g.Call(sub_8220FEA0, hud->boost_meter);
  Node(g, hud->movie, 0, 5)->value_36 = uint32_t(hud->boost);
}

// RaceHud::ShowGauge: r4 the gauge, r5 its value, into nodes 12 + r4 and
// 9 + r4.
extern "C" REX_FUNC(sub_82210A90) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210A90)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  const uint32_t gauge = ctx.r4.u32, value = ctx.r5.u32;
  UiNode* first = Node(g, hud->movie, 0, gauge + 12);
  UiNode* second = Node(g, hud->movie, 0, gauge + 9);
  first->value_80 = value;
  second->value_80 = value;
}

// RaceHud::SetGear: r4 the gear into node 11's frame: 1 to 5 are frames 0
// to 4, 6 is 7, 0 (neutral) 5, reverse 6.
extern "C" REX_FUNC(sub_82210B38) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210B38)
  const int32_t gear = ctx.r4.s32;
  UiNode* node = Node(g, At<RaceHud>(ctx.r3.u32)->movie, -1, 11);
  int32_t frame;
  if (gear > 0) {
    frame = gear == 6 ? 7 : gear - 1;
  } else {
    frame = gear == 0 ? 5 : 6;
  }
  node->frame = frame;
}

// RaceHud::SetPlayer: r4 the player, and its vehicle.
extern "C" REX_FUNC(sub_82210418) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210418)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  const uint32_t player = ctx.r4.u32;
  hud->player = player;
  hud->vehicle = guest::AddressOf(PlayerVehicle(g, player));
}

// RaceHud::Touching: whether r4's vehicle's collision object touches
// anything of the touching groups (sub_820B7B30).
extern "C" REX_FUNC(sub_82213608) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82213608)
  game::Vehicle* vehicle = PlayerVehicle(g, ctx.r4.u32);
  game::CollisionObject* object = vehicle->dynamics.collision_object;
  if (object == nullptr) {
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r3.u64 = g.Call(sub_820B7B30, object, kTouchGroups) != 0 ? 1 : 0;
}

// RaceHud::DrawWarning: when shown, the warning movie drawn.
extern "C" REX_FUNC(sub_82210850) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210850)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  if (hud->warning == 0) {
    return;
  }
  RunMovie(g, hud->warning_movie, 1);
}

// RaceHud::Draw: the HUD's movie run, then with the gauges and the HUD
// shown, node 19 drawn (sub_8211C0B8).
extern "C" REX_FUNC(sub_82210630) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82210630)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  RunMovie(g, hud->movie, 0);
  if (hud->gauges == 0 || hud->shown == 0) {
    return;
  }
  g.Call(sub_8211C0B8, Node(g, hud->movie, -1, 19));
}

// RaceHud::UpdateItem: the race's item shown (sub_822113A0, r3 as it came)
// when it's changed.
extern "C" REX_FUNC(sub_82211FA0) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82211FA0)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  const uint32_t item = At<RaceProgress>(*At<u32_be>(kRaceProgress))->item;
  if (hud->item == item) {
    return;
  }
  hud->item = item;
  ctx.r4.u64 = item;
  g.Call(sub_822113A0);
}

// RaceHud::TearDown: sub_822888E8 on the HUD, then its node list freed
// from the movie (sub_8211AD60, r4 = 56).
extern "C" REX_FUNC(sub_82211F08) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_82211F08)
  RaceHud* hud = At<RaceHud>(ctx.r3.u32);
  g.Call(sub_822888E8);
  if (hud->nodes == 0) {
    return;
  }
  g.Call(sub_8211AD60, hud->movie, 56u, hud->nodes);
}

// PowInt: f1 to the power r4 (doubles, by squaring; 1.0 / the result for a
// negative power).
extern "C" REX_FUNC(sub_822119A8) {
  SBX_RACE_HUD_OBJECT_FALLBACK(sub_822119A8)
  const int32_t power = ctx.r4.s32;
  double x = ctx.f1.f64;
  double result = 1.0;
  uint32_t n = power < 0 ? uint32_t(-int64_t(power)) : uint32_t(power);
  for (;;) {
    if (n & 1) {
      result *= x;
    }
    n >>= 1;
    if (n == 0) {
      break;
    }
    x *= x;
  }
  if (power < 0) {
    result = 1.0 / result;
  }
  ctx.f1.f64 = result;
}
