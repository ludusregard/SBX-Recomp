// The modes' tick helpers as our own C++ (--native_mode_tick, on by
// default): the screen fades' set-up and the players' input blocked.

#include "game/menu.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_mode_tick, true, "SBX",
                    "Our own C++ for the screen fades' set-up and the players' input blocked "
                    "instead of the recompiled functions");

namespace {

// The screen fades' 16 words.
constexpr uint32_t kScreenFades = 0x82694D8C;

bool On() { return REXCVAR_GET(native_mode_tick); }

}  // namespace

// ScreenFades::Init: the fades cleared. Returns r3.
extern "C" REX_FUNC(sub_8209CF08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8209CF08(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  for (int i = 0; i < 16; i++) {
    guest::At<guest::u32_be>(kScreenFades)[i] = 0;
  }
}

// ActionMappers::SetBlocked: r3 whether every player's input is blocked.
// Returns r3.
extern "C" REX_FUNC(sub_8209F450) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8209F450(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  for (int player = 0; player < game::kPlayers; player++) {
    guest::Element<game::MapperEntry>(game::kMapperEntries, player)->blocked = ctx.r3.s32;
  }
}
