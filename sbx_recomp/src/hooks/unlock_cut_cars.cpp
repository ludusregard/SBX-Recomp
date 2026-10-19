// Unlocks the four cut cars (ECLI, S30, L86, T86) from the start of a career:
// they're offered with the starter cars and in the shop.
//
// The starter cars and the shop's car lists (sub_8217F2A0, sub_82272370,
// sub_8224A0E8) only offer a car whose bit is set in the save's car unlock
// bits: three words at save +0x1985C, bit n = car id n (docs/game_data.md). A
// new game sets them in sub_82113FB0 (SaveData::NewGame), leaving out the cars
// won in play (CON_X, BOSS00) and the cut cars, so the cut cars never show up
// without a save edit.
//
// This wraps the two ways to start a career and sets the cut cars' bits after
// each:
// - sub_82113FB0, a new game (also run at boot on the empty saves);
// - sub_82114278, a new game that keeps the old save's cars and unlock bits
//   (it runs sub_82113FB0, then copies the old bits back over it).
// A save already in progress is left as it is.
//
// Only the unlocks change: the cut cars' specs still have the price 999999
// (not sold).
//
// --unlock_cut_cars=false turns it off.

#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(unlock_cut_cars, true, "SBX",
                    "The cut cars (ECLI, S30, L86, T86) are offered from the "
                    "start of a new game (experimental)");

namespace {

constexpr uint32_t kCarUnlockBits = 0x1985C;  // In the save; bit n = car id n.

// Car ids (docs/game_data.md), all in the first word of the unlock bits.
constexpr uint32_t kEcli = 3;
constexpr uint32_t kS30 = 9;
constexpr uint32_t kL86 = 18;
constexpr uint32_t kT86 = 20;
constexpr uint32_t kCutCars = (1u << kEcli) | (1u << kS30) | (1u << kL86) | (1u << kT86);

void UnlockCutCars(uint8_t* base, uint32_t save) {
  if (!REXCVAR_GET(unlock_cut_cars) || save == 0) {
    return;
  }
  uint32_t address = save + kCarUnlockBits;
  uint32_t bits = guest::LoadU32(base, address);
  guest::StoreU32(base, address, bits | kCutCars);
  REXLOG_INFO("unlock_cut_cars: cut cars unlocked in the save at {:08X} (bits {:08X})",
              save, bits);
}

}  // namespace

// SaveData::NewGame: r3 = the save.
extern "C" REX_FUNC(sub_82113FB0) {
  uint32_t save = ctx.r3.u32;
  __imp__sub_82113FB0(ctx, base);
  UnlockCutCars(base, save);
}

// SaveData::NewGameKeepCars: r3 = the save.
extern "C" REX_FUNC(sub_82114278) {
  uint32_t save = ctx.r3.u32;
  __imp__sub_82114278(ctx, base);
  UnlockCutCars(base, save);
}
