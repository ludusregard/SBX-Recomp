// The garage's car records as our own C++ (--native_car_makers, on by
// default): a garage car's reset, the garage cars' static set-up and a car
// record's part order reset.
//
// The garage stage's car set-up, its records' copies and reapplies stay
// recompiled.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_car_makers, true, "SBX",
                    "Our own C++ for the garage cars' resets and a car record's bytes instead of the recompiled functions; the garage stage stays recompiled");

namespace {

using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u32_be;

// A car in the garage (two of them, kept one after the other).
struct GarageCar {
  uint8_t car[1880];  // The car itself (Car::Reset).
  u32_be unknown_1880;
  f32_be unknown_1884[2];
  u32_be unknown_1892[2];
  struct Settings {
    u32_be words[15];
  } settings;  // Copied from the defaults at 0x826B2D24.
  u32_be unknown_1960;
  u32_be unknown_1964;
};
GUEST_SIZE(GarageCar, 1968);
GUEST_OFFSET(GarageCar, settings, 1900);

constexpr uint32_t kGarageCars = 0x8280DEC8;
constexpr uint32_t kDefaultSettings = 0x826B2D24;

// A car record's part slots: which part goes in each, in order.
struct CarRecord {
  u32_be part_order[11];
};
constexpr uint32_t kDefaultPartOrder[11] = {0, 0, 0, 1, 2, 3, 4, 5, 0, 6, 7};

bool On() { return REXCVAR_GET(native_car_makers); }

void ResetGarageCar(Guest& g, GarageCar* car) {
  g.Call(sub_820CD590, car->car);  // Car::Reset
  car->unknown_1880 = 0;
  car->unknown_1884[0] = 0.0f;
  car->unknown_1884[1] = 0.0f;
  car->unknown_1892[0] = 0;
  car->unknown_1892[1] = 0;
  car->unknown_1960 = 1;
  car->settings = *At<GarageCar::Settings>(kDefaultSettings);
}

}  // namespace

#define SBX_CAR_MAKERS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  Guest g(ctx, base);

// GarageCar::Reset: r3 the garage car.
extern "C" REX_FUNC(sub_82110370) {
  SBX_CAR_MAKERS_FALLBACK(sub_82110370)
  ResetGarageCar(g, At<GarageCar>(ctx.r3.u32));
}

// GarageCars::StaticInit: both garage cars reset.
extern "C" REX_FUNC(sub_825BF2C8) {
  SBX_CAR_MAKERS_FALLBACK(sub_825BF2C8)
  for (int i = 0; i < 2; i++) {
    GarageCar* car = guest::Element<GarageCar>(kGarageCars, i);
    g.Call(sub_82110370, car);  // GarageCar::Reset
    car->unknown_1964 = 0;
  }
}

// CarRecord::ResetBytes: r3 the record. Returns it.
extern "C" REX_FUNC(sub_820D06C0) {
  SBX_CAR_MAKERS_FALLBACK(sub_820D06C0)
  CarRecord* record = At<CarRecord>(ctx.r3.u32);
  for (int i = 0; i < 11; i++) {
    record->part_order[i] = kDefaultPartOrder[i];
  }
}
