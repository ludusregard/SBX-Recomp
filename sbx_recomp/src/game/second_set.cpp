// The scene parts' point tables as our own C++ (--native_second_set, on by
// default): the point records' constructors and destructors for the car
// body's parts, the cockpit, the interior, the lights, the traffic cars and
// the wheels, their resets and finds (each record found by node or shape
// name in a model), a point record's node and shape names, the models'
// stream getters and the morph's run. The groups' defaults (which name the
// shapes), the finds' searches, the models' draws and the morph's blend
// stay recompiled.

#include "game/model.h"
#include "game/scene_points.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_second_set, true, "SBX",
                    "Our own C++ for the scene parts' point tables (their defaults and constructors), the models' streams and the morph's run instead of the recompiled functions; the models' draws stay recompiled");

namespace {

using game::BrakePoints;
using game::CockpitPoints;
using game::DoorPoints;
using game::HeadlightPoints;
using game::InteriorPoints;
using game::LightPoints;
using game::Model;
using game::Morph;
using game::MufflerPoints;
using game::PartsPoints;
using game::PointGroup;
using game::PointRecord;
using game::RimPoints;
using game::SceneCockpit;
using game::SceneInterior;
using game::SceneLights;
using game::SceneModelBlock;
using game::SceneParts;
using game::SceneTraffic;
using game::SceneWheels;
using game::TirePoints;
using game::TrafficPoints;
using game::Vector;
using game::WheelPoints;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_second_set); }

// Writable: copied into each scene object when it's reset.
constexpr uint32_t kResetValue = 0x826B2B24;

// How near two found points are taken to be one (sub_820D16B0's f1).
constexpr double kPartsLinkDistance = 5.0;
constexpr double kNearLinkDistance = 1.0;  // The cockpit's and the lights' finds.
constexpr double kTrafficLinkDistance = 3.0;

void SetW(Vector& v) { v.w = 1.0f; }

// PointRecord::PointRecord (ours below) on each, in order.
template <size_t N>
void ConstructRecords(Guest& g, PointRecord (&records)[N]) {
  for (PointRecord& record : records) {
    g.Call(sub_820D0030, &record);
  }
}

// The records' destructor (an empty one) on each, the last first.
void DestroyRecord(Guest& g, PointRecord& record) { g.Call(sub_821611F0, &record); }

template <size_t N>
void DestroyRecords(Guest& g, PointRecord (&records)[N]) {
  for (int i = int(N) - 1; i >= 0; i--) {
    DestroyRecord(g, records[i]);
  }
}

// A group's records made, then its defaults (a recompiled function that
// names their shapes) on the group.
template <size_t N>
void ConstructGroup(Guest& g, PointRecord (&records)[N], Guest::Function* defaults) {
  ConstructRecords(g, records);
  g.Call(defaults, records);
}

template <int N>
void ConstructGroup(Guest& g, PointGroup<N>& group, Guest::Function* defaults) {
  ConstructRecords(g, group.records);
  g.Call(defaults, &group);
}

// A record found in a model (r4 of the finds): by its node name or its
// shape name.
void FindByNode(Guest& g, PointRecord& record, uint32_t model) { g.Call(sub_820D0090, &record, model); }
void FindByShape(Guest& g, PointRecord& record, uint32_t model) { g.Call(sub_820CE8F0, &record, model); }

// The first `nodes` records found by node, the rest by shape.
void FindRecords(Guest& g, PointRecord* records, int count, int nodes, uint32_t model) {
  for (int i = 0; i < count; i++) {
    if (i < nodes) {
      FindByNode(g, records[i], model);
    } else {
      FindByShape(g, records[i], model);
    }
  }
}

// sub_820D16B0 on what a find left in the record, with a distance in f1.
void Link(Guest& g, const PointRecord& record, double distance) {
  g.ctx.f1.f64 = distance;
  g.Call(sub_820D16B0, record.unknown_4, record.unknown_228);
}

void ResetValue(u32_be& to) { to = *At<u32_be>(kResetValue); }

}  // namespace

#define SBX_SECOND_SET_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                \
  if (!On()) {                        \
    __imp__##name(ctx, base);         \
    return;                           \
  }                                   \
  Guest g(ctx, base);

// --- Point records ---

// PointRecord::PointRecord: r3 the record. Returns it.
extern "C" REX_FUNC(sub_820D0030) {
  SBX_SECOND_SET_FALLBACK(sub_820D0030)
  PointRecord* record = At<PointRecord>(ctx.r3.u32);
  for (Vector& point : record->points) {
    SetW(point);
  }
  g.Call(sub_820CE7C8, record);  // PointRecord::Clear
  ctx.r3.u64 = guest::AddressOf(record);
}

// PointRecord::SetNode and SetShape: r3 the record, r4 the name copied in
// (with its terminator).
extern "C" REX_FUNC(sub_820CE698) {
  SBX_SECOND_SET_FALLBACK(sub_820CE698)
  char* to = At<PointRecord>(ctx.r3.u32)->node;
  for (const char* from = At<char>(ctx.r4.u32); (*to++ = *from++) != 0;) {
  }
}

extern "C" REX_FUNC(sub_820CE6C0) {
  SBX_SECOND_SET_FALLBACK(sub_820CE6C0)
  char* to = At<PointRecord>(ctx.r3.u32)->shape;
  for (const char* from = At<char>(ctx.r4.u32); (*to++ = *from++) != 0;) {
  }
}

// --- The car body's parts ---

// PartsPoints::InitLeftDoor and InitRightDoor: r3 the door. Returns it.
extern "C" REX_FUNC(sub_820DF160) {
  SBX_SECOND_SET_FALLBACK(sub_820DF160)
  DoorPoints* door = At<DoorPoints>(ctx.r3.u32);
  ConstructRecords(g, door->records);
  for (Vector& point : door->points) {
    SetW(point);
  }
  g.Call(sub_820DE3C8, door);  // the left door's defaults
  ctx.r3.u64 = guest::AddressOf(door);
}

extern "C" REX_FUNC(sub_820DF1B8) {
  SBX_SECOND_SET_FALLBACK(sub_820DF1B8)
  DoorPoints* door = At<DoorPoints>(ctx.r3.u32);
  ConstructRecords(g, door->records);
  for (Vector& point : door->points) {
    SetW(point);
  }
  g.Call(sub_820DE638, door);  // the right door's defaults
  ctx.r3.u64 = guest::AddressOf(door);
}

// PartsPoints::InitHeadlights: r3 the headlights. Returns them.
extern "C" REX_FUNC(sub_820E0F48) {
  SBX_SECOND_SET_FALLBACK(sub_820E0F48)
  HeadlightPoints* lights = At<HeadlightPoints>(ctx.r3.u32);
  ConstructRecords(g, lights->records);
  for (Vector& point : lights->points) {
    SetW(point);
  }
  g.Call(sub_820DF2E8, lights);  // their defaults
  ctx.r3.u64 = guest::AddressOf(lights);
}

// PartsPoints::InitMufflers: r3 the mufflers. Returns them.
extern "C" REX_FUNC(sub_820DFBB8) {
  SBX_SECOND_SET_FALLBACK(sub_820DFBB8)
  MufflerPoints* mufflers = At<MufflerPoints>(ctx.r3.u32);
  for (Vector& point : mufflers->points) {
    SetW(point);
  }
  ConstructRecords(g, mufflers->records);
  g.Call(sub_820DEE88, mufflers);  // their defaults
  ctx.r3.u64 = guest::AddressOf(mufflers);
}

// PartsPoints::PartsPoints: r3 the points. Each group made and given its
// defaults. Returns them.
extern "C" REX_FUNC(sub_820E25D8) {
  SBX_SECOND_SET_FALLBACK(sub_820E25D8)
  PartsPoints* parts = At<PartsPoints>(ctx.r3.u32);
  ConstructGroup(g, parts->front_bumper, sub_820DDD58);
  g.Call(sub_820DF0D0, &parts->cooler);
  ConstructGroup(g, parts->rear_bumper, sub_820DDFF8);
  g.Call(sub_820DF118, &parts->side_skirts);
  ConstructGroup(g, parts->rear_spoiler, sub_820DE260);
  ConstructGroup(g, parts->bonnet, sub_820DE310);
  g.Call(sub_820DF160, &parts->left_door);   // PartsPoints::InitLeftDoor
  g.Call(sub_820DF1B8, &parts->right_door);  // PartsPoints::InitRightDoor
  ConstructGroup(g, parts->doors, sub_820DE798);
  g.Call(sub_820DF210, &parts->unknown_23760);
  g.Call(sub_820DF258, &parts->unknown_24000);
  g.Call(sub_820DF2A0, &parts->unknown_24240);
  g.Call(sub_820E0F48, &parts->headlights);  // PartsPoints::InitHeadlights
  ConstructGroup(g, parts->tail_lights, sub_820DE9B8);
  ConstructGroup(g, parts->fenders, sub_820DEA98);
  ConstructGroup(g, parts->cabin, sub_820DEC50);
  ConstructGroup(g, parts->engine, sub_820DEDE8);
  g.Call(sub_820DFBB8, &parts->mufflers);  // PartsPoints::InitMufflers
  ConstructGroup(g, parts->shadows, sub_820DEEF0);
  ctx.r3.u64 = guest::AddressOf(parts);
}

// PartsPoints::~PartsPoints: r3 the points; every record destroyed, the
// last first.
extern "C" REX_FUNC(sub_820E27D8) {
  SBX_SECOND_SET_FALLBACK(sub_820E27D8)
  PartsPoints* parts = At<PartsPoints>(ctx.r3.u32);
  DestroyRecords(g, parts->shadows);
  DestroyRecords(g, parts->mufflers.records);
  DestroyRecords(g, parts->engine);
  DestroyRecords(g, parts->cabin);
  DestroyRecords(g, parts->fenders);
  DestroyRecords(g, parts->tail_lights);
  DestroyRecords(g, parts->headlights.records);
  DestroyRecord(g, parts->unknown_24240);
  DestroyRecord(g, parts->unknown_24000);
  DestroyRecord(g, parts->unknown_23760);
  DestroyRecords(g, parts->doors);
  DestroyRecords(g, parts->right_door.records);
  DestroyRecords(g, parts->left_door.records);
  DestroyRecords(g, parts->bonnet);
  DestroyRecords(g, parts->rear_spoiler);
  DestroyRecord(g, parts->side_skirts);
  DestroyRecords(g, parts->rear_bumper);
  DestroyRecord(g, parts->cooler);
  DestroyRecords(g, parts->front_bumper);
}

// The car body's finds: r3 the group's first record, r4 the model.
extern "C" REX_FUNC(sub_820DE2B0) {  // PartsPoints::FindRearSpoiler
  SBX_SECOND_SET_FALLBACK(sub_820DE2B0)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 3, 0, ctx.r4.u32);
  Link(g, records[2], kPartsLinkDistance);
  Link(g, records[1], kPartsLinkDistance);
}

extern "C" REX_FUNC(sub_820DE378) {  // PartsPoints::FindBonnet
  SBX_SECOND_SET_FALLBACK(sub_820DE378)
  FindRecords(g, At<PointRecord>(ctx.r3.u32), 2, 1, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_820DE430) {  // PartsPoints::FindLeftDoor
  SBX_SECOND_SET_FALLBACK(sub_820DE430)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 25, 2, ctx.r4.u32);
  for (int i = 7; i < 11; i++) {
    Link(g, records[i], kPartsLinkDistance);
  }
}

extern "C" REX_FUNC(sub_820DE6C8) {  // PartsPoints::FindRightDoor
  SBX_SECOND_SET_FALLBACK(sub_820DE6C8)
  FindRecords(g, At<PointRecord>(ctx.r3.u32), 25, 2, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_820DE7E8) {  // PartsPoints::FindDoors
  SBX_SECOND_SET_FALLBACK(sub_820DE7E8)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 23, 0, ctx.r4.u32);
  for (int i = 3; i < 11; i++) {
    Link(g, records[i], kPartsLinkDistance);
  }
}

extern "C" REX_FUNC(sub_820DEA20) {  // PartsPoints::FindTailLights
  SBX_SECOND_SET_FALLBACK(sub_820DEA20)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 15, 8, ctx.r4.u32);
  for (int i = 8; i < 13; i++) {
    Link(g, records[i], kPartsLinkDistance);
  }
}

extern "C" REX_FUNC(sub_820DEAE8) {  // PartsPoints::FindFenders
  SBX_SECOND_SET_FALLBACK(sub_820DEAE8)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 7, 0, ctx.r4.u32);
  Link(g, records[3], kPartsLinkDistance);
  Link(g, records[4], kPartsLinkDistance);
}

extern "C" REX_FUNC(sub_820DECA0) {  // PartsPoints::FindCabin
  SBX_SECOND_SET_FALLBACK(sub_820DECA0)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 5, 0, ctx.r4.u32);
  Link(g, records[0], kPartsLinkDistance);
  Link(g, records[1], kPartsLinkDistance);
}

// SceneParts::Reset: r3 the scene parts.
extern "C" REX_FUNC(sub_820E00B8) {
  SBX_SECOND_SET_FALLBACK(sub_820E00B8)
  SceneParts* scene = At<SceneParts>(ctx.r3.u32);
  g.Call(sub_820D31A0, &scene->record);
  g.Call(sub_820DFC98, &scene->points);  // the parts' defaults
  scene->unknown_24 = 0;
  scene->unknown_28 = 0;
  scene->unknown_104 = 0;
  scene->unknown_37280 = 0;
  scene->unknown_100 = 0;
  ResetValue(scene->unknown_37284);
  g.Call(sub_8258C950, scene->unknown_32, 0u, uint32_t(sizeof(scene->unknown_32)));  // memset
}

// --- The cockpit ---

// CockpitPoints::CockpitPoints: r3 the points. Returns them.
extern "C" REX_FUNC(sub_820DBE00) {
  SBX_SECOND_SET_FALLBACK(sub_820DBE00)
  CockpitPoints* cockpit = At<CockpitPoints>(ctx.r3.u32);
  ConstructGroup(g, cockpit->records_0, sub_820DB4E8);
  ConstructGroup(g, cockpit->records_720, sub_820DB5A0);
  g.Call(sub_820DBD18, &cockpit->record_1200);
  ConstructGroup(g, cockpit->records_1440, sub_820DB658);
  g.Call(sub_820DBD60, &cockpit->record_1920);
  ConstructGroup(g, cockpit->records_2160, sub_820DB710);
  ctx.r3.u64 = guest::AddressOf(cockpit);
}

// CockpitPoints::~CockpitPoints: r3 the points.
extern "C" REX_FUNC(sub_820DBEC8) {
  SBX_SECOND_SET_FALLBACK(sub_820DBEC8)
  CockpitPoints* cockpit = At<CockpitPoints>(ctx.r3.u32);
  DestroyRecords(g, cockpit->records_2160);
  DestroyRecord(g, cockpit->record_1920);
  DestroyRecords(g, cockpit->records_1440);
  DestroyRecord(g, cockpit->record_1200);
  DestroyRecords(g, cockpit->records_720);
  DestroyRecords(g, cockpit->records_0);
}

// CockpitPoints::FindHeadlights: r3 the first of 8 records, r4 the model.
extern "C" REX_FUNC(sub_820DB778) {
  SBX_SECOND_SET_FALLBACK(sub_820DB778)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 8, 4, ctx.r4.u32);
  Link(g, records[5], kNearLinkDistance);
  Link(g, records[6], kNearLinkDistance);
}

// SceneCockpit::Reset: r3 the scene cockpit.
extern "C" REX_FUNC(sub_820DBA90) {
  SBX_SECOND_SET_FALLBACK(sub_820DBA90)
  SceneCockpit* scene = At<SceneCockpit>(ctx.r3.u32);
  scene->unknown_24 = 0;
  scene->unknown_28 = 0;
  scene->unknown_32 = 0;
  scene->unknown_4312 = 0;
  ResetValue(scene->unknown_4316);
  g.Call(sub_820D3100, &scene->record);
  g.Call(sub_820DB880, &scene->points);  // the cockpit's defaults
  scene->record_address = guest::AddressOf(&scene->record);
  for (u32_be& v : scene->unknown_4288) {
    v = 0;
  }
}

// --- The interior ---

// InteriorPoints::InteriorPoints: r3 the points. Returns them.
extern "C" REX_FUNC(sub_820E4BC8) {
  SBX_SECOND_SET_FALLBACK(sub_820E4BC8)
  InteriorPoints* interior = At<InteriorPoints>(ctx.r3.u32);
  g.Call(sub_820E3578, &interior->record_0);
  ConstructGroup(g, interior->group_240, sub_820E2B90);
  ConstructGroup(g, interior->meters, sub_820E2DC8);
  ConstructGroup(g, interior->group_8512, sub_820E2FB8);
  ConstructGroup(g, interior->group_9488, sub_820E3008);
  ConstructGroup(g, interior->group_10464, sub_820E30C0);
  ctx.r3.u64 = guest::AddressOf(interior);
}

// InteriorPoints::~InteriorPoints: r3 the points.
extern "C" REX_FUNC(sub_820E4CB0) {
  SBX_SECOND_SET_FALLBACK(sub_820E4CB0)
  InteriorPoints* interior = At<InteriorPoints>(ctx.r3.u32);
  DestroyRecords(g, interior->group_10464.records);
  DestroyRecords(g, interior->group_9488.records);
  DestroyRecords(g, interior->group_8512.records);
  DestroyRecords(g, interior->meters.records);
  DestroyRecords(g, interior->group_240.records);
  DestroyRecord(g, interior->record_0);
}

// InteriorPoints::FindMeters and FindSeats: r3 the group, r4 the model.
extern "C" REX_FUNC(sub_820E2E60) {
  SBX_SECOND_SET_FALLBACK(sub_820E2E60)
  FindRecords(g, At<PointGroup<22>>(ctx.r3.u32)->records, 22, 7, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_820E3070) {
  SBX_SECOND_SET_FALLBACK(sub_820E3070)
  FindRecords(g, At<PointGroup<4>>(ctx.r3.u32)->records, 4, 2, ctx.r4.u32);
}

// SceneInterior::Reset: r3 the scene interior.
extern "C" REX_FUNC(sub_820E3310) {
  SBX_SECOND_SET_FALLBACK(sub_820E3310)
  SceneInterior* scene = At<SceneInterior>(ctx.r3.u32);
  for (u32_be& v : scene->unknown_24) {
    v = 0;
  }
  for (u32_be& v : scene->unknown_11656) {
    v = 0;
  }
  g.Call(sub_820D3488, &scene->record);
  g.Call(sub_820E31A0, &scene->points);  // the interior's defaults
  ResetValue(scene->unknown_11668);
  for (u32_be& v : scene->unknown_11632) {
    v = 0;
  }
}

// --- The lights ---

// LightPoints::LightPoints: r3 the points. Returns them.
extern "C" REX_FUNC(sub_820DD970) {
  SBX_SECOND_SET_FALLBACK(sub_820DD970)
  LightPoints* lights = At<LightPoints>(ctx.r3.u32);
  ConstructGroup(g, lights->groups[0], sub_820DC3B8);
  ConstructGroup(g, lights->groups[1], sub_820DC410);
  ConstructGroup(g, lights->groups[2], sub_820DC4C0);
  ctx.r3.u64 = guest::AddressOf(lights);
}

// LightPoints::~LightPoints: r3 the points.
extern "C" REX_FUNC(sub_820DDA08) {
  SBX_SECOND_SET_FALLBACK(sub_820DDA08)
  LightPoints* lights = At<LightPoints>(ctx.r3.u32);
  for (int i = 2; i >= 0; i--) {
    DestroyRecords(g, lights->groups[i].records);
  }
}

// LightPoints::Reset: r3 the points; each group's defaults again.
extern "C" REX_FUNC(sub_820DC518) {
  SBX_SECOND_SET_FALLBACK(sub_820DC518)
  LightPoints* lights = At<LightPoints>(ctx.r3.u32);
  g.Call(sub_820DC3B8, &lights->groups[0]);
  g.Call(sub_820DC410, &lights->groups[1]);
  g.Call(sub_820DC4C0, &lights->groups[2]);
  for (u32_be& v : lights->unknown_8) {
    v = 0;
  }
}

// LightPoints::FindGroup: r3 a group, r4 the model. Each record found by
// shape and linked.
extern "C" REX_FUNC(sub_820DC468) {
  SBX_SECOND_SET_FALLBACK(sub_820DC468)
  const uint32_t model = ctx.r4.u32;
  for (PointRecord& record : At<PointGroup<18>>(ctx.r3.u32)->records) {
    FindByShape(g, record, model);
    Link(g, record, kNearLinkDistance);
  }
}

// LightPoints::Find: r3 the points, r4 the model; each group's find.
extern "C" REX_FUNC(sub_820DC570) {
  SBX_SECOND_SET_FALLBACK(sub_820DC570)
  LightPoints* lights = At<LightPoints>(ctx.r3.u32);
  const uint32_t model = ctx.r4.u32;
  for (auto& group : lights->groups) {
    g.Call(sub_820DC468, &group, model);  // LightPoints::FindGroup
  }
}

// SceneLights::Reset: r3 the scene lights.
extern "C" REX_FUNC(sub_820DC6B0) {
  SBX_SECOND_SET_FALLBACK(sub_820DC6B0)
  SceneLights* scene = At<SceneLights>(ctx.r3.u32);
  scene->unknown_24 = 0;
  scene->pool4_object = 0;
  scene->unknown_13152 = 0;
  scene->unknown_13156 = 0;
  g.Call(sub_820D3128, &scene->record);  // Pool4Record::Reset
  g.Call(sub_820DC518, &scene->points);  // LightPoints::Reset
  ResetValue(scene->unknown_13160);
  const uint32_t record = guest::AddressOf(&scene->record);
  scene->points.record = record;
  for (auto& group : scene->points.groups) {
    group.record = record;
  }
}

// --- The traffic cars ---

// TrafficPoints::TrafficPoints: r3 the points. Returns them.
extern "C" REX_FUNC(sub_820E64D8) {
  SBX_SECOND_SET_FALLBACK(sub_820E64D8)
  TrafficPoints* traffic = At<TrafficPoints>(ctx.r3.u32);
  ConstructGroup(g, traffic->body, sub_820E50D8);
  ConstructGroup(g, traffic->headlights, sub_820E52C0);
  ConstructGroup(g, traffic->tail_lights, sub_820E5390);
  ctx.r3.u64 = guest::AddressOf(traffic);
}

// TrafficPoints::~TrafficPoints: r3 the points.
extern "C" REX_FUNC(sub_820E6570) {
  SBX_SECOND_SET_FALLBACK(sub_820E6570)
  TrafficPoints* traffic = At<TrafficPoints>(ctx.r3.u32);
  DestroyRecords(g, traffic->tail_lights);
  DestroyRecords(g, traffic->headlights);
  DestroyRecords(g, traffic->body);
}

// TrafficPoints::Reset: r3 the points; each group's defaults again.
extern "C" REX_FUNC(sub_820E5470) {
  SBX_SECOND_SET_FALLBACK(sub_820E5470)
  TrafficPoints* traffic = At<TrafficPoints>(ctx.r3.u32);
  traffic->unknown_0 = 0;
  g.Call(sub_820E50D8, traffic->body);
  g.Call(sub_820E52C0, traffic->headlights);
  g.Call(sub_820E5390, traffic->tail_lights);
}

// TrafficPoints::FindBody: r3 the body's 6 records, r4 to r7 four models,
// each record found by shape in one of them.
extern "C" REX_FUNC(sub_820E5128) {
  SBX_SECOND_SET_FALLBACK(sub_820E5128)
  PointRecord* body = At<PointRecord>(ctx.r3.u32);
  const uint32_t a = ctx.r4.u32, b = ctx.r5.u32, c = ctx.r6.u32, d = ctx.r7.u32;
  FindByShape(g, body[0], d);
  FindByShape(g, body[1], a);
  FindByShape(g, body[2], c);
  FindByShape(g, body[3], c);
  FindByShape(g, body[4], a);
  FindByShape(g, body[5], b);
}

extern "C" REX_FUNC(sub_820E5328) {  // TrafficPoints::FindHeadlights
  SBX_SECOND_SET_FALLBACK(sub_820E5328)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 7, 4, ctx.r4.u32);
  Link(g, records[5], kTrafficLinkDistance);
}

extern "C" REX_FUNC(sub_820E53F8) {  // TrafficPoints::FindTailLights
  SBX_SECOND_SET_FALLBACK(sub_820E53F8)
  PointRecord* records = At<PointRecord>(ctx.r3.u32);
  FindRecords(g, records, 12, 8, ctx.r4.u32);
  Link(g, records[9], kTrafficLinkDistance);
  Link(g, records[10], kTrafficLinkDistance);
}

// SceneTraffic::Reset: r3 the scene traffic.
extern "C" REX_FUNC(sub_820E5520) {
  SBX_SECOND_SET_FALLBACK(sub_820E5520)
  SceneTraffic* scene = At<SceneTraffic>(ctx.r3.u32);
  g.Call(sub_820D0848, &scene->record);
  g.Call(sub_820E5470, &scene->points);  // TrafficPoints::Reset
  scene->unknown_9764 = 0;
  scene->unknown_9760 = 0;
  scene->unknown_9792 = 0;
  for (u32_be& v : scene->unknown_9768) {
    v = 0;
  }
}

// --- The wheels ---

// WheelPoints::InitTire, InitRimA, InitRimB and InitBrakes: r3 the part.
// Returns it.
extern "C" REX_FUNC(sub_820E8668) {
  SBX_SECOND_SET_FALLBACK(sub_820E8668)
  TirePoints* tire = At<TirePoints>(ctx.r3.u32);
  ConstructRecords(g, tire->records);
  SetW(tire->points[0]);
  SetW(tire->points[1]);
  g.Call(sub_820E70A0, tire);  // the tire's defaults
  ctx.r3.u64 = guest::AddressOf(tire);
}

extern "C" REX_FUNC(sub_820E95A8) {
  SBX_SECOND_SET_FALLBACK(sub_820E95A8)
  RimPoints* rim = At<RimPoints>(ctx.r3.u32);
  ConstructRecords(g, rim->records);
  SetW(rim->points[0]);
  SetW(rim->points[1]);
  g.Call(sub_820E7500, rim);  // rim A's defaults
  ctx.r3.u64 = guest::AddressOf(rim);
}

extern "C" REX_FUNC(sub_820EA4D8) {
  SBX_SECOND_SET_FALLBACK(sub_820EA4D8)
  RimPoints* rim = At<RimPoints>(ctx.r3.u32);
  ConstructRecords(g, rim->records);
  SetW(rim->points[0]);
  SetW(rim->points[1]);
  g.Call(sub_820E77F0, rim);  // rim B's defaults
  ctx.r3.u64 = guest::AddressOf(rim);
}

extern "C" REX_FUNC(sub_820EAF18) {
  SBX_SECOND_SET_FALLBACK(sub_820EAF18)
  BrakePoints* brakes = At<BrakePoints>(ctx.r3.u32);
  ConstructRecords(g, brakes->records);
  SetW(brakes->points[0]);
  SetW(brakes->points[1]);
  g.Call(sub_820E7B78, brakes);  // the brakes' defaults
  ctx.r3.u64 = guest::AddressOf(brakes);
}

// WheelPoints::WheelPoints: r3 the points; two of each part. Returns them.
extern "C" REX_FUNC(sub_820EC880) {
  SBX_SECOND_SET_FALLBACK(sub_820EC880)
  WheelPoints* wheels = At<WheelPoints>(ctx.r3.u32);
  for (TirePoints& tire : wheels->tires) {
    g.Call(sub_820E8668, &tire);  // WheelPoints::InitTire
  }
  for (RimPoints& rim : wheels->rims_a) {
    g.Call(sub_820E95A8, &rim);  // WheelPoints::InitRimA
  }
  for (RimPoints& rim : wheels->rims_b) {
    g.Call(sub_820EA4D8, &rim);  // WheelPoints::InitRimB
  }
  for (BrakePoints& brakes : wheels->brakes) {
    g.Call(sub_820EAF18, &brakes);  // WheelPoints::InitBrakes
  }
  ctx.r3.u64 = guest::AddressOf(wheels);
}

// WheelPoints::~WheelPoints: r3 the points.
extern "C" REX_FUNC(sub_820EC8F0) {
  SBX_SECOND_SET_FALLBACK(sub_820EC8F0)
  WheelPoints* wheels = At<WheelPoints>(ctx.r3.u32);
  for (int i = 1; i >= 0; i--) {
    DestroyRecords(g, wheels->brakes[i].records);
  }
  for (int i = 1; i >= 0; i--) {
    DestroyRecords(g, wheels->rims_b[i].records);
  }
  for (int i = 1; i >= 0; i--) {
    DestroyRecords(g, wheels->rims_a[i].records);
  }
  for (int i = 1; i >= 0; i--) {
    DestroyRecords(g, wheels->tires[i].records);
  }
}

// WheelPoints::Reset: r3 the points; each part's defaults again.
extern "C" REX_FUNC(sub_820E81B0) {
  SBX_SECOND_SET_FALLBACK(sub_820E81B0)
  WheelPoints* wheels = At<WheelPoints>(ctx.r3.u32);
  for (u32_be& v : wheels->unknown_0) {
    v = 0;
  }
  for (TirePoints& tire : wheels->tires) {
    g.Call(sub_820E70A0, &tire);
  }
  for (RimPoints& rim : wheels->rims_a) {
    g.Call(sub_820E7500, &rim);
  }
  for (RimPoints& rim : wheels->rims_b) {
    g.Call(sub_820E77F0, &rim);
  }
  for (BrakePoints& brakes : wheels->brakes) {
    g.Call(sub_820E7B78, &brakes);
  }
  wheels->tires[1].unknown_300 = 0;
  wheels->rims_a[1].unknown_12 = 0;
  wheels->rims_b[1].unknown_12 = 0;
  wheels->brakes[1].unknown_12 = 0;
}

// SceneWheels::Reset: r3 the scene wheels.
extern "C" REX_FUNC(sub_820E8490) {
  SBX_SECOND_SET_FALLBACK(sub_820E8490)
  SceneWheels* scene = At<SceneWheels>(ctx.r3.u32);
  scene->unknown_24 = 0;
  scene->unknown_28 = 0;
  for (u32_be& v : scene->unknown_16768) {
    v = 0;
  }
  scene->unknown_16776 = 0;
  scene->unknown_16780 = 0;
  ResetValue(scene->unknown_16784);
  g.Call(sub_820D3548, &scene->record);  // WheelsRecord::Reset
  g.Call(sub_820E81B0, &scene->points);  // WheelPoints::Reset
}

// --- Models ---

// SceneModelBlock::SceneModelBlock: r3 the block; w = 1 on its points.
extern "C" REX_FUNC(sub_820D9DD0) {
  SBX_SECOND_SET_FALLBACK(sub_820D9DD0)
  SceneModelBlock* block = At<SceneModelBlock>(ctx.r3.u32);
  for (Vector& point : block->points_16) {
    SetW(point);
  }
  for (Vector& point : block->points_80) {
    SetW(point);
  }
  for (Vector& point : block->points_336) {
    SetW(point);
  }
  for (Vector& point : block->points_656) {
    SetW(point);
  }
}

// Morph::Run: r4 the morph, r5 and r6 its inputs; then Morph::Blend on it.
extern "C" REX_FUNC(sub_8257A610) {
  SBX_SECOND_SET_FALLBACK(sub_8257A610)
  Morph* morph = At<Morph>(ctx.r4.u32);
  morph->input_a = ctx.r5.u32;
  morph->input_b = ctx.r6.u32;
  g.Call(sub_825718A8, morph);  // Morph::Blend (r4 to r6 as they came)
}

// A model's mesh's streams: r3 the model, r4 the mesh's index.
extern "C" REX_FUNC(sub_8257A360) {  // Model::StreamData
  SBX_SECOND_SET_FALLBACK(sub_8257A360)
  ctx.r3.u64 = uint32_t(At<Model>(ctx.r3.u32)->MeshAt(ctx.r4.u32)->stream_data);
}

extern "C" REX_FUNC(sub_8257A378) {  // Model::StreamSize
  SBX_SECOND_SET_FALLBACK(sub_8257A378)
  ctx.r3.u64 = uint32_t(At<Model>(ctx.r3.u32)->MeshAt(ctx.r4.u32)->stream_size);
}

extern "C" REX_FUNC(sub_8257A390) {  // Model::StreamData2
  SBX_SECOND_SET_FALLBACK(sub_8257A390)
  ctx.r3.u64 = uint32_t(At<Model>(ctx.r3.u32)->MeshAt(ctx.r4.u32)->stream2_data);
}

extern "C" REX_FUNC(sub_8257A3A8) {  // Model::StreamSize2
  SBX_SECOND_SET_FALLBACK(sub_8257A3A8)
  ctx.r3.u64 = uint32_t(At<Model>(ctx.r3.u32)->MeshAt(ctx.r4.u32)->stream2_size);
}
