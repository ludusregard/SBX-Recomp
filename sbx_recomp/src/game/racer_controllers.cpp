// The racers' controllers as our own C++ (--native_racer_controllers, on by
// default): the cars' input records freed, and three of the race flow's
// state steps.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_racer_controllers, true, "SBX",
                    "Our own C++ for the cars' input records' teardown and three race flow "
                    "steps instead of the recompiled functions");

namespace {

// The cars' input records (one a car, seven), each a block on the heap.
constexpr uint32_t kCarInputs = 0x826B5810;
constexpr int kCarInputCount = 7;

bool On() { return REXCVAR_GET(native_racer_controllers); }

}  // namespace

#define SBX_CONTROLLERS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                   \
  if (!On()) {                           \
    __imp__##name(ctx, base);            \
    return;                              \
  }                                      \
  guest::Guest g(ctx, base);

// CarInputs::Destroy: each car's input record deleted.
extern "C" REX_FUNC(sub_82102488) {
  SBX_CONTROLLERS_FALLBACK(sub_82102488)
  for (int i = 0; i < kCarInputCount; i++) {
    g.Call(sub_8258C868, guest::At<guest::u32_be>(kCarInputs)[i]);  // operator delete
  }
}

// RaceFlow::OnEnd: the cars' input records freed.
extern "C" REX_FUNC(sub_8215D538) {
  SBX_CONTROLLERS_FALLBACK(sub_8215D538)
  g.Call(sub_82102488, ctx.r3.u32);  // CarInputs::Destroy
}

// RaceFlow::OnHold and OnState7: r3 the race flow, passed on (recompiled).
extern "C" REX_FUNC(sub_8215DAE0) {
  SBX_CONTROLLERS_FALLBACK(sub_8215DAE0)
  g.Call(sub_8215D960, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_8215E808) {
  SBX_CONTROLLERS_FALLBACK(sub_8215E808)
  g.Call(sub_8215E598, ctx.r3.u32);
}
