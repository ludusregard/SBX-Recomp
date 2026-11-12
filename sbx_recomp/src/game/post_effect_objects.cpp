// The post effects' objects as our own C++ (--native_post_effects, on by
// default): the glare's definition built, and the color filter's, motion
// blur's and glow's resources released when they're destroyed. The effects'
// set-ups and draws stay recompiled (src/hooks/post_effects.cpp is SBX's
// own --bloom hook on them).

#include "game/math.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_post_effects, true, "SBX",
                    "Our own C++ for the post effects' glare definition and their destructors "
                    "instead of the recompiled functions");

namespace {

using game::Vector;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kPostEffectVtable = 0x82008794;  // The effects' base.
constexpr uint32_t kColorFilterVtable = 0x820087B4;
constexpr uint32_t kMotionBlurVtable = 0x820087F8;
constexpr uint32_t kGlowVtable = 0x82008800;

// An effect's resources: textures (released by sub_82131EB0) and render
// targets (sub_82132738), kept after its 8064 bytes of settings.
struct PostEffect {
  u32_be vtable;
  uint8_t unknown_4[8060];
  u32_be resources[32];
};
GUEST_OFFSET(PostEffect, resources, 8064);

// The glow keeps more settings, and its resources after them.
struct Glow {
  u32_be vtable;
  uint8_t unknown_4[8348];
  u32_be textures[15];
  u32_be targets[6];
};
GUEST_OFFSET(Glow, textures, 8352);
GUEST_OFFSET(Glow, targets, 8412);

struct GlareDef {
  f32_be unknown_0[3];
  uint8_t unknown_12[4];
  f32_be unknown_16[6];
  uint8_t unknown_40[8];
  u32_be unknown_48;
  uint8_t unknown_52[96];
  f32_be unknown_148;
  u32_be unknown_152;
  uint8_t unknown_156[4];
  Vector points[8];
};
GUEST_OFFSET(GlareDef, unknown_148, 148);
GUEST_OFFSET(GlareDef, points, 160);

bool On() { return REXCVAR_GET(native_post_effects); }

void ReleaseTexture(Guest& g, u32_be& texture) { g.Call(sub_82131EB0, &texture); }
void ReleaseTarget(Guest& g, u32_be& target) { g.Call(sub_82132738, &target); }

}  // namespace

#define SBX_POST_EFFECTS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                    \
  if (!On()) {                            \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// GlareDef::GlareDef: r3 the definition. Returns it.
extern "C" REX_FUNC(sub_8212DAC8) {
  SBX_POST_EFFECTS_FALLBACK(sub_8212DAC8)
  GlareDef* glare = At<GlareDef>(ctx.r3.u32);
  for (Vector& point : glare->points) {
    point.w = 1.0f;
  }
  glare->unknown_148 = 0.0f;
  glare->unknown_48 = 0;
  glare->unknown_152 = 0;
  for (f32_be& v : glare->unknown_0) {
    v = 0.0f;
  }
  for (f32_be& v : glare->unknown_16) {
    v = 0.0f;
  }
}

// ColorFilter::~ColorFilter: r3 the filter; its texture released.
extern "C" REX_FUNC(sub_82133C80) {
  SBX_POST_EFFECTS_FALLBACK(sub_82133C80)
  PostEffect* filter = At<PostEffect>(ctx.r3.u32);
  filter->vtable = kColorFilterVtable;
  ReleaseTexture(g, filter->resources[0]);
  filter->vtable = kPostEffectVtable;
}

// MotionBlur::~MotionBlur: r3 the blur; two targets and two textures
// released, the last first.
extern "C" REX_FUNC(sub_82134E68) {
  SBX_POST_EFFECTS_FALLBACK(sub_82134E68)
  PostEffect* blur = At<PostEffect>(ctx.r3.u32);
  blur->vtable = kMotionBlurVtable;
  ReleaseTarget(g, blur->resources[3]);
  ReleaseTarget(g, blur->resources[2]);
  ReleaseTexture(g, blur->resources[1]);
  ReleaseTexture(g, blur->resources[0]);
  blur->vtable = kPostEffectVtable;
}

// Glow::~Glow: r3 the glow; its targets and textures released, the last
// first.
extern "C" REX_FUNC(sub_82135908) {
  SBX_POST_EFFECTS_FALLBACK(sub_82135908)
  Glow* glow = At<Glow>(ctx.r3.u32);
  glow->vtable = kGlowVtable;
  for (int i = 5; i >= 0; i--) {
    ReleaseTarget(g, glow->targets[i]);
  }
  for (int i = 14; i >= 0; i--) {
    ReleaseTexture(g, glow->textures[i]);
  }
  glow->vtable = kPostEffectVtable;
}
