// The tuning screen's debug steps as our own C++ (--native_tuning_screen,
// on by default): each answers the state it goes to.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_tuning_screen, true, "SBX",
                    "Our own C++ for the tuning screen's debug steps instead of the "
                    "recompiled functions");

namespace {

constexpr uint32_t kDebugLayerSetState = 700;
constexpr uint32_t kDebugEditState = 500;

bool On() { return REXCVAR_GET(native_tuning_screen); }

}  // namespace

#define SBX_TUNING_SCREEN_STEP(address, state) \
  extern "C" REX_FUNC(sub_##address) {         \
    REX_FUNC_PROLOGUE();                       \
    if (!On()) {                               \
      __imp__sub_##address(ctx, base);         \
      return;                                  \
    }                                          \
    ctx.r3.u64 = state;                        \
  }

SBX_TUNING_SCREEN_STEP(821FC1C0, kDebugLayerSetState)  // TuningScreen::DebugLayerSet
SBX_TUNING_SCREEN_STEP(821FC240, kDebugEditState)      // TuningScreen::DebugEditPosition
SBX_TUNING_SCREEN_STEP(821FC2B8, kDebugEditState)      // TuningScreen::DebugEditView

#undef SBX_TUNING_SCREEN_STEP
