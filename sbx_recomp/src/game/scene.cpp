// The scene's set-ups as our own C++ (--native_scene, on by default): the
// scene set-up's, the scene assets' and the car scene's constructors, the
// assets' destructor and release, a list's destructor and the pages freed
// in a range. The scene's updates and teardown stay recompiled.

#include "game/scene.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scene, true, "SBX",
                    "Our own C++ for the scene's constructors and destructors (its set-up, "
                    "assets, the car scene) instead of the recompiled functions; the scene's "
                    "updates and teardown stay recompiled");

namespace {

using game::AssetLoad;
using game::AssetRecord;
using game::CarScene;
using game::CarSceneEntry;
using game::LoadSlot;
using game::PartModel;
using game::SceneAssets;
using game::SceneSetUp;
using game::Vector;
using game::XPR;
using game::XPRList;
using guest::At;
using guest::Guest;
using guest::u16_be;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_scene); }

constexpr uint32_t kMemRelease = 0x8000;

// An XPR made as the assets' constructor makes it inline: its words 0.
void ClearXPR(XPR& xpr) {
  for (u32_be& word : xpr.words) {
    word = 0;
  }
}

void ConstructList(Guest& g, XPRList& list) { g.Call(sub_8257A150, &list, -1); }  // Reserved: 64.
void DestroyList(Guest& g, XPRList& list) { g.Call(sub_82579948, &list); }
void DestroyXPR(Guest& g, XPR& xpr) { g.Call(sub_8257CA60, &xpr); }
void ConstructPart(Guest& g, PartModel& part) { g.Call(sub_820CE6E8, &part); }
void DestroyPart(Guest& g, PartModel& part) { g.Call(sub_820CE750, &part); }
void FreeSlot(Guest& g, LoadSlot& slot) { g.Call(sub_820D0518, &slot); }  // LoadSlot::Free
void FreePages(Guest& g, u32_be& pages) { g.Call(sub_820D0750, &pages); }

void ClearRecord(AssetRecord& record) {
  for (uint8_t& byte : record.unknown_0) {
    byte = 0;
  }
  record.unknown_4[0] = 0;
  record.unknown_4[1] = 0;
  record.unknown_8 = 0;
  for (u16_be& half : record.unknown_10) {
    half = 0;
  }
  record.unknown_18[0] = 0;
  record.unknown_18[1] = 0;
}

// An asset load's XPR cleared and its list made.
void ConstructLoad(Guest& g, AssetLoad& load) {
  ClearXPR(load.xpr);
  ConstructList(g, load.list);
}

}  // namespace

#define SBX_SCENE_FALLBACK(name) \
  REX_FUNC_PROLOGUE();           \
  if (!On()) {                   \
    __imp__##name(ctx, base);    \
    return;                      \
  }                              \
  Guest g(ctx, base);

// SceneSetUp::SceneSetUp: r3 the set-up, returned.
extern "C" REX_FUNC(sub_820D0AE0) {
  SBX_SCENE_FALLBACK(sub_820D0AE0)
  SceneSetUp* setup = At<SceneSetUp>(ctx.r3.u32);
  setup->unknown_36[0] = 0;
  setup->unknown_36[1] = 0;
}

// FreePagesIn: r3 a word holding pages, freed (VirtualFree, sub_82292D70).
extern "C" REX_FUNC(sub_820D0750) {
  SBX_SCENE_FALLBACK(sub_820D0750)
  g.Call(sub_82292D70, *At<u32_be>(ctx.r3.u32), 0u, kMemRelease);
}

// SceneAssets::SceneAssets: r3 the assets, returned. The XPRs cleared, the
// lists and part models made, the records cleared.
extern "C" REX_FUNC(sub_820C4E00) {
  SBX_SCENE_FALLBACK(sub_820C4E00)
  const uint32_t address = ctx.r3.u32;
  SceneAssets* assets = At<SceneAssets>(address);
  ClearXPR(assets->xpr_24);
  for (XPR& xpr : assets->xprs_96) {
    ClearXPR(xpr);
  }
  for (XPRList& list : assets->lists_144) {
    ConstructList(g, list);
  }
  for (AssetLoad& load : assets->loads_184) {
    ConstructLoad(g, load);
  }
  ConstructPart(g, assets->part_416);
  ClearRecord(assets->record_928);
  for (AssetLoad& load : assets->loads_952) {
    ConstructLoad(g, load);
  }
  for (PartModel& part : assets->parts_1184) {
    ConstructPart(g, part);
  }
  for (AssetRecord& record : assets->records_3668) {
    ClearRecord(record);
  }
  ConstructPart(g, assets->part_4096);
  for (XPR& xpr : assets->xprs_4656) {
    ClearXPR(xpr);
  }
  for (XPRList& list : assets->lists_4752) {
    ConstructList(g, list);
  }
  ctx.r3.u64 = address;
}

// SceneAssets::~SceneAssets: its lists, XPRs and part models destroyed,
// last first.
extern "C" REX_FUNC(sub_820C4F58) {
  SBX_SCENE_FALLBACK(sub_820C4F58)
  SceneAssets* assets = At<SceneAssets>(ctx.r3.u32);
  for (int i = 3; i >= 0; i--) {
    DestroyList(g, assets->lists_4752[i]);
  }
  for (int i = 3; i >= 0; i--) {
    DestroyXPR(g, assets->xprs_4656[i]);
  }
  DestroyPart(g, assets->part_4096);
  for (int i = 3; i >= 0; i--) {
    DestroyPart(g, assets->parts_1184[i]);
  }
  for (int i = 1; i >= 0; i--) {
    DestroyList(g, assets->loads_952[i].list);
    DestroyXPR(g, assets->loads_952[i].xpr);
  }
  DestroyPart(g, assets->part_416);
  for (int i = 2; i >= 0; i--) {
    DestroyList(g, assets->loads_184[i].list);
    DestroyXPR(g, assets->loads_184[i].xpr);
  }
  DestroyList(g, assets->lists_144[1]);
  DestroyList(g, assets->lists_144[0]);
  DestroyXPR(g, assets->xprs_96[1]);
  DestroyXPR(g, assets->xprs_96[0]);
  DestroyXPR(g, assets->xpr_24);
}

// CarScene::CarScene: r3 the scene, returned. Its assets made, its
// entries cleared and its vectors' w = 1.0.
extern "C" REX_FUNC(sub_820C8760) {
  SBX_SCENE_FALLBACK(sub_820C8760)
  const uint32_t address = ctx.r3.u32;
  CarScene* scene = At<CarScene>(address);
  scene->unknown_40[0] = 0;
  scene->unknown_40[1] = 0;
  g.Call(sub_820C4E00, &scene->assets);
  for (CarSceneEntry& entry : scene->entries) {
    entry.unknown_0 = 0;
    entry.unknown_8 = 0;
  }
  for (Vector& v : scene->unknown_5120) {
    v.w = 1.0f;
  }
  ctx.r3.u64 = address;
}

// List::~List: sub_825795A8 on r3.
extern "C" REX_FUNC(sub_82579750) {
  SBX_SCENE_FALLBACK(sub_82579750)
  g.Call(sub_825795A8, ctx.r3.u32);
}

// SceneAssets::Release: its load slots freed and its pages released, then
// sub_820C4880 on the assets.
extern "C" REX_FUNC(sub_820C4A80) {
  SBX_SCENE_FALLBACK(sub_820C4A80)
  SceneAssets* assets = At<SceneAssets>(ctx.r3.u32);
  FreeSlot(g, assets->slot_0);
  FreeSlot(g, assets->slot_48);
  FreeSlot(g, assets->slot_72);
  for (AssetLoad& load : assets->loads_184) {
    FreeSlot(g, load.slot);
  }
  FreeSlot(g, assets->slot_388);
  FreeSlot(g, assets->slot_880);
  FreeSlot(g, assets->slot_904);
  FreePages(g, assets->pages_948);
  for (AssetLoad& load : assets->loads_952) {
    FreeSlot(g, load.slot);
  }
  FreeSlot(g, assets->slot_4048);
  FreeSlot(g, assets->slot_4072);
  g.Call(sub_8257A270, &assets->part_4096);  // Model::Release
  for (int i = 0; i < 4; i++) {
    FreeSlot(g, assets->slots_1088[i]);
    FreeSlot(g, assets->slots_3040[i]);
  }
  for (int i = 0; i < 19; i++) {
    FreeSlot(g, assets->slots_3136[i]);
    FreePages(g, assets->pages_3592[i]);
  }
  for (LoadSlot& slot : assets->slots_4560) {
    FreeSlot(g, slot);
  }
  g.Call(sub_820C4880, assets);
}
