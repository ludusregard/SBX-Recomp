// The scene parts' point records: for each of a car's parts (bumpers,
// doors, lights, mufflers, the cockpit's and interior's parts, the lights,
// the traffic cars, the wheels' tires, rims and brakes) a record of the
// model node and shape it's found under, in groups whose defaults name the
// shapes.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"
#include "game/scene_loads.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

// One part's point: where it's found in the model (by node or by shape
// name), and its matrix and points.
struct PointRecord {
  u32_be unknown_0;
  u32_be unknown_4;
  u32_be unknown_8;
  uint8_t unknown_12[4];
  Vector points[9];  // A matrix (identity after a clear), then five points.
  char node[32];
  char shape[32];
  i32_be unknown_224;
  i32_be unknown_228;
  uint8_t unknown_232[8];
};
GUEST_SIZE(PointRecord, 240);
GUEST_OFFSET(PointRecord, points, 16);
GUEST_OFFSET(PointRecord, node, 160);
GUEST_OFFSET(PointRecord, shape, 192);
GUEST_OFFSET(PointRecord, unknown_228, 228);

// A group with a header before its records; the header's first word points
// at the scene object's record once the object is reset.
template <int N>
struct PointGroup {
  u32_be record;
  uint8_t unknown_4[4];
  u32_be unknown_8;  // An interior's +9656 (SceneInterior::Apply).
  uint8_t unknown_12[4];
  PointRecord records[N];
};
static_assert(sizeof(PointGroup<1>) == 256);

// A door: 25 records, then four points.
struct DoorPoints {
  PointRecord records[25];
  uint8_t unknown_6000[16];
  Vector points[4];
  uint8_t unknown_6080[16];
};
GUEST_SIZE(DoorPoints, 6096);
GUEST_OFFSET(DoorPoints, points, 6016);

struct HeadlightPoints {
  PointRecord records[13];
  Vector points[4];
};
GUEST_SIZE(HeadlightPoints, 3184);

struct MufflerPoints {
  Vector points[16];
  PointRecord records[5];
};
GUEST_SIZE(MufflerPoints, 1456);

// The car body's parts.
struct PartsPoints {
  PointRecord front_bumper[11];
  uint8_t unknown_2640[16];
  PointRecord cooler;
  PointRecord rear_bumper[7];
  uint8_t unknown_4576[16];
  PointRecord side_skirts;
  PointRecord rear_spoiler[3];
  PointRecord bonnet[2];
  DoorPoints left_door;
  DoorPoints right_door;
  PointRecord doors[23];
  uint8_t unknown_23744[16];
  PointRecord unknown_23760;
  PointRecord unknown_24000;
  PointRecord unknown_24240;
  HeadlightPoints headlights;
  uint8_t unknown_27664[16];
  PointRecord tail_lights[15];
  PointRecord fenders[7];
  PointRecord cabin[5];
  PointRecord engine[2];
  MufflerPoints mufflers;
  PointRecord shadows[4];
};
GUEST_OFFSET(PartsPoints, cooler, 2656);
GUEST_OFFSET(PartsPoints, rear_bumper, 2896);
GUEST_OFFSET(PartsPoints, side_skirts, 4592);
GUEST_OFFSET(PartsPoints, rear_spoiler, 4832);
GUEST_OFFSET(PartsPoints, bonnet, 5552);
GUEST_OFFSET(PartsPoints, left_door, 6032);
GUEST_OFFSET(PartsPoints, right_door, 12128);
GUEST_OFFSET(PartsPoints, doors, 18224);
GUEST_OFFSET(PartsPoints, unknown_23760, 23760);
GUEST_OFFSET(PartsPoints, headlights, 24480);
GUEST_OFFSET(PartsPoints, tail_lights, 27680);
GUEST_OFFSET(PartsPoints, fenders, 31280);
GUEST_OFFSET(PartsPoints, cabin, 32960);
GUEST_OFFSET(PartsPoints, engine, 34160);
GUEST_OFFSET(PartsPoints, mufflers, 34640);
GUEST_OFFSET(PartsPoints, shadows, 36096);
GUEST_SIZE(PartsPoints, 37056);

struct CockpitPoints {
  PointRecord records_0[3];
  PointRecord records_720[2];
  PointRecord record_1200;
  PointRecord records_1440[2];
  PointRecord record_1920;
  PointRecord records_2160[8];
};
GUEST_SIZE(CockpitPoints, 4080);
GUEST_OFFSET(CockpitPoints, records_2160, 2160);

struct InteriorPoints {
  PointRecord record_0;
  PointGroup<12> group_240;
  uint8_t unknown_3136[16];
  PointGroup<22> meters;
  uint8_t unknown_8448[64];
  PointGroup<4> group_8512;
  PointGroup<4> group_9488;
  PointGroup<4> group_10464;
};
GUEST_OFFSET(InteriorPoints, meters, 3152);
GUEST_OFFSET(InteriorPoints, group_8512, 8512);
GUEST_OFFSET(InteriorPoints, group_10464, 10464);
GUEST_SIZE(InteriorPoints, 11440);

struct LightPoints {
  u32_be record;
  uint8_t unknown_4[4];
  u32_be unknown_8[8];
  uint8_t unknown_40[8];
  PointGroup<18> groups[3];
};
GUEST_OFFSET(LightPoints, groups, 48);
GUEST_SIZE(LightPoints, 13056);

struct TrafficPoints {
  u32_be unknown_0;
  uint8_t unknown_4[12];
  PointRecord body[6];
  uint8_t unknown_1456[16];
  PointRecord headlights[7];
  PointRecord tail_lights[12];
};
GUEST_OFFSET(TrafficPoints, headlights, 1472);
GUEST_OFFSET(TrafficPoints, tail_lights, 3152);
GUEST_SIZE(TrafficPoints, 6032);

struct TirePoints {
  uint8_t unknown_0[300];
  u32_be unknown_300;
  uint8_t unknown_304[64];
  PointRecord records[6];
  Vector points[2];
};
GUEST_SIZE(TirePoints, 1840);

struct RimPoints {
  uint8_t unknown_0[12];
  u32_be unknown_12;
  uint8_t unknown_16[16];
  PointRecord records[7];
  Vector points[2];
};
GUEST_SIZE(RimPoints, 1744);

struct BrakePoints {
  uint8_t unknown_0[12];
  u32_be unknown_12;
  uint8_t unknown_16[16];
  PointRecord records[12];
  Vector points[2];
};
GUEST_SIZE(BrakePoints, 2944);

struct WheelPoints {
  u32_be unknown_0[7];
  uint8_t unknown_28[4];
  TirePoints tires[2];
  RimPoints rims_a[2];
  RimPoints rims_b[2];
  BrakePoints brakes[2];
};
GUEST_OFFSET(WheelPoints, tires, 32);
GUEST_OFFSET(WheelPoints, rims_a, 3712);
GUEST_OFFSET(WheelPoints, rims_b, 7200);
GUEST_OFFSET(WheelPoints, brakes, 10688);
GUEST_SIZE(WheelPoints, 16576);

// The scene objects the point tables belong to. Each is reset with a copy
// of [0x826B2B24] (writable) at its end.
struct SceneParts {
  uint8_t unknown_0[24];
  u32_be unknown_24;  // SceneParts::Link's car model,
  u32_be unknown_28;  // and its skeleton.
  uint8_t unknown_32[68];
  u32_be unknown_100;
  u32_be unknown_104;
  PartsRecord record;
  uint8_t unknown_168[8];
  PartsPoints points;
  uint8_t unknown_37232[16];
  u32_be unknown_37248;  // SceneParts::SetRecord.
  uint8_t unknown_37252[28];
  u32_be unknown_37280;
  u32_be unknown_37284;  // Its model record: +4 its kind.
};
GUEST_OFFSET(SceneParts, record, 108);
GUEST_OFFSET(SceneParts, points, 176);
GUEST_OFFSET(SceneParts, unknown_37284, 37284);

struct SceneCockpit {
  uint8_t unknown_0[24];
  u32_be unknown_24;
  u32_be unknown_28;
  u32_be unknown_32;
  CockpitRecord record;
  uint8_t unknown_180[12];
  CockpitPoints points;
  u32_be record_address;
  uint8_t unknown_4276[12];
  u32_be unknown_4288[6];
  u32_be unknown_4312;
  u32_be unknown_4316;
};
GUEST_OFFSET(SceneCockpit, record, 36);
GUEST_OFFSET(SceneCockpit, points, 192);
GUEST_OFFSET(SceneCockpit, record_address, 4272);
GUEST_OFFSET(SceneCockpit, unknown_4316, 4316);

struct SceneInterior {
  uint8_t unknown_0[24];
  // SceneInterior::Link's car model and its skeleton and node block, then
  // the object its record is also copied to (at its +28).
  u32_be unknown_24[4];
  InteriorRecord record;
  InteriorPoints points;
  uint8_t unknown_11600[32];
  u32_be unknown_11632[6];
  u32_be unknown_11656[3];
  u32_be unknown_11668;
};
GUEST_OFFSET(SceneInterior, record, 40);
GUEST_OFFSET(SceneInterior, points, 160);
GUEST_OFFSET(SceneInterior, unknown_11632, 11632);
GUEST_OFFSET(SceneInterior, unknown_11668, 11668);

struct SceneLights {
  uint8_t unknown_0[24];
  u32_be unknown_24;
  u32_be pool4_object;  // The lights' pool 4 object.
  Pool4Record record;
  uint8_t unknown_84[12];
  LightPoints points;
  u32_be unknown_13152;
  u32_be unknown_13156;
  u32_be unknown_13160;
};
GUEST_OFFSET(SceneLights, record, 32);
GUEST_OFFSET(SceneLights, points, 96);
GUEST_OFFSET(SceneLights, unknown_13160, 13160);

struct SceneTraffic {
  uint8_t unknown_0[32];
  SceneCarRecord record;
  TrafficPoints points;
  u32_be unknown_9760;
  u32_be unknown_9764;
  u32_be unknown_9768[6];
  u32_be unknown_9792;
};
GUEST_OFFSET(SceneTraffic, points, 3728);
GUEST_OFFSET(SceneTraffic, unknown_9792, 9792);

struct SceneWheels {
  uint8_t unknown_0[24];
  u32_be unknown_24;
  u32_be unknown_28;
  WheelsRecord record;
  uint8_t unknown_188[4];
  WheelPoints points;
  u32_be unknown_16768[2];
  u32_be unknown_16776;  // SceneWheels::SetRecord.
  u32_be unknown_16780;
  u32_be unknown_16784;
};
GUEST_OFFSET(SceneWheels, points, 192);
GUEST_OFFSET(SceneWheels, unknown_16784, 16784);

// Three blocks of points a scene model keeps.
struct SceneModelBlock {
  uint8_t unknown_0[16];
  Vector points_16[4];
  Vector points_80[16];
  Vector points_336[16];
  uint8_t unknown_592[64];
  Vector points_656[2];
};
GUEST_OFFSET(SceneModelBlock, points_80, 80);
GUEST_OFFSET(SceneModelBlock, points_336, 336);
GUEST_OFFSET(SceneModelBlock, points_656, 656);

// A morph: its two inputs at +8 and +12 (Morph::Run).
struct Morph {
  uint8_t unknown_0[8];
  u32_be input_a;
  u32_be input_b;
};
GUEST_OFFSET(Morph, input_a, 8);

}  // namespace game
