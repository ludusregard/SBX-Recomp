// The scene parts' point tables as our own C++ (--native_second_set, on by
// default): the parts', cockpit's, interior's, lights', traffic's and
// wheels' points' constructors and defaults (the bumpers, doors, lights,
// mufflers, tires, rims, brakes), a point record's node and shape names,
// the models' stream getters and the morph's run.
//
// Written from SBX's generated code: the defaults' stores as tables in
// their order. The models' draws, the morph's blend and the scene parts'
// releases stay recompiled.
//
// A model's streams: +40 its meshes (544 bytes each): +268 and +388 the
// first stream's size and data, +284 and +392 the second's.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_second_set, true, "SBX",
                    "Our own C++ for the scene parts' point tables (their defaults and constructors), the models' streams and the morph's run instead of the recompiled functions; the models' draws stay recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// PartsPoints::InitLeftDoor (sub_820DF160).
// 4 stores
constexpr Fill kPartsPoints_InitLeftDoor0[] = {
    {6028, kImage, 0x82000A20, 4, 16},
};

// PartsPoints::InitRightDoor (sub_820DF1B8).
// 4 stores
constexpr Fill kPartsPoints_InitRightDoor0[] = {
    {6028, kImage, 0x82000A20, 4, 16},
};

// PartsPoints::InitHeadlights (sub_820E0F48).
// 4 stores
constexpr Fill kPartsPoints_InitHeadlights0[] = {
    {3132, kImage, 0x82000A20, 4, 16},
};

// PartsPoints::InitMufflers (sub_820DFBB8).
// 16 stores
constexpr Fill kPartsPoints_InitMufflers0[] = {
    {12, kImage, 0x82000A20, 16, 16},
};

// LightPoints::Reset (sub_820DC518).
// 8 stores
constexpr Fill kLightPoints_Reset0[] = {
    {8, kWord, 0x0, 8, 4},
};

// TrafficPoints::Reset (sub_820E5470).
// 1 stores
constexpr Fill kTrafficPoints_Reset0[] = {
    {0, kWord, 0x0},
};

// WheelPoints::InitTire (sub_820E8668).
// 2 stores
constexpr Fill kWheelPoints_InitTire0[] = {
    {1820, kImage, 0x82000A20, 2, 16},
};

// WheelPoints::InitRimA (sub_820E95A8).
// 2 stores
constexpr Fill kWheelPoints_InitRimA0[] = {
    {1724, kImage, 0x82000A20, 2, 16},
};

// WheelPoints::InitRimB (sub_820EA4D8).
// 2 stores
constexpr Fill kWheelPoints_InitRimB0[] = {
    {1724, kImage, 0x82000A20, 2, 16},
};

// WheelPoints::InitBrakes (sub_820EAF18).
// 2 stores
constexpr Fill kWheelPoints_InitBrakes0[] = {
    {2924, kImage, 0x82000A20, 2, 16},
};

// WheelPoints::Reset (sub_820E81B0).
// 7 stores
constexpr Fill kWheelPoints_Reset0[] = {
    {0, kWord, 0x0, 7, 4},
};
// 4 stores
constexpr Fill kWheelPoints_Reset1[] = {
    {2172, kWord, 0x0, 2, 3296}, {8956, kWord, 0x0, 2, 4688},
};

// SceneParts::Reset (sub_820E00B8).
// 7 stores
constexpr Fill kSceneParts_Reset0[] = {
    {24, kWord, 0x0, 2, 37260}, {28, kWord, 0x0, 2, 76}, {37280, kWord, 0x0}, {100, kWord, 0x0},
    {37284, kImage, 0x826B2B24},
};

// SceneCockpit::Reset (sub_820DBA90).
// 5 stores
constexpr Fill kSceneCockpit_Reset0[] = {
    {24, kWord, 0x0, 3, 4}, {4312, kWord, 0x0}, {4316, kImage, 0x826B2B24},
};
// 7 stores
constexpr Fill kSceneCockpit_Reset1[] = {
    {4272, kSelf, 36}, {4288, kWord, 0x0, 6, 4},
};

// SceneInterior::Reset (sub_820E3310).
// 7 stores
constexpr Fill kSceneInterior_Reset0[] = {
    {24, kWord, 0x0, 4, 4}, {11656, kWord, 0x0, 3, 4},
};
// 7 stores
constexpr Fill kSceneInterior_Reset1[] = {
    {11668, kImage, 0x826B2B24}, {11632, kWord, 0x0, 6, 4},
};

// SceneLights::Reset (sub_820DC6B0).
// 4 stores
constexpr Fill kSceneLights_Reset0[] = {
    {24, kWord, 0x0, 2, 4}, {13152, kWord, 0x0, 2, 4},
};
// 5 stores
constexpr Fill kSceneLights_Reset1[] = {
    {13160, kImage, 0x826B2B24}, {96, kSelf, 32, 2, 48}, {4480, kSelf, 32, 2, 4336},
};

// SceneTraffic::Reset (sub_820E5520).
// 9 stores
constexpr Fill kSceneTraffic_Reset0[] = {
    {9764, kWord, 0x0}, {9760, kWord, 0x0, 2, 32}, {9768, kWord, 0x0, 6, 4},
};

// SceneWheels::Reset (sub_820E8490).
// 46 stores
constexpr Fill kSceneWheels_Reset0[] = {
    {24, kWord, 0x0, 2, 4}, {16768, kWord, 0x0, 4, 4}, {16784, kImage, 0x826B2B24}, {32, kWord, 0x1},
    {52, kImage, 0x82002520}, {48, kWord, 0xFFFFFFFF}, {68, kImage, 0x82002520}, {64, kWord, 0xFFFFFFFF},
    {56, kImage, 0x82001BDC, 2, 16}, {36, kWord, 0x0, 3, 4}, {60, kWord, 0x0}, {80, kImage, 0x82001BD8, 2, 4},
    {76, kWord, 0xFFFFFFFF}, {92, kImage, 0x82001BD8}, {88, kWord, 0xFFFFFFFF}, {96, kImage, 0x82001BD8},
    {100, kWord, 0x0}, {160, kImage, 0x82000A24}, {156, kWord, 0xFFFFFFFF}, {164, kImage, 0x82000A24},
    {104, kWord, 0x0}, {168, kImage, 0x82000A24}, {108, kWord, 0x0}, {172, kImage, 0x82000A24},
    {112, kWord, 0x0}, {176, kImage, 0x82000A24}, {116, kWord, 0x0}, {180, kImage, 0x82000A24},
    {120, kWord, 0x0}, {184, kImage, 0x82000A24}, {132, kImage, 0x82000A24}, {124, kWord, 0x0, 2, 24},
    {140, kWord, 0xB}, {136, kImage, 0x82000A24}, {128, kWord, 0x0, 2, 24}, {144, kWord, 0xB},
};

// SceneModelBlock::SceneModelBlock (sub_820D9DD0).
// 38 stores
constexpr Fill kSceneModelBlock_SceneModelBlock0[] = {
    {28, kImage, 0x82000A20, 36, 16}, {668, kImage, 0x82000A20, 2, 16},
};

// PointRecord::PointRecord (sub_820D0030).
// 9 stores
constexpr Fill kPointRecord_PointRecord0[] = {
    {28, kImage, 0x82000A20, 9, 16},
};

bool On() { return REXCVAR_GET(native_second_set); }

using guest::LoadU32;
using guest::LoadU8;
using guest::StoreU32;
using guest::StoreU8;

// strcpy of the name at `from` to `to` (the terminator copied too).
void CopyName(uint8_t* base, uint32_t to, uint32_t from) {
  for (uint32_t i = 0;; i++) {
    const uint8_t c = LoadU8(base, from + i);
    StoreU8(base, to + i, c);
    if (c == 0) {
      return;
    }
  }
}

}  // namespace

// PartsPoints::InitLeftDoor.
extern "C" REX_FUNC(sub_820DF160) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DF160(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 240;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 720;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 960;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1200;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1680;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1920;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2160;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2640;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2880;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3120;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3360;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3600;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3840;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4080;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4320;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4560;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4800;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5040;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5280;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5520;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5760;
  sub_820D0030(ctx, base);
  guest_memory::Apply(base, object, kPartsPoints_InitLeftDoor0, args);
  ctx.r3.u64 = object + 0;
  sub_820DE3C8(ctx, base);
  ctx.r3.u64 = object;
}

// PartsPoints::InitRightDoor.
extern "C" REX_FUNC(sub_820DF1B8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DF1B8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 240;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 720;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 960;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1200;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1680;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1920;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2160;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2640;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2880;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3120;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3360;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3600;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3840;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4080;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4320;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4560;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4800;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5040;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5280;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5520;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5760;
  sub_820D0030(ctx, base);
  guest_memory::Apply(base, object, kPartsPoints_InitRightDoor0, args);
  ctx.r3.u64 = object + 0;
  sub_820DE638(ctx, base);
  ctx.r3.u64 = object;
}

// PartsPoints::InitHeadlights.
extern "C" REX_FUNC(sub_820E0F48) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E0F48(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 240;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 720;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 960;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1200;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1680;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1920;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2160;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2640;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2880;
  sub_820D0030(ctx, base);
  guest_memory::Apply(base, object, kPartsPoints_InitHeadlights0, args);
  ctx.r3.u64 = object + 0;
  sub_820DF2E8(ctx, base);
  ctx.r3.u64 = object;
}

// PartsPoints::InitMufflers.
extern "C" REX_FUNC(sub_820DFBB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DFBB8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPartsPoints_InitMufflers0, args);
  ctx.r3.u64 = object + 256;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 496;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 736;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 976;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1216;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820DEE88(ctx, base);
  ctx.r3.u64 = object;
}

// PartsPoints::PartsPoints.
extern "C" REX_FUNC(sub_820E25D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E25D8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 240;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 720;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 960;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1200;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1680;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1920;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2160;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820DDD58(ctx, base);
  ctx.r3.u64 = object + 2656;
  sub_820DF0D0(ctx, base);
  ctx.r3.u64 = object + 2896;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3136;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3376;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3616;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3856;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4096;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4336;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2896;
  sub_820DDFF8(ctx, base);
  ctx.r3.u64 = object + 4592;
  sub_820DF118(ctx, base);
  ctx.r3.u64 = object + 4832;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5072;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5312;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4832;
  sub_820DE260(ctx, base);
  ctx.r3.u64 = object + 5552;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5792;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5552;
  sub_820DE310(ctx, base);
  ctx.r3.u64 = object + 6032;
  sub_820DF160(ctx, base);
  ctx.r3.u64 = object + 12128;
  sub_820DF1B8(ctx, base);
  ctx.r3.u64 = object + 18224;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 18464;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 18704;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 18944;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 19184;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 19424;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 19664;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 19904;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 20144;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 20384;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 20624;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 20864;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 21104;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 21344;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 21584;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 21824;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 22064;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 22304;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 22544;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 22784;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 23024;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 23264;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 23504;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 18224;
  sub_820DE798(ctx, base);
  ctx.r3.u64 = object + 23760;
  sub_820DF210(ctx, base);
  ctx.r3.u64 = object + 24000;
  sub_820DF258(ctx, base);
  ctx.r3.u64 = object + 24240;
  sub_820DF2A0(ctx, base);
  ctx.r3.u64 = object + 24480;
  sub_820E0F48(ctx, base);
  ctx.r3.u64 = object + 27680;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 27920;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 28160;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 28400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 28640;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 28880;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 29120;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 29360;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 29600;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 29840;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 30080;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 30320;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 30560;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 30800;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 31040;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 27680;
  sub_820DE9B8(ctx, base);
  ctx.r3.u64 = object + 31280;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 31520;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 31760;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 32000;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 32240;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 32480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 32720;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 31280;
  sub_820DEA98(ctx, base);
  ctx.r3.u64 = object + 32960;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 33200;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 33440;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 33680;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 33920;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 32960;
  sub_820DEC50(ctx, base);
  ctx.r3.u64 = object + 34160;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 34400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 34160;
  sub_820DEDE8(ctx, base);
  ctx.r3.u64 = object + 34640;
  sub_820DFBB8(ctx, base);
  ctx.r3.u64 = object + 36096;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 36336;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 36576;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 36816;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 36096;
  sub_820DEEF0(ctx, base);
  ctx.r3.u64 = object;
}

// CockpitPoints::CockpitPoints.
extern "C" REX_FUNC(sub_820DBE00) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DBE00(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 240;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820DB4E8(ctx, base);
  ctx.r3.u64 = object + 720;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 960;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 720;
  sub_820DB5A0(ctx, base);
  ctx.r3.u64 = object + 1200;
  sub_820DBD18(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1680;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820DB658(ctx, base);
  ctx.r3.u64 = object + 1920;
  sub_820DBD60(ctx, base);
  ctx.r3.u64 = object + 2160;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2640;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2880;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3120;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3360;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3600;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3840;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2160;
  sub_820DB710(ctx, base);
  ctx.r3.u64 = object;
}

// InteriorPoints::InteriorPoints.
extern "C" REX_FUNC(sub_820E4BC8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E4BC8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_820E3578(ctx, base);
  ctx.r3.u64 = object + 256;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 496;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 736;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 976;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1216;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1456;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1696;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1936;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2176;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2416;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2656;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2896;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 240;
  sub_820E2B90(ctx, base);
  ctx.r3.u64 = object + 3168;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3408;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3648;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3888;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4128;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4368;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4608;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4848;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5088;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5328;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5568;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5808;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6048;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6288;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6528;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6768;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7008;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7248;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7488;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7728;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7968;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8208;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3152;
  sub_820E2DC8(ctx, base);
  ctx.r3.u64 = object + 8528;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8768;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9008;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9248;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8512;
  sub_820E2FB8(ctx, base);
  ctx.r3.u64 = object + 9504;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9744;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9984;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10224;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9488;
  sub_820E3008(ctx, base);
  ctx.r3.u64 = object + 10480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10720;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10960;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 11200;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10464;
  sub_820E30C0(ctx, base);
  ctx.r3.u64 = object;
}

// LightPoints::LightPoints.
extern "C" REX_FUNC(sub_820DD970) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DD970(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 64;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 304;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 544;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 784;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1024;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1264;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1504;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1744;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1984;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2224;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2464;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2704;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2944;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3184;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3424;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3664;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3904;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4144;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 48;
  sub_820DC3B8(ctx, base);
  ctx.r3.u64 = object + 4400;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4640;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4880;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5120;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5360;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5600;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5840;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6080;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6320;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6560;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 6800;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7040;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7280;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7520;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 7760;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8000;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8240;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8480;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4384;
  sub_820DC410(ctx, base);
  ctx.r3.u64 = object + 8736;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8976;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9216;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9456;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9696;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 9936;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10176;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10416;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10656;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 10896;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 11136;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 11376;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 11616;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 11856;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 12096;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 12336;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 12576;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 12816;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 8720;
  sub_820DC4C0(ctx, base);
  ctx.r3.u64 = object;
}

// LightPoints::Reset.
extern "C" REX_FUNC(sub_820DC518) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DC518(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 48;
  sub_820DC3B8(ctx, base);
  ctx.r3.u64 = object + 4384;
  sub_820DC410(ctx, base);
  ctx.r3.u64 = object + 8720;
  sub_820DC4C0(ctx, base);
  guest_memory::Apply(base, object, kLightPoints_Reset0, args);
}

// TrafficPoints::TrafficPoints.
extern "C" REX_FUNC(sub_820E64D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E64D8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 16;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 256;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 496;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 736;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 976;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1216;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 16;
  sub_820E50D8(ctx, base);
  ctx.r3.u64 = object + 1472;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1712;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1952;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2192;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2432;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2672;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2912;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1472;
  sub_820E52C0(ctx, base);
  ctx.r3.u64 = object + 3152;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3392;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3632;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3872;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4112;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4352;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4592;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 4832;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5072;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5312;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5552;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 5792;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 3152;
  sub_820E5390(ctx, base);
  ctx.r3.u64 = object;
}

// TrafficPoints::Reset.
extern "C" REX_FUNC(sub_820E5470) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E5470(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kTrafficPoints_Reset0, args);
  ctx.r3.u64 = object + 16;
  sub_820E50D8(ctx, base);
  ctx.r3.u64 = object + 1472;
  sub_820E52C0(ctx, base);
  ctx.r3.u64 = object + 3152;
  sub_820E5390(ctx, base);
}

// WheelPoints::InitTire.
extern "C" REX_FUNC(sub_820E8668) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E8668(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 368;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 608;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 848;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1088;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1328;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1568;
  sub_820D0030(ctx, base);
  guest_memory::Apply(base, object, kWheelPoints_InitTire0, args);
  ctx.r3.u64 = object + 0;
  sub_820E70A0(ctx, base);
  ctx.r3.u64 = object;
}

// WheelPoints::InitRimA.
extern "C" REX_FUNC(sub_820E95A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E95A8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 32;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 272;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 512;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 752;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 992;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1232;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1472;
  sub_820D0030(ctx, base);
  guest_memory::Apply(base, object, kWheelPoints_InitRimA0, args);
  ctx.r3.u64 = object + 0;
  sub_820E7500(ctx, base);
  ctx.r3.u64 = object;
}

// WheelPoints::InitRimB.
extern "C" REX_FUNC(sub_820EA4D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820EA4D8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 32;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 272;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 512;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 752;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 992;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1232;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1472;
  sub_820D0030(ctx, base);
  guest_memory::Apply(base, object, kWheelPoints_InitRimB0, args);
  ctx.r3.u64 = object + 0;
  sub_820E77F0(ctx, base);
  ctx.r3.u64 = object;
}

// WheelPoints::InitBrakes.
extern "C" REX_FUNC(sub_820EAF18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820EAF18(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 32;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 272;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 512;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 752;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 992;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1232;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1472;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1712;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 1952;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2192;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2432;
  sub_820D0030(ctx, base);
  ctx.r3.u64 = object + 2672;
  sub_820D0030(ctx, base);
  guest_memory::Apply(base, object, kWheelPoints_InitBrakes0, args);
  ctx.r3.u64 = object + 0;
  sub_820E7B78(ctx, base);
  ctx.r3.u64 = object;
}

// WheelPoints::WheelPoints.
extern "C" REX_FUNC(sub_820EC880) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820EC880(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 32;
  sub_820E8668(ctx, base);
  ctx.r3.u64 = object + 1872;
  sub_820E8668(ctx, base);
  ctx.r3.u64 = object + 3712;
  sub_820E95A8(ctx, base);
  ctx.r3.u64 = object + 5456;
  sub_820E95A8(ctx, base);
  ctx.r3.u64 = object + 7200;
  sub_820EA4D8(ctx, base);
  ctx.r3.u64 = object + 8944;
  sub_820EA4D8(ctx, base);
  ctx.r3.u64 = object + 10688;
  sub_820EAF18(ctx, base);
  ctx.r3.u64 = object + 13632;
  sub_820EAF18(ctx, base);
  ctx.r3.u64 = object;
}

// WheelPoints::Reset.
extern "C" REX_FUNC(sub_820E81B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E81B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kWheelPoints_Reset0, args);
  ctx.r3.u64 = object + 32;
  sub_820E70A0(ctx, base);
  ctx.r3.u64 = object + 1872;
  sub_820E70A0(ctx, base);
  ctx.r3.u64 = object + 3712;
  sub_820E7500(ctx, base);
  ctx.r3.u64 = object + 5456;
  sub_820E7500(ctx, base);
  ctx.r3.u64 = object + 7200;
  sub_820E77F0(ctx, base);
  ctx.r3.u64 = object + 8944;
  sub_820E77F0(ctx, base);
  ctx.r3.u64 = object + 10688;
  sub_820E7B78(ctx, base);
  ctx.r3.u64 = object + 13632;
  sub_820E7B78(ctx, base);
  guest_memory::Apply(base, object, kWheelPoints_Reset1, args);
}

// PartsPoints::~PartsPoints.
extern "C" REX_FUNC(sub_820E27D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E27D8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object;
}

// CockpitPoints::~CockpitPoints.
extern "C" REX_FUNC(sub_820DBEC8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DBEC8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object;
}

// InteriorPoints::~InteriorPoints.
extern "C" REX_FUNC(sub_820E4CB0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E4CB0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object;
}

// LightPoints::~LightPoints.
extern "C" REX_FUNC(sub_820DDA08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DDA08(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 64;
}

// WheelPoints::~WheelPoints.
extern "C" REX_FUNC(sub_820EC8F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820EC8F0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 400;
}

// TrafficPoints::~TrafficPoints.
extern "C" REX_FUNC(sub_820E6570) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E6570(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 16;
}

// PartsPoints::FindRearSpoiler.
extern "C" REX_FUNC(sub_820DE2B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DE2B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 484);
  ctx.r4.u64 = guest::LoadU32(base, object + 708);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 244);
  ctx.r4.u64 = guest::LoadU32(base, object + 468);
  sub_820D16B0(ctx, base);
}

// PartsPoints::FindBonnet.
extern "C" REX_FUNC(sub_820DE378) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DE378(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
}

// PartsPoints::FindLeftDoor.
extern "C" REX_FUNC(sub_820DE430) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DE430(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1680;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1920;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2160;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2400;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2640;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2880;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3120;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3360;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3600;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3840;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4080;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4320;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4560;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4800;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5040;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5280;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5520;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5760;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1684);
  ctx.r4.u64 = guest::LoadU32(base, object + 1908);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1924);
  ctx.r4.u64 = guest::LoadU32(base, object + 2148);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2164);
  ctx.r4.u64 = guest::LoadU32(base, object + 2388);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2404);
  ctx.r4.u64 = guest::LoadU32(base, object + 2628);
  sub_820D16B0(ctx, base);
}

// PartsPoints::FindRightDoor.
extern "C" REX_FUNC(sub_820DE6C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DE6C8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1680;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1920;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2160;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2400;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2640;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2880;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3120;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3360;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3600;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3840;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4080;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4320;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4560;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4800;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5040;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5280;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5520;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5760;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
}

// PartsPoints::FindDoors.
extern "C" REX_FUNC(sub_820DE7E8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DE7E8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1680;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1920;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2160;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2400;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2640;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2880;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3120;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3360;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3600;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3840;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4080;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4320;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4560;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4800;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5040;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5280;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 724);
  ctx.r4.u64 = guest::LoadU32(base, object + 948);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 964);
  ctx.r4.u64 = guest::LoadU32(base, object + 1188);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1204);
  ctx.r4.u64 = guest::LoadU32(base, object + 1428);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1444);
  ctx.r4.u64 = guest::LoadU32(base, object + 1668);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1684);
  ctx.r4.u64 = guest::LoadU32(base, object + 1908);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1924);
  ctx.r4.u64 = guest::LoadU32(base, object + 2148);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2164);
  ctx.r4.u64 = guest::LoadU32(base, object + 2388);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2404);
  ctx.r4.u64 = guest::LoadU32(base, object + 2628);
  sub_820D16B0(ctx, base);
}

// PartsPoints::FindTailLights.
extern "C" REX_FUNC(sub_820DEA20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DEA20(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1680;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1920;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2160;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2400;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2640;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2880;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3120;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3360;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1924);
  ctx.r4.u64 = guest::LoadU32(base, object + 2148);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2164);
  ctx.r4.u64 = guest::LoadU32(base, object + 2388);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2404);
  ctx.r4.u64 = guest::LoadU32(base, object + 2628);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2644);
  ctx.r4.u64 = guest::LoadU32(base, object + 2868);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2884);
  ctx.r4.u64 = guest::LoadU32(base, object + 3108);
  sub_820D16B0(ctx, base);
}

// PartsPoints::FindFenders.
extern "C" REX_FUNC(sub_820DEAE8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DEAE8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 724);
  ctx.r4.u64 = guest::LoadU32(base, object + 948);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 964);
  ctx.r4.u64 = guest::LoadU32(base, object + 1188);
  sub_820D16B0(ctx, base);
}

// PartsPoints::FindCabin.
extern "C" REX_FUNC(sub_820DECA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DECA0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 4);
  ctx.r4.u64 = guest::LoadU32(base, object + 228);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82005FC4u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 244);
  ctx.r4.u64 = guest::LoadU32(base, object + 468);
  sub_820D16B0(ctx, base);
}

// CockpitPoints::FindHeadlights.
extern "C" REX_FUNC(sub_820DB778) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DB778(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1680;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1204);
  ctx.r4.u64 = guest::LoadU32(base, object + 1428);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1444);
  ctx.r4.u64 = guest::LoadU32(base, object + 1668);
  sub_820D16B0(ctx, base);
}

// InteriorPoints::FindMeters.
extern "C" REX_FUNC(sub_820E2E60) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E2E60(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 16;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 256;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 496;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 736;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 976;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1216;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1456;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1696;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1936;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2176;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2416;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2656;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2896;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3136;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3376;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3616;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 3856;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4096;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4336;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4576;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 4816;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 5056;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
}

// InteriorPoints::FindSeats.
extern "C" REX_FUNC(sub_820E3070) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E3070(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 16;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 256;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 496;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 736;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
}

// LightPoints::FindGroup.
extern "C" REX_FUNC(sub_820DC468) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DC468(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 16;
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 20);
  ctx.r4.u64 = guest::LoadU32(base, object + 244);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 256;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 260);
  ctx.r4.u64 = guest::LoadU32(base, object + 484);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 496;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 500);
  ctx.r4.u64 = guest::LoadU32(base, object + 724);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 736;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 740);
  ctx.r4.u64 = guest::LoadU32(base, object + 964);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 976;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 980);
  ctx.r4.u64 = guest::LoadU32(base, object + 1204);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 1216;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1220);
  ctx.r4.u64 = guest::LoadU32(base, object + 1444);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 1456;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1460);
  ctx.r4.u64 = guest::LoadU32(base, object + 1684);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 1696;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1700);
  ctx.r4.u64 = guest::LoadU32(base, object + 1924);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 1936;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1940);
  ctx.r4.u64 = guest::LoadU32(base, object + 2164);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 2176;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2180);
  ctx.r4.u64 = guest::LoadU32(base, object + 2404);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 2416;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2420);
  ctx.r4.u64 = guest::LoadU32(base, object + 2644);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 2656;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2660);
  ctx.r4.u64 = guest::LoadU32(base, object + 2884);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 2896;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2900);
  ctx.r4.u64 = guest::LoadU32(base, object + 3124);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 3136;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 3140);
  ctx.r4.u64 = guest::LoadU32(base, object + 3364);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 3376;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 3380);
  ctx.r4.u64 = guest::LoadU32(base, object + 3604);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 3616;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 3620);
  ctx.r4.u64 = guest::LoadU32(base, object + 3844);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 3856;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 3860);
  ctx.r4.u64 = guest::LoadU32(base, object + 4084);
  sub_820D16B0(ctx, base);
  ctx.r3.u64 = object + 4096;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82000A20u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 4100);
  ctx.r4.u64 = guest::LoadU32(base, object + 4324);
  sub_820D16B0(ctx, base);
}

// LightPoints::Find.
extern "C" REX_FUNC(sub_820DC570) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DC570(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 48;
  sub_820DC468(ctx, base);
  ctx.r3.u64 = object + 4384;
  ctx.r4.u64 = args[4];
  sub_820DC468(ctx, base);
  ctx.r3.u64 = object + 8720;
  ctx.r4.u64 = args[4];
  sub_820DC468(ctx, base);
}

// TrafficPoints::FindBody.
extern "C" REX_FUNC(sub_820E5128) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E5128(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  ctx.r4.u64 = args[7];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[6];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[6];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[5];
  sub_820CE8F0(ctx, base);
}

// TrafficPoints::FindHeadlights.
extern "C" REX_FUNC(sub_820E5328) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E5328(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82006400u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 1204);
  ctx.r4.u64 = guest::LoadU32(base, object + 1428);
  sub_820D16B0(ctx, base);
}

// TrafficPoints::FindTailLights.
extern "C" REX_FUNC(sub_820E53F8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E53F8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 240;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 480;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 720;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 960;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1200;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1440;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1680;
  ctx.r4.u64 = args[4];
  sub_820D0090(ctx, base);
  ctx.r3.u64 = object + 1920;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2160;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2400;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.r3.u64 = object + 2640;
  ctx.r4.u64 = args[4];
  sub_820CE8F0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82006400u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2164);
  ctx.r4.u64 = guest::LoadU32(base, object + 2388);
  sub_820D16B0(ctx, base);
  ctx.f1.f64 = double(std::bit_cast<float>(guest::LoadU32(base, 0x82006400u)));
  ctx.r3.u64 = guest::LoadU32(base, object + 2404);
  ctx.r4.u64 = guest::LoadU32(base, object + 2628);
  sub_820D16B0(ctx, base);
}

// SceneParts::Reset.
extern "C" REX_FUNC(sub_820E00B8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E00B8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 108;
  sub_820D31A0(ctx, base);
  ctx.r3.u64 = object + 176;
  sub_820DFC98(ctx, base);
  guest_memory::Apply(base, object, kSceneParts_Reset0, args);
  guest_memory::Clear(base, object + 32, 68);
}

// SceneCockpit::Reset.
extern "C" REX_FUNC(sub_820DBA90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DBA90(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneCockpit_Reset0, args);
  ctx.r3.u64 = object + 36;
  sub_820D3100(ctx, base);
  ctx.r3.u64 = object + 192;
  sub_820DB880(ctx, base);
  guest_memory::Apply(base, object, kSceneCockpit_Reset1, args);
}

// SceneInterior::Reset.
extern "C" REX_FUNC(sub_820E3310) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E3310(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneInterior_Reset0, args);
  ctx.r3.u64 = object + 40;
  sub_820D3488(ctx, base);
  ctx.r3.u64 = object + 160;
  sub_820E31A0(ctx, base);
  guest_memory::Apply(base, object, kSceneInterior_Reset1, args);
}

// SceneLights::Reset.
extern "C" REX_FUNC(sub_820DC6B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DC6B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneLights_Reset0, args);
  ctx.r3.u64 = object + 32;
  sub_820D3128(ctx, base);
  ctx.r3.u64 = object + 96;
  sub_820DC518(ctx, base);
  guest_memory::Apply(base, object, kSceneLights_Reset1, args);
}

// SceneTraffic::Reset.
extern "C" REX_FUNC(sub_820E5520) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E5520(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 32;
  sub_820D0848(ctx, base);
  ctx.r3.u64 = object + 3728;
  sub_820E5470(ctx, base);
  guest_memory::Apply(base, object, kSceneTraffic_Reset0, args);
}

// SceneWheels::Reset.
extern "C" REX_FUNC(sub_820E8490) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E8490(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneWheels_Reset0, args);
  ctx.r3.u64 = object + 192;
  ctx.r8.u64 = 0x0u;
  sub_820E81B0(ctx, base);
}

// SceneModelBlock::SceneModelBlock.
extern "C" REX_FUNC(sub_820D9DD0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D9DD0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneModelBlock_SceneModelBlock0, args);
  ctx.r3.u64 = object;
}

// PointRecord::PointRecord.
extern "C" REX_FUNC(sub_820D0030) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D0030(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPointRecord_PointRecord0, args);
  ctx.r3.u64 = object + 0;
  sub_820CE7C8(ctx, base);
  ctx.r3.u64 = object;
}


// The models' streams: r4 the mesh's word at +388, +268, +392 or +284.
#define SBX_MESH_WORD(address, at)                                            \
  extern "C" REX_FUNC(sub_##address) {                                        \
    REX_FUNC_PROLOGUE();                                                      \
    if (!On()) {                                                              \
      __imp__sub_##address(ctx, base);                                        \
      return;                                                                 \
    }                                                                         \
    ctx.r3.u64 = LoadU32(base, LoadU32(base, ctx.r3.u32 + 40) + 544 * ctx.r4.u32 + at); \
  }

SBX_MESH_WORD(8257A360, 388)  // Model::StreamData
SBX_MESH_WORD(8257A378, 268)  // Model::StreamSize
SBX_MESH_WORD(8257A390, 392)  // Model::StreamData2
SBX_MESH_WORD(8257A3A8, 284)  // Model::StreamSize2

#undef SBX_MESH_WORD

// Morph::Run: r4 the morph, its +8 and +12 = r5 and r6, then Morph::Blend
// (sub_825718A8) on it.
extern "C" REX_FUNC(sub_8257A610) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8257A610(ctx, base);
    return;
  }
  const uint32_t morph = ctx.r4.u32;
  StoreU32(base, morph + 8, ctx.r5.u32);
  StoreU32(base, morph + 12, ctx.r6.u32);
  ctx.r3.u64 = morph;
  sub_825718A8(ctx, base);
}

// PointRecord::SetNode and SetShape: r4's name to +160 or +192.
extern "C" REX_FUNC(sub_820CE698) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820CE698(ctx, base);
    return;
  }
  CopyName(base, ctx.r3.u32 + 160, ctx.r4.u32);
}

extern "C" REX_FUNC(sub_820CE6C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820CE6C0(ctx, base);
    return;
  }
  CopyName(base, ctx.r3.u32 + 192, ctx.r4.u32);
}
