// The front end's car draw as our own C++ (--native_front_end_car_draw, on
// by default): the license plates shown and hidden.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_front_end_car_draw, true, "SBX",
                    "Our own C++ for the front end car's plates shown and hidden instead of "
                    "the recompiled functions");

namespace {

// The front end car's two plates' shown flags.
struct FrontEndCar {
  uint8_t unknown_0[2640];
  guest::u32_be front_plate_shown;
  uint8_t unknown_2644[1932];
  guest::u32_be rear_plate_shown;
};
GUEST_OFFSET(FrontEndCar, rear_plate_shown, 4576);

void SetPlates(PPCContext& ctx, uint8_t* base, bool shown) {
  guest::Guest g(ctx, base);
  FrontEndCar* car = guest::At<FrontEndCar>(ctx.r3.u32);
  car->front_plate_shown = shown;
  car->rear_plate_shown = shown;
}

bool On() { return REXCVAR_GET(native_front_end_car_draw); }

}  // namespace

// Plates::Show and Plates::Hide: r3 the car. Return r3.
extern "C" REX_FUNC(sub_820DEF48) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DEF48(ctx, base);
    return;
  }
  SetPlates(ctx, base, true);
}

extern "C" REX_FUNC(sub_820DEF58) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DEF58(ctx, base);
    return;
  }
  SetPlates(ctx, base, false);
}
