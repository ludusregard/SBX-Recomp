// The load polls' set-ups as our own C++ (--native_load_poll, on by
// default): the wheels' and lights' records and caliper texture, the
// parts', interior's, cockpit's and traffic's paints applied as their
// loads finish, the car model's ready check and the gauges' table. The
// polls themselves, the paints' and textures' own functions stay
// recompiled.

#include "game/scene_loads.h"
#include "game/scene_points.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_load_poll, true, "SBX",
                    "Our own C++ for the scene's records and paints set as the loads finish, and the car model's load checks, instead of the recompiled functions; the polls stay recompiled");

namespace {

using game::CarModel;
using game::CarModelRecord;
using game::SceneCockpit;
using game::SceneInterior;
using game::SceneLights;
using game::SceneParts;
using game::SceneTraffic;
using game::Wheels;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_load_poll); }

constexpr uint32_t kGauges = 0x820035E0;  // Read-only: 16 bytes an entry, two a gauge.

// A texture set on a model (sub_820D1420), r5 = -1.
void SetTexture(Guest& g, uint32_t model, uint32_t texture) {
  g.Call(sub_820D1420, model, texture, -1);
}

}  // namespace

#define SBX_LOAD_POLL_FALLBACK(name) \
  REX_FUNC_PROLOGUE();               \
  if (!On()) {                       \
    __imp__##name(ctx, base);        \
    return;                          \
  }                                  \
  Guest g(ctx, base);

// Wheels::SetRecord: r3 the wheels, r4 the record (156 bytes) copied in.
extern "C" REX_FUNC(sub_820D8B88) {
  SBX_LOAD_POLL_FALLBACK(sub_820D8B88)
  Wheels* wheels = At<Wheels>(ctx.r3.u32);
  g.Call(sub_8258CB90, &wheels->record, ctx.r4.u32, uint32_t(sizeof(wheels->record)));  // memcpy
}

// Wheels::SetCaliperTexture: r3 the wheels; the record's caliper texture
// set on the two brake models.
extern "C" REX_FUNC(sub_820D8B98) {
  SBX_LOAD_POLL_FALLBACK(sub_820D8B98)
  Wheels* wheels = At<Wheels>(ctx.r3.u32);
  for (auto& part : wheels->loads.parts_2144) {
    SetTexture(g, guest::AddressOf(&part), wheels->record.caliper_texture);
  }
}

// SceneLights::SetRecord: r3 the lights, r4 the record (52 bytes) copied
// in, then given to the lights' pool 4 object (Pool4Object::SetRecord).
extern "C" REX_FUNC(sub_820DC7E8) {
  SBX_LOAD_POLL_FALLBACK(sub_820DC7E8)
  SceneLights* lights = At<SceneLights>(ctx.r3.u32);
  const uint32_t record = ctx.r4.u32;
  g.Call(sub_8258CB90, &lights->record, record, uint32_t(sizeof(lights->record)));  // memcpy
  g.Call(sub_820D6710, lights->pool4_object, record);  // Pool4Object::SetRecord
}

// SceneLights::ApplyRecord: r3 the lights, r4 the record; the points'
// words cleared, SceneLights::SetRecord, then the record's texture (its
// first word) set on the lights' model.
extern "C" REX_FUNC(sub_820DC838) {
  SBX_LOAD_POLL_FALLBACK(sub_820DC838)
  SceneLights* lights = At<SceneLights>(ctx.r3.u32);
  for (u32_be& v : lights->points.unknown_8) {
    v = 0;
  }
  g.Call(sub_820DC7E8, lights);  // SceneLights::SetRecord (r4 as it came)
  SetTexture(g, lights->unknown_13152, lights->record.texture);
}

// The paints applied to each scene object's model (the paints' functions,
// recompiled): r3 the scene object.
extern "C" REX_FUNC(sub_820E04B0) {  // SceneParts::ApplyPaint
  SBX_LOAD_POLL_FALLBACK(sub_820E04B0)
  g.Call(sub_820D7038, At<SceneParts>(ctx.r3.u32)->unknown_104);
}

extern "C" REX_FUNC(sub_820E3E18) {  // SceneInterior::ApplyPaint
  SBX_LOAD_POLL_FALLBACK(sub_820E3E18)
  g.Call(sub_820D74F0, At<SceneInterior>(ctx.r3.u32)->unknown_24[3]);  // Its +36.
}

// SceneCockpit::ApplyPaint: the cockpit's own (sub_820D5FD0), then
// SceneInterior::ApplyPaint on the object at +28 with r4 to r9 as they came.
extern "C" REX_FUNC(sub_820DBDA8) {
  SBX_LOAD_POLL_FALLBACK(sub_820DBDA8)
  SceneCockpit* cockpit = At<SceneCockpit>(ctx.r3.u32);
  const uint64_t r4 = ctx.r4.u64, r5 = ctx.r5.u64, r6 = ctx.r6.u64, r7 = ctx.r7.u64,
                 r8 = ctx.r8.u64, r9 = ctx.r9.u64;
  g.Call(sub_820D5FD0, cockpit->unknown_32);
  g.Call(sub_820E3E18, cockpit->unknown_28, r4, r5, r6, r7, r8, r9);
}

extern "C" REX_FUNC(sub_820E5DC0) {  // SceneTraffic::ApplyPaint
  SBX_LOAD_POLL_FALLBACK(sub_820E5DC0)
  g.Call(sub_820D7AC8, At<SceneTraffic>(ctx.r3.u32)->unknown_9760);
}

// GaugeTableEntry: r3 the gauge, r4 the level; the gauge's first entry, or
// its second for levels 3 and up.
extern "C" REX_FUNC(sub_820D32A0) {
  SBX_LOAD_POLL_FALLBACK(sub_820D32A0)
  constexpr uint32_t kEntryBytes = 16;
  const uint32_t second = ctx.r4.s32 >= 3 ? 1 : 0;
  ctx.r3.u64 = uint32_t((ctx.r3.u32 * 2 + second) * kEntryBytes + kGauges);
}

// CarModel::IsReady: r3 the car model. CarModel::IsLoaded for a record of
// kind 0 or 1, else 0.
extern "C" REX_FUNC(sub_820D3CE8) {
  SBX_LOAD_POLL_FALLBACK(sub_820D3CE8)
  CarModel* model = At<CarModel>(ctx.r3.u32);
  if (At<CarModelRecord>(model->record_address)->kind > 1) {
    ctx.r3.u64 = 0;
    return;
  }
  g.Call(sub_820D38C8, model);  // CarModel::IsLoaded
}
