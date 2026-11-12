// The scene loaders as our own C++ (--native_scene_loaders, on by
// default): the car model's, cockpit's, traffic's, wheels' and parts'
// loads' constructors and destructors, the pool 4 objects' and wheels'
// resets and releases, and the load checks (a car model, a pool 4 object,
// an XPR's count). The loads, the models' builds, the palettes and the
// searches stay recompiled.

#include "game/scene_loads.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scene_loaders, true, "SBX",
                    "Our own C++ for the scene loaders' constructors, destructors, resets and load checks instead of the recompiled functions; the loads themselves stay recompiled");

namespace {

using game::CarModel;
using game::CarModelLoads;
using game::CarParts;
using game::CarPartsLoads;
using game::CockpitLoads;
using game::LoadSlot;
using game::PartModel;
using game::Pool4Object;
using game::TrafficLoads;
using game::Vector;
using game::Wheels;
using game::WheelsLoads;
using game::XPR;
using game::XPRList;
using guest::At;
using guest::Guest;

bool On() { return REXCVAR_GET(native_scene_loaders); }

// Writable: copied into the wheels and pool 4 objects when they're reset.
constexpr uint32_t kResetValue = 0x826B2B20;

// The recompiled (or other files' native) functions these call.
void ConstructSkeleton(Guest& g, uint8_t* skeleton) { g.Call(sub_8257EEF0, skeleton); }
void DestroySkeleton(Guest& g, uint8_t* skeleton) { g.Call(sub_8257F180, skeleton); }
void ConstructXPR(Guest& g, XPR* xpr) { g.Call(sub_8257C780, xpr); }
void DestroyXPR(Guest& g, XPR* xpr) { g.Call(sub_8257CA60, xpr); }
void ConstructList(Guest& g, XPRList* list) { g.Call(sub_8257A150, list, -1); }  // Reserved size: 64.
void DestroyList(Guest& g, XPRList* list) { g.Call(sub_82579948, list); }
void ConstructPart(Guest& g, PartModel* part) { g.Call(sub_820CE6E8, part); }
void DestroyPart(Guest& g, PartModel* part) { g.Call(sub_820CE750, part); }
void ReleaseModel(Guest& g, PartModel* part) { g.Call(sub_8257A270, part); }  // Model::Release
void ClearPart(Guest& g, PartModel* part) { g.Call(sub_820CE788, part); }      // PartModel::Clear
void ResetSlot(Guest& g, LoadSlot* slot) { g.Call(sub_820D04F0, slot); }       // LoadSlot::Reset
void FreeSlot(Guest& g, LoadSlot* slot) { g.Call(sub_820D0518, slot); }        // LoadSlot::Free

template <size_t N>
void ConstructParts(Guest& g, PartModel (&parts)[N]) {
  for (PartModel& part : parts) {
    ConstructPart(g, &part);
  }
}

template <size_t N>
void DestroyParts(Guest& g, PartModel (&parts)[N]) {
  for (int i = int(N) - 1; i >= 0; i--) {
    DestroyPart(g, &parts[i]);
  }
}

// The skeleton, two XPRs and two XPR lists every model's loads start with.
void ConstructModelParts(Guest& g, uint8_t* skeleton, XPR (&xprs)[2], XPRList (&lists)[2]) {
  ConstructSkeleton(g, skeleton);
  ConstructXPR(g, &xprs[0]);
  ConstructXPR(g, &xprs[1]);
  ConstructList(g, &lists[0]);
  ConstructList(g, &lists[1]);
}

void DestroyModelParts(Guest& g, uint8_t* skeleton, XPR (&xprs)[2], XPRList (&lists)[2]) {
  DestroyList(g, &lists[1]);
  DestroyList(g, &lists[0]);
  DestroyXPR(g, &xprs[1]);
  DestroyXPR(g, &xprs[0]);
  DestroySkeleton(g, skeleton);
}

}  // namespace

#define SBX_SCENE_LOADERS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                   \
  if (!On()) {                           \
    __imp__##name(ctx, base);            \
    return;                              \
  }                                      \
  Guest g(ctx, base);

// CarModelLoads::CarModelLoads: r3 the loads. Returns them.
extern "C" REX_FUNC(sub_820D3A80) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D3A80)
  CarModelLoads* loads = At<CarModelLoads>(ctx.r3.u32);
  ConstructModelParts(g, loads->skeleton, loads->xprs, loads->lists);
  ctx.r3.u64 = guest::AddressOf(loads);
}

// CarModelLoads::~CarModelLoads: r3 the loads.
extern "C" REX_FUNC(sub_820D3AE0) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D3AE0)
  CarModelLoads* loads = At<CarModelLoads>(ctx.r3.u32);
  DestroyModelParts(g, loads->skeleton, loads->xprs, loads->lists);
}

// CockpitLoads::CockpitLoads: r3 the loads. Returns them.
extern "C" REX_FUNC(sub_820D5CB8) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D5CB8)
  CockpitLoads* loads = At<CockpitLoads>(ctx.r3.u32);
  ConstructModelParts(g, loads->model.skeleton, loads->model.xprs, loads->model.lists);
  ConstructParts(g, loads->parts);
  ctx.r3.u64 = guest::AddressOf(loads);
}

// CockpitLoads::~CockpitLoads: r3 the loads.
extern "C" REX_FUNC(sub_820D5D38) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D5D38)
  CockpitLoads* loads = At<CockpitLoads>(ctx.r3.u32);
  DestroyParts(g, loads->parts);
  DestroyModelParts(g, loads->model.skeleton, loads->model.xprs, loads->model.lists);
}

// TrafficLoads::TrafficLoads: r3 the loads. Returns them.
extern "C" REX_FUNC(sub_820D78F0) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D78F0)
  TrafficLoads* loads = At<TrafficLoads>(ctx.r3.u32);
  ConstructModelParts(g, loads->skeleton, loads->xprs, loads->lists);
  ConstructParts(g, loads->parts);
  ctx.r3.u64 = guest::AddressOf(loads);
}

// TrafficLoads::~TrafficLoads: r3 the loads.
extern "C" REX_FUNC(sub_820D7970) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D7970)
  TrafficLoads* loads = At<TrafficLoads>(ctx.r3.u32);
  DestroyParts(g, loads->parts);
  DestroyModelParts(g, loads->skeleton, loads->xprs, loads->lists);
}

// WheelsLoads::WheelsLoads: r3 the loads. Returns them.
extern "C" REX_FUNC(sub_820D8C28) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D8C28)
  WheelsLoads* loads = At<WheelsLoads>(ctx.r3.u32);
  ConstructParts(g, loads->parts_48);
  ConstructParts(g, loads->parts_1024);
  ConstructXPR(g, &loads->xprs[0]);
  ConstructXPR(g, &loads->xprs[1]);
  ConstructList(g, &loads->lists[0]);
  ConstructList(g, &loads->lists[1]);
  ConstructParts(g, loads->parts_2144);
  ctx.r3.u64 = guest::AddressOf(loads);
}

// WheelsLoads::~WheelsLoads: r3 the loads.
extern "C" REX_FUNC(sub_820D8CD8) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D8CD8)
  WheelsLoads* loads = At<WheelsLoads>(ctx.r3.u32);
  DestroyParts(g, loads->parts_2144);
  DestroyList(g, &loads->lists[1]);
  DestroyList(g, &loads->lists[0]);
  DestroyXPR(g, &loads->xprs[1]);
  DestroyXPR(g, &loads->xprs[0]);
  DestroyParts(g, loads->parts_1024);
  DestroyParts(g, loads->parts_48);
}

// CarPartsLoads::CarPartsLoads: r3 the loads. Returns them.
extern "C" REX_FUNC(sub_820D6C38) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D6C38)
  CarPartsLoads* loads = At<CarPartsLoads>(ctx.r3.u32);
  ConstructParts(g, loads->parts);
  for (Vector& point : loads->points) {
    point.w = 1.0f;
  }
  ctx.r3.u64 = guest::AddressOf(loads);
}

// CarPartsLoads::~CarPartsLoads: r3 the loads.
extern "C" REX_FUNC(sub_820D6C98) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D6C98)
  DestroyParts(g, At<CarPartsLoads>(ctx.r3.u32)->parts);
}

// CarParts::Release: r3 the parts; each part's slot freed and its model
// released and cleared.
extern "C" REX_FUNC(sub_820D69B0) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D69B0)
  CarPartsLoads& loads = At<CarParts>(ctx.r3.u32)->loads;
  for (int i = 0; i < CarPartsLoads::kParts; i++) {
    FreeSlot(g, &loads.slots[i]);
    ReleaseModel(g, &loads.parts[i]);
    ClearPart(g, &loads.parts[i]);
  }
}

// Wheels::Reset: r3 the wheels.
extern "C" REX_FUNC(sub_820D8810) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D8810)
  Wheels* wheels = At<Wheels>(ctx.r3.u32);
  wheels->unknown_24 = 0;
  wheels->unknown_3268 = *At<guest::u32_be>(kResetValue);
  g.Call(sub_820D3548, &wheels->record);  // WheelsRecord::Reset
  g.Call(sub_820D8760, &wheels->loads);   // the loads' slots and models reset
}

// Wheels::Release: r3 the wheels; their slots freed, then Wheels::Reset.
extern "C" REX_FUNC(sub_820D8B30) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D8B30)
  Wheels* wheels = At<Wheels>(ctx.r3.u32);
  WheelsLoads& loads = wheels->loads;
  for (int i = 0; i < 2; i++) {
    FreeSlot(g, &loads.slots_0[i]);
    FreeSlot(g, &loads.slots_976[i]);
    FreeSlot(g, &loads.slots_1952[i]);
    FreeSlot(g, &loads.slots_2088[i]);
  }
  g.Call(sub_820D8810, wheels);  // Wheels::Reset
}

// Pool4Object::Reset: r3 the object.
extern "C" REX_FUNC(sub_820D65F8) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D65F8)
  Pool4Object* object = At<Pool4Object>(ctx.r3.u32);
  object->unknown_24 = 0;
  g.Call(sub_820D3128, &object->record);  // Pool4Record::Reset
  ResetSlot(g, &object->slot);
  ReleaseModel(g, &object->model);
  ClearPart(g, &object->model);
  object->unknown_576 = *At<guest::u32_be>(kResetValue);
}

// Pool4Object::Release: r3 the object; its slot freed and model released,
// then Pool4Object::Reset.
extern "C" REX_FUNC(sub_820D66D0) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D66D0)
  Pool4Object* object = At<Pool4Object>(ctx.r3.u32);
  FreeSlot(g, &object->slot);
  ReleaseModel(g, &object->model);
  g.Call(sub_820D65F8, object);  // Pool4Object::Reset
}

// Pool4Object::SetRecord: r3 the object, r4 the record (52 bytes) copied in.
extern "C" REX_FUNC(sub_820D6710) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D6710)
  Pool4Object* object = At<Pool4Object>(ctx.r3.u32);
  g.Call(sub_8258CB90, &object->record, ctx.r4.u32, uint32_t(sizeof(game::Pool4Record)));  // memcpy
}

// CarModel::IsLoaded: r3 the car model. 0 while either of its two loads
// is loading; otherwise whether its own load isn't.
extern "C" REX_FUNC(sub_820D38C8) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D38C8)
  CarModel* model = At<CarModel>(ctx.r3.u32);
  bool loaded = true;
  for (const LoadSlot& slot : model->slots) {
    if (slot.state == LoadSlot::kLoading) {
      loaded = false;
      break;
    }
  }
  if (loaded) {
    loaded = model->slot.state != LoadSlot::kLoading;
  }
  ctx.r3.u64 = loaded ? 1 : 0;
}

// Pool4Object::IsLoaded: r3 the object. Whether its load isn't loading.
extern "C" REX_FUNC(sub_820D66B8) {
  SBX_SCENE_LOADERS_FALLBACK(sub_820D66B8)
  ctx.r3.u64 = At<Pool4Object>(ctx.r3.u32)->slot.state != LoadSlot::kLoading ? 1 : 0;
}

// XPR::Count: r3 the XPR. Its count (word 5), or 0 without a header
// (word 4).
extern "C" REX_FUNC(sub_8257C838) {
  SBX_SCENE_LOADERS_FALLBACK(sub_8257C838)
  XPR* xpr = At<XPR>(ctx.r3.u32);
  ctx.r3.u64 = xpr->words[4] == 0 ? 0 : uint32_t(xpr->words[5]);
}
