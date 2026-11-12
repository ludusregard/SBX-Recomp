// The boot mode's screen machine as our own C++ (--native_boot_mode, on by
// default): its constructor.

#include "game/game_modes.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_boot_mode, true, "SBX",
                    "Our own C++ for the boot mode's screen machine constructor instead of "
                    "the recompiled function");

namespace {

// A state machine with eight more words of its own.
struct ScreenMachine {
  game::StateMachine machine;
  guest::u32_be unknown_9608[8];
};

}  // namespace

// ScreenMachine::ScreenMachine: r3 the machine; built and emptied.
// Returns it.
extern "C" REX_FUNC(sub_82107480) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_boot_mode)) {
    __imp__sub_82107480(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  ScreenMachine* machine = guest::At<ScreenMachine>(ctx.r3.u32);
  g.Call(sub_82107230, &machine->machine);  // StateMachine::StateMachine
  machine->machine.count = 0;
  machine->machine.state = 0;
  for (guest::u32_be& word : machine->unknown_9608) {
    word = 0;
  }
  ctx.r3.u64 = guest::AddressOf(machine);
}
