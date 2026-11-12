// The cars' wheel models on the stands as our own C++
// (--native_stand_models, on by default): the tires' textures and grades
// set, and the parts' light value.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_stand_models, true, "SBX",
                    "Our own C++ for the wheel models' tire textures and grades and the "
                    "parts' light value instead of the recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u32_be;

// A car's wheel models: the front and rear tires and their materials.
struct WheelModels {
  uint8_t unknown_0[40];
  u32_be front_tire;
  uint8_t unknown_44[12];
  u32_be rear_tire;
  uint8_t unknown_60[180];
  uint8_t front_material[464];
  uint8_t rear_material[4];
};
GUEST_OFFSET(WheelModels, front_material, 240);
GUEST_OFFSET(WheelModels, rear_material, 704);

struct SceneWheels {
  uint8_t unknown_0[28];
  guest::ptr<WheelModels> models;
  uint8_t tires[12];  // The tires' set-up, passed on whole.
  u32_be front_grade;
  uint8_t unknown_48[12];
  u32_be rear_grade;
};
GUEST_OFFSET(SceneWheels, front_grade, 44);
GUEST_OFFSET(SceneWheels, rear_grade, 60);

struct SceneParts {
  uint8_t unknown_0[27664];
  guest::f32_be light_value;
};

bool On() { return REXCVAR_GET(native_stand_models); }

// Material::SetTexture (recompiled), for every pass (-1).
void SetTireTexture(Guest& g, uint8_t* material, uint32_t tire) {
  g.Call(sub_820D1420, material, tire, -1);
}

}  // namespace

#define SBX_STAND_MODELS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                    \
  if (!On()) {                            \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// Wheels::SetTireTextures: r3 the wheel models.
extern "C" REX_FUNC(sub_820D8BE0) {
  SBX_STAND_MODELS_FALLBACK(sub_820D8BE0)
  WheelModels* models = At<WheelModels>(ctx.r3.u32);
  SetTireTexture(g, models->front_material, models->front_tire);
  SetTireTexture(g, models->rear_material, models->rear_tire);
}

// SceneWheels::SetTireGrades: r3 the scene's wheels, r4 and r5 the front
// and rear grades. The models' tires set from them, then their textures.
extern "C" REX_FUNC(sub_820E7058) {
  SBX_STAND_MODELS_FALLBACK(sub_820E7058)
  SceneWheels* wheels = At<SceneWheels>(ctx.r3.u32);
  wheels->front_grade = ctx.r4.u32;
  wheels->rear_grade = ctx.r5.u32;
  g.Call(sub_820D8B88, wheels->models, guest::AddressOf(wheels->tires));  // Wheels::SetTires
  g.Call(sub_820D8BE0, wheels->models);                           // Wheels::SetTireTextures
}

// SceneParts::LightValue: r3 the parts; the value in f1.
extern "C" REX_FUNC(sub_820DEF68) {
  SBX_STAND_MODELS_FALLBACK(sub_820DEF68)
  ctx.fpscr.disableFlushMode();
  ctx.f1.f64 = double(float(At<SceneParts>(ctx.r3.u32)->light_value));
}
