// The location screen as our own C++ (--native_location_screen, on by
// default): its reset.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_location_screen, true, "SBX",
                    "Our own C++ for the location screen's reset instead of the recompiled "
                    "function");

namespace {

using guest::i32_be;
using guest::u32_be;

struct LocationScreen {
  u32_be unknown_0;
  uint8_t unknown_4[132];
  u32_be unknown_136;
  uint8_t unknown_140[264];
  u32_be unknown_404;
  uint8_t unknown_408[136];
  u32_be unknown_544[3];  // 544, 548, 552.
  uint8_t unknown_556[4];
  uint8_t member_560[240];  // Reset by sub_82225310.
  u32_be unknown_800[6];    // 800 to 820.
  uint8_t unknown_824[28];
  u32_be unknown_852[7];  // 852 to 876.
  i32_be unknown_880[2];
  u32_be unknown_888[2];
  uint8_t unknown_896[4];
  u32_be unknown_900;
};
GUEST_OFFSET(LocationScreen, member_560, 560);
GUEST_OFFSET(LocationScreen, unknown_852, 852);
GUEST_OFFSET(LocationScreen, unknown_900, 900);

}  // namespace

// LocationScreen::Reset: r3 the screen.
extern "C" REX_FUNC(sub_822254A0) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_location_screen)) {
    __imp__sub_822254A0(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  LocationScreen* screen = guest::At<LocationScreen>(ctx.r3.u32);
  screen->unknown_0 = 0;
  screen->unknown_544[2] = 0;
  screen->unknown_800[0] = 0;
  screen->unknown_852[1] = 0;
  screen->unknown_852[2] = 0;
  screen->unknown_852[3] = 14;
  g.Call(sub_82225310, screen->member_560);
  screen->unknown_136 = 0;
  screen->unknown_404 = 0;
  screen->unknown_544[0] = 0;
  screen->unknown_544[1] = 0;
  screen->unknown_800[2] = 0;
  screen->unknown_800[1] = 4;
  screen->unknown_852[0] = 0;
  screen->unknown_852[4] = 0;
  screen->unknown_852[5] = 0;
  screen->unknown_852[6] = 0;
  screen->unknown_800[3] = 0;
  screen->unknown_880[0] = -1;
  screen->unknown_880[1] = -1;
  screen->unknown_800[4] = 0;
  screen->unknown_800[5] = 0;
  screen->unknown_888[0] = 0;
  screen->unknown_888[1] = 0;
  screen->unknown_900 = 0;
}
