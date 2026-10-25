// The race's post effects (ITC-Recomp's docs/functions/post-effects.md;
// addresses through docs/itc_to_sbx_functions.tsv, and SBX's post effect
// code sits 0x1528 below ITC's: Glow::Glow ITC 82138610 -> 821370E8,
// MotionBlur::MotionBlur 82136130 -> 82134C08, both "calls 0.90").
//
// The glow (kind 0, the bloom and star streaks) records its draws each
// frame in Glow::Record (sub_821364B0; ITC sub_821379D8, its vtable slot 0),
// reading its strength from +8452 (Glow::SetStrength, sub_821359F0: +8452 =
// f1, set by the speed each update) into the bloom combine's c33.y (pixel
// shader 9750599DF5ED3B39). --bloom=false records it with the strength at 0,
// so the combine adds nothing, as ITC's --glow does on the emulated GPU. It's
// game state, so it works with either renderer.

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(bloom, true, "SBX",
                    "The game's bloom (the glow over bright lights in races); false records "
                    "it with no strength");

namespace {
constexpr uint32_t kGlowStrength = 8452;  // float
}  // namespace

// Glow::Record: r3 = the glow.
extern "C" REX_FUNC(sub_821364B0) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(bloom)) {
    __imp__sub_821364B0(ctx, base);
    return;
  }
  uint32_t glow = ctx.r3.u32;
  uint32_t strength = guest::LoadU32(base, glow + kGlowStrength);
  guest::StoreU32(base, glow + kGlowStrength, 0);  // 0.0f
  __imp__sub_821364B0(ctx, base);
  guest::StoreU32(base, glow + kGlowStrength, strength);
}
