// The race HUD object as our own C++ (--native_race_hud_object, on by
// default): its gauges' and gear's nodes, the boost value, the player it
// follows, the warning and movie starts and runs, its draw, the item's
// update, the touching test and its teardown, and the integer power its
// meters use.
//
// Written from SBX's generated code. The HUD: +16 the player, +440 its
// vehicle, +460 the item shown, +468 and +472 whether the warning and the
// gauges are drawn, +588 its node list, +608 and +616 its two UI movies.
// The UI movies' node lookups (sub_8211A748), run (sub_8211E378) and draw
// stay recompiled, and so do the HUD's own set-up, its racers' and route
// ticks and RaceHud::RunMovie (native_race_hud).

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_hud_object, true, "SBX",
                    "Our own C++ for the race HUD object (its gauges, gear, boost, player, "
                    "warning, draw, item and teardown) instead of the recompiled functions; "
                    "the UI movies and the HUD's set-up stay recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// RaceHud::ClearWarning (sub_82210028).
// 1 stores
constexpr Fill kRaceHud_ClearWarning0[] = {
    {468, kWord, 0x0},
};

// RaceHud::StartCountdown (sub_8220FFF8).
// 1 stores
constexpr Fill kRaceHud_StartCountdown0[] = {
    {436, kWord, 0x1},
};

bool On() { return REXCVAR_GET(native_race_hud_object); }

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kRaceData = 0x826C7D90;  // [it] + 7240: the item to show
constexpr uint32_t kOneDouble = 0x82002210;  // 1.0

// UiMovie::Node (sub_8211A748): r3 the movie, r4 the parent (0 or -1), r5
// the node's index.
uint32_t Node(PPCContext& ctx, uint8_t* base, uint32_t movie, uint32_t parent, uint32_t index) {
  ctx.r3.u64 = movie;
  ctx.r4.u64 = uint64_t(int64_t(int32_t(parent)));
  ctx.r5.u64 = index;
  sub_8211A748(ctx, base);
  return ctx.r3.u32;
}

// Racers::Vehicle (sub_820C0288) of a player.
uint32_t PlayerVehicle(PPCContext& ctx, uint8_t* base, uint32_t player) {
  ctx.r3.u64 = player;
  sub_820C0288(ctx, base);
  return ctx.r3.u32;
}

}  // namespace

// RaceHud::ClearWarning.
extern "C" REX_FUNC(sub_82210028) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210028(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceHud_ClearWarning0, args);
  ctx.r3.u64 = object;
}

// RaceHud::StartCountdown.
extern "C" REX_FUNC(sub_8220FFF8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8220FFF8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRaceHud_StartCountdown0, args);
  ctx.r3.u64 = guest::LoadU32(base, object + 612);
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  ctx.r5.u64 = 0x0u;
  ctx.r6.u64 = 0x0u;
  ctx.r7.u64 = 0x0u;
  ctx.r8.u64 = 0x0u;
  ctx.r9.u64 = 0x0u;
  sub_8211DB38(ctx, base);
}

// RaceHud::StartMovie1.
extern "C" REX_FUNC(sub_822104A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822104A0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 628);
  ctx.r4.u64 = 0x0u;
  ctx.r5.u64 = 0x0u;
  ctx.r6.u64 = 0x0u;
  ctx.r7.u64 = 0x0u;
  ctx.r8.u64 = 0x0u;
  sub_8211DC88(ctx, base);
  ctx.r3.u64 = guest::LoadU32(base, object + 628);
  ctx.r4.u64 = 0x0u;
  sub_8211DFA0(ctx, base);
}

// RaceHud::FinishMovie1.
extern "C" REX_FUNC(sub_82210600) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210600(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 628);
  ctx.r4.u64 = 0x0u;
  sub_8211E0E0(ctx, base);
}

// RaceHud::RunMovie1.
extern "C" REX_FUNC(sub_82210880) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210880(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 628);
  ctx.r4.u64 = 0x0u;
  sub_8211E378(ctx, base);
}

// RaceScreens::DrawPauseMenu.
extern "C" REX_FUNC(sub_8220FA90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8220FA90(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 1012);
  sub_8220D848(ctx, base);
}

// RaceScreens::DrawAfterRaceMenu.
extern "C" REX_FUNC(sub_8220FA98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8220FA98(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 1020);
  sub_822197F0(ctx, base);
}


// RaceHud::SetBoost: sub_8220FEA0 on +132, then node 5's +36 = the boost
// (+232).
extern "C" REX_FUNC(sub_82210BA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210BA0(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32;
  ctx.r3.u64 = hud + 132;
  sub_8220FEA0(ctx, base);
  const uint32_t node = Node(ctx, base, LoadU32(base, hud + 608), 0, 5);
  StoreU32(base, node + 36, LoadU32(base, hud + 232));
}

// RaceHud::ShowGauge: r4 the gauge, r5 its value, into nodes 12 + r4 and
// 9 + r4's +80.
extern "C" REX_FUNC(sub_82210A90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210A90(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32, gauge = ctx.r4.u32, value = ctx.r5.u32;
  const uint32_t first = Node(ctx, base, LoadU32(base, hud + 608), 0, gauge + 12);
  const uint32_t second = Node(ctx, base, LoadU32(base, hud + 608), 0, gauge + 9);
  StoreU32(base, first + 80, value);
  StoreU32(base, second + 80, value);
}

// RaceHud::SetGear: r4 the gear into node 11's frame (+108): 1 to 5 are
// 0 to 4, 6 is 7, 0 (neutral) 5, reverse 6.
extern "C" REX_FUNC(sub_82210B38) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210B38(ctx, base);
    return;
  }
  const int32_t gear = int32_t(ctx.r4.u32);
  const uint32_t node = Node(ctx, base, LoadU32(base, ctx.r3.u32 + 608), 0xFFFFFFFFu, 11);
  int32_t frame;
  if (gear > 0) {
    frame = gear == 6 ? 7 : gear - 1;
  } else {
    frame = gear == 0 ? 5 : 6;
  }
  StoreU32(base, node + 108, uint32_t(frame));
}

// RaceHud::SetPlayer: +16 the player, +440 its vehicle.
extern "C" REX_FUNC(sub_82210418) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210418(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32, player = ctx.r4.u32;
  StoreU32(base, hud + 16, player);
  StoreU32(base, hud + 440, PlayerVehicle(ctx, base, player));
}

// RaceHud::Touching: whether r4's vehicle's collision object (+340)
// touches anything of groups 0x07000000 (sub_820B7B30).
extern "C" REX_FUNC(sub_82213608) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82213608(ctx, base);
    return;
  }
  const uint32_t object = LoadU32(base, PlayerVehicle(ctx, base, ctx.r4.u32) + 340);
  if (object == 0) {
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r3.u64 = object;
  ctx.r4.u64 = 0x07000000;
  sub_820B7B30(ctx, base);
  ctx.r3.u64 = ctx.r3.u32 != 0 ? 1 : 0;
}

// RaceHud::DrawWarning: with +468 set, the warning movie (+616) drawn
// (UiMovie::Run, r4 = 1: draw only).
extern "C" REX_FUNC(sub_82210850) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210850(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32;
  if (int32_t(LoadU32(base, hud + 468)) == 0) {
    return;
  }
  ctx.r3.u64 = LoadU32(base, hud + 616);
  ctx.r4.u64 = 1;
  sub_8211E378(ctx, base);
}

// RaceHud::Draw: the HUD's movie run, then with +472 and +12 set node 19
// drawn (sub_8211C0B8).
extern "C" REX_FUNC(sub_82210630) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82210630(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32;
  ctx.r3.u64 = LoadU32(base, hud + 608);
  ctx.r4.u64 = 0;
  sub_8211E378(ctx, base);
  if (int32_t(LoadU32(base, hud + 472)) == 0 || int32_t(LoadU32(base, hud + 12)) == 0) {
    return;
  }
  ctx.r3.u64 = Node(ctx, base, LoadU32(base, hud + 608), 0xFFFFFFFFu, 19);
  sub_8211C0B8(ctx, base);
}

// RaceHud::UpdateItem: the race data's item ([0x826C7D90] + 7240) shown
// (sub_822113A0) when it's changed (+460).
extern "C" REX_FUNC(sub_82211FA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82211FA0(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32;
  const uint32_t item = LoadU32(base, LoadU32(base, kRaceData) + 7240);
  if (LoadU32(base, hud + 460) == item) {
    return;
  }
  StoreU32(base, hud + 460, item);
  ctx.r4.u64 = item;
  sub_822113A0(ctx, base);
}

// RaceHud::TearDown: sub_822888E8 on the HUD, then its node list (+588)
// freed from the movie (sub_8211AD60, r4 = 56).
extern "C" REX_FUNC(sub_82211F08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82211F08(ctx, base);
    return;
  }
  const uint32_t hud = ctx.r3.u32;
  sub_822888E8(ctx, base);
  const uint32_t list = LoadU32(base, hud + 588);
  if (list == 0) {
    return;
  }
  ctx.r3.u64 = LoadU32(base, hud + 608);
  ctx.r4.u64 = 56;
  ctx.r5.u64 = list;
  sub_8211AD60(ctx, base);
}

// PowInt: f1 to the power r4 (doubles, by squaring; 1.0 / the result for
// a negative power).
extern "C" REX_FUNC(sub_822119A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822119A8(ctx, base);
    return;
  }
  const int32_t power = int32_t(ctx.r4.u32);
  double x = ctx.f1.f64;
  const double one = std::bit_cast<double>(REX_LOAD_U64(kOneDouble));
  double result = one;
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
    result = one / result;
  }
  ctx.f1.f64 = result;
}
