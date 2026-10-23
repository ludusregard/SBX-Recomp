// Stock wheels for the cut cars (ECLI, L86, S30, T86).
//
// A car's front and rear wheel numbers are -1 for the stock wheel or n for
// the aftermarket wheel 36 + n (wheel i's files: XPR2 586 + 3i, rim 587 + 3i,
// brake 588 + 3i; docs/game_data.md, "Wheels"). When a car is set up
// (sub_820E6CA8) with both at -1 and its model block is 18 or more,
// sub_820D3620 replaces them with a wheel per block. For the four cut cars
// (blocks 19-22) that is AD00 (n = 0), so the ECLI's own stock wheel
// (wheel 19) is never used, and the others have no stock wheel at all.
//
// --cut_car_wheels (on by default): the ECLI keeps -1 and gets its own wheel
// (the stock path in sub_820D8860 / sub_820D8900 covers blocks 0-19), and the
// L86, S30 and T86 get the BOSS00's wheel (TA03, n = 18).
//
// --cut_car_steelies (on by default, needs --cut_car_wheels): the L86, S30
// and T86 get the LORRY's steel wheels (wheel 20) instead. Their numbers stay
// -1, the stock path gives them AD00 (blocks over 19), and the wheel models
// are then changed to the LORRY's (00000646.xpr lorry_w, 00000647.part,
// 00000648.part).
//
// The wheel numbers are only filled in while they're -1, so a car bought
// before this change may keep AD00.
//
// Inch-up: the wheel shop offers up to sub_821ED5D8's count of inch-ups, a
// fixed number per car id that is 20 inches minus the stock rim (at most 4)
// for the road cars, and 0 for the cut cars. With --cut_car_wheels the cut
// cars get the same rule: the ECLI 3 (17-inch), the L86, S30 and T86 4
// (14-inch).

#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(cut_car_wheels, true, "SBX",
                    "Give the cut cars stock wheels (the ECLI its own, the L86, S30 and "
                    "T86 the BOSS00's) and inch-ups in the wheel shop");
REXCVAR_DEFINE_BOOL(cut_car_steelies, true, "SBX",
                    "Give the L86, S30 and T86 the LORRY's steel wheels instead "
                    "(needs cut_car_wheels)");

namespace {

constexpr int32_t kEcliBlock = 19;
constexpr int32_t kBoss00Block = 23;
constexpr int32_t kTraffic = 3;  // car model + 28 for traffic.
constexpr uint32_t kLorryXpr = 586 + 3 * 20;  // Wheel 20: 646.

// Car ids (docs/game_data.md, "72: car specs").
constexpr uint32_t kEcliId = 3;
constexpr uint32_t kS30Id = 9;
constexpr uint32_t kL86Id = 18;
constexpr uint32_t kT86Id = 20;

bool IsSteelieBlock(int32_t block) { return block >= 20 && block <= 22; }  // L86, S30, T86.

bool UseSteelies() { return REXCVAR_GET(cut_car_wheels) && REXCVAR_GET(cut_car_steelies); }

// After sub_820D8860 / sub_820D8900: a stock wheel on the L86, S30 or T86
// becomes the LORRY's. second_offset: the model after the XPR2, 1 for the rim
// (587 + 3i), 2 for the brake (588 + 3i).
void UseLorryWheels(uint8_t* base, uint32_t car, uint32_t second_offset) {
  if (!UseSteelies() || int32_t(guest::LoadU32(base, car + 28)) == kTraffic) {
    return;
  }
  int32_t block = int32_t(guest::LoadU32(base, guest::LoadU32(base, car + 24) + 32));
  if (!IsSteelieBlock(block)) {
    return;
  }
  // Front: wheel number +44, XPR2 +2144, model +1168. Rear: +60, +2168, +1192.
  if (int32_t(guest::LoadU32(base, car + 44)) == -1) {
    guest::StoreU32(base, car + 2144, kLorryXpr);
    guest::StoreU32(base, car + 1168, kLorryXpr + second_offset);
  }
  if (int32_t(guest::LoadU32(base, car + 60)) == -1) {
    guest::StoreU32(base, car + 2168, kLorryXpr);
    guest::StoreU32(base, car + 1192, kLorryXpr + second_offset);
  }
}

// The car id of the garage car the wheel shop is showing, found the way
// sub_821ED5D8 finds it: r3 the menu; [menu+18904] the save, its garage slots
// at +6120 (1,964 bytes each, the car id at +224), a linked list of slot
// numbers from +6120+204 walked [save+0x19858] steps.
uint32_t ShopCarId(uint8_t* base, uint32_t menu) {
  auto load_s16 = [&](uint32_t address) { return int16_t(REX_LOAD_U16(address)); };
  uint32_t save = guest::LoadU32(base, menu + 18904);
  uint32_t slots = save + 6120;
  uint32_t steps = guest::LoadU32(base, save + 0x19858);
  uint32_t entry = (uint32_t(int32_t(load_s16(slots + 204))) << 2) + slots;
  for (; steps != 0; --steps) {
    entry = (uint32_t(int32_t(load_s16(entry))) << 2) + slots;
  }
  int32_t slot = load_s16(entry + 2);
  return guest::LoadU32(base, slots + uint32_t(slot) * 1964 + 224);
}

}  // namespace

// r3 the tuning menu: returns how many inch-ups the wheel shop offers.
extern "C" REX_FUNC(sub_821ED5D8) {
  uint32_t menu = ctx.r3.u32;
  __imp__sub_821ED5D8(ctx, base);
  if (!REXCVAR_GET(cut_car_wheels) || ctx.r3.u32 != 0) {
    return;
  }
  uint32_t car = ShopCarId(base, menu);
  if (car == kEcliId) {
    ctx.r3.u64 = 3;
  } else if (car == kS30Id || car == kL86Id || car == kT86Id) {
    ctx.r3.u64 = 4;
  }
}

// r3 a model block (18 or more), r4 and r5 the front and rear wheel numbers
// to fill in.
extern "C" REX_FUNC(sub_820D3620) {
  int32_t block = ctx.r3.s32;
  uint32_t front = ctx.r4.u32;
  uint32_t rear = ctx.r5.u32;
  if (REXCVAR_GET(cut_car_wheels)) {
    if (block == kEcliBlock || (IsSteelieBlock(block) && UseSteelies())) {
      guest::StoreU32(base, front, uint32_t(-1));
      guest::StoreU32(base, rear, uint32_t(-1));
      REXLOG_INFO("cut_car_wheels: block {}: stock wheel", block);
      return;
    }
    if (IsSteelieBlock(block)) {
      ctx.r3.s64 = kBoss00Block;
      __imp__sub_820D3620(ctx, base);
      REXLOG_INFO("cut_car_wheels: block {}: the BOSS00's wheels (front {}, rear {})", block,
                  int32_t(guest::LoadU32(base, front)), int32_t(guest::LoadU32(base, rear)));
      return;
    }
  }
  __imp__sub_820D3620(ctx, base);
}

extern "C" REX_FUNC(sub_820D8860) {
  uint32_t car = ctx.r3.u32;
  __imp__sub_820D8860(ctx, base);
  UseLorryWheels(base, car, 1);
}

extern "C" REX_FUNC(sub_820D8900) {
  uint32_t car = ctx.r3.u32;
  __imp__sub_820D8900(ctx, base);
  UseLorryWheels(base, car, 2);
}
