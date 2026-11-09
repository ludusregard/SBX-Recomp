// The course scenery's objects as our own C++ (--native_course_scenery, on
// by default): the scenery pools' objects (their flags, keys, data and
// constructor), the pools' lookups and loads, the scenery's physical
// allocations and their count, a config's model asset, the course set's
// resets, and the texture animations' step (every frame).
//
// Written from SBX's generated code. A pool object: +0 its vtable, +4 its
// data's start, +8 its data's end (AllocData moves it), +16 and +24 the
// pool's, +32 its flags (0x1 and 0x8: loaded; 0x10 visible, 0x20
// expiring; 0x2 and 0x4 load flags), +36 its key. TextureAnimations::
// Advance (sub_820F1038) keeps subtick's override, which calls ours below
// in place of the recompiled function; the scenery's lists, parse, loads
// and draw stay recompiled.

#include "guest.h"
#include "hooks/native_calls.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_course_scenery, true, "SBX",
                    "Our own C++ for the course scenery's pool objects and lookups, its "
                    "physical allocations, the course set's resets and the texture animations' "
                    "step instead of the recompiled functions; the scenery's loads and draw "
                    "stay recompiled");

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

// PoolObject::Reset (sub_820FB8B8).
// 3 stores
constexpr Fill kPoolObject_Reset0[] = {
    {32, kWord, 0x0}, {36, kWord, 0xFFFFFFFF}, {12, kWord, 0x0},
};

// ModelList::Init (sub_820F18B0).
// 135 stores
constexpr Fill kModelList_Init0[] = {
    {6, kHalf, 0x0}, {4, kHalf, 0x1, 2, 6}, {8, kHalf, 0x2, 2, 6}, {12, kHalf, 0x3, 2, 6},
    {16, kHalf, 0x4, 2, 6}, {20, kHalf, 0x5, 2, 6}, {24, kHalf, 0x6, 2, 6}, {28, kHalf, 0x7, 2, 6},
    {32, kHalf, 0x8, 2, 6}, {36, kHalf, 0x9, 2, 6}, {40, kHalf, 0xA, 2, 6}, {44, kHalf, 0xB, 2, 6},
    {48, kHalf, 0xC, 2, 6}, {52, kHalf, 0xD, 2, 6}, {56, kHalf, 0xE, 2, 6}, {60, kHalf, 0xF, 2, 6},
    {64, kHalf, 0x10, 2, 6}, {68, kHalf, 0x11, 2, 6}, {72, kHalf, 0x12, 2, 6}, {76, kHalf, 0x13, 2, 6},
    {80, kHalf, 0x14, 2, 6}, {84, kHalf, 0x15, 2, 6}, {88, kHalf, 0x16, 2, 6}, {92, kHalf, 0x17, 2, 6},
    {96, kHalf, 0x18, 2, 6}, {100, kHalf, 0x19, 2, 6}, {104, kHalf, 0x1A, 2, 6}, {108, kHalf, 0x1B, 2, 6},
    {112, kHalf, 0x1C, 2, 6}, {116, kHalf, 0x1D, 2, 6}, {120, kHalf, 0x1E, 2, 6}, {124, kHalf, 0x1F, 2, 6},
    {128, kHalf, 0x20, 2, 6}, {132, kHalf, 0x21, 2, 6}, {136, kHalf, 0x22, 2, 6}, {140, kHalf, 0x23, 2, 6},
    {144, kHalf, 0x24, 2, 6}, {148, kHalf, 0x25, 2, 6}, {152, kHalf, 0x26, 2, 6}, {156, kHalf, 0x27, 2, 6},
    {160, kHalf, 0x28, 2, 6}, {164, kHalf, 0x29, 2, 6}, {168, kHalf, 0x2A, 2, 6}, {172, kHalf, 0x2B, 2, 6},
    {176, kHalf, 0x2C, 2, 6}, {180, kHalf, 0x2D, 2, 6}, {184, kHalf, 0x2E, 2, 6}, {188, kHalf, 0x2F, 2, 6},
    {192, kHalf, 0x30, 2, 6}, {196, kHalf, 0x31, 2, 6}, {200, kHalf, 0x32, 2, 6}, {204, kHalf, 0x33, 2, 6},
    {208, kHalf, 0x34, 2, 6}, {212, kHalf, 0x35, 2, 6}, {216, kHalf, 0x36, 2, 6}, {220, kHalf, 0x37, 2, 6},
    {224, kHalf, 0x38, 2, 6}, {228, kHalf, 0x39, 2, 6}, {232, kHalf, 0x3A, 2, 6}, {236, kHalf, 0x3B, 2, 6},
    {240, kHalf, 0x3C, 2, 6}, {244, kHalf, 0x3D, 2, 6}, {248, kHalf, 0x3E, 2, 6}, {252, kHalf, 0x3F, 2, 6},
    {256, kHalf, 0x40}, {260, kHalf, 0x0, 2, 8}, {256, kHalf, 0xFFFF, 2, 8}, {266, kHalf, 0xFFFF},
    {262, kHalf, 0xFFFF}, {0, kWord, 0x0},
};

// Scenery::ClearGlobal (sub_820F1310).
// 1 stores
constexpr Fill kScenery_ClearGlobal0[] = {
    {0x826B51F4, kAbsWord, 0x0},
};

// CourseSet::InitParts (sub_820F20C0).
// 6 stores
constexpr Fill kCourseSet_InitParts0[] = {
    {0, kWord, 0x0, 6, 4},
};
// 814 stores
constexpr Fill kCourseSet_InitParts1[] = {
    {470, kHalf, 0x0}, {468, kHalf, 0x1, 2, 6}, {472, kHalf, 0x2, 2, 6}, {476, kHalf, 0x3, 2, 6},
    {480, kHalf, 0x4, 2, 6}, {484, kHalf, 0x5, 2, 6}, {488, kHalf, 0x6, 2, 6}, {492, kHalf, 0x7, 2, 6},
    {496, kHalf, 0x8, 2, 6}, {500, kHalf, 0x9, 2, 6}, {504, kHalf, 0xA, 2, 6}, {508, kHalf, 0xB, 2, 6},
    {512, kHalf, 0xC, 2, 6}, {516, kHalf, 0xD, 2, 6}, {520, kHalf, 0xE, 2, 6}, {524, kHalf, 0xF, 2, 6},
    {528, kHalf, 0x10, 2, 6}, {532, kHalf, 0x11, 2, 6}, {536, kHalf, 0x12, 2, 6}, {540, kHalf, 0x13, 2, 6},
    {544, kHalf, 0x14, 2, 6}, {548, kHalf, 0x15, 2, 6}, {552, kHalf, 0x16, 2, 6}, {556, kHalf, 0x17, 2, 6},
    {560, kHalf, 0x18, 2, 6}, {564, kHalf, 0x19, 2, 6}, {568, kHalf, 0x1A, 2, 6}, {572, kHalf, 0x1B, 2, 6},
    {576, kHalf, 0x1C, 2, 6}, {580, kHalf, 0x1D, 2, 6}, {584, kHalf, 0x1E, 2, 6}, {588, kHalf, 0x1F, 2, 6},
    {592, kHalf, 0x20, 2, 6}, {596, kHalf, 0x21, 2, 6}, {600, kHalf, 0x22, 2, 6}, {604, kHalf, 0x23, 2, 6},
    {608, kHalf, 0x24, 2, 6}, {612, kHalf, 0x25, 2, 6}, {616, kHalf, 0x26, 2, 6}, {620, kHalf, 0x27, 2, 6},
    {624, kHalf, 0x28, 2, 6}, {628, kHalf, 0x29, 2, 6}, {632, kHalf, 0x2A, 2, 6}, {636, kHalf, 0x2B, 2, 6},
    {640, kHalf, 0x2C, 2, 6}, {644, kHalf, 0x2D, 2, 6}, {648, kHalf, 0x2E, 2, 6}, {652, kHalf, 0x2F, 2, 6},
    {656, kHalf, 0x30, 2, 6}, {660, kHalf, 0x31, 2, 6}, {664, kHalf, 0x32, 2, 6}, {668, kHalf, 0x33, 2, 6},
    {672, kHalf, 0x34, 2, 6}, {676, kHalf, 0x35, 2, 6}, {680, kHalf, 0x36, 2, 6}, {684, kHalf, 0x37, 2, 6},
    {688, kHalf, 0x38, 2, 6}, {692, kHalf, 0x39, 2, 6}, {696, kHalf, 0x3A, 2, 6}, {700, kHalf, 0x3B, 2, 6},
    {704, kHalf, 0x3C, 2, 6}, {708, kHalf, 0x3D, 2, 6}, {712, kHalf, 0x3E, 2, 6}, {716, kHalf, 0x3F, 2, 6},
    {720, kHalf, 0x40}, {724, kHalf, 0x0, 2, 8}, {720, kHalf, 0xFFFF, 2, 8}, {730, kHalf, 0xFFFF},
    {726, kHalf, 0xFFFF}, {464, kWord, 0x0}, {1006, kHalf, 0x0}, {1004, kHalf, 0x1, 2, 6},
    {1008, kHalf, 0x2, 2, 6}, {1012, kHalf, 0x3, 2, 6}, {1016, kHalf, 0x4, 2, 6}, {1020, kHalf, 0x5, 2, 6},
    {1024, kHalf, 0x6, 2, 6}, {1028, kHalf, 0x7, 2, 6}, {1032, kHalf, 0x8, 2, 6}, {1036, kHalf, 0x9, 2, 6},
    {1040, kHalf, 0xA, 2, 6}, {1044, kHalf, 0xB, 2, 6}, {1048, kHalf, 0xC, 2, 6}, {1052, kHalf, 0xD, 2, 6},
    {1056, kHalf, 0xE, 2, 6}, {1060, kHalf, 0xF, 2, 6}, {1064, kHalf, 0x10, 2, 6}, {1068, kHalf, 0x11, 2, 6},
    {1072, kHalf, 0x12, 2, 6}, {1076, kHalf, 0x13, 2, 6}, {1080, kHalf, 0x14, 2, 6}, {1084, kHalf, 0x15, 2, 6},
    {1088, kHalf, 0x16, 2, 6}, {1092, kHalf, 0x17, 2, 6}, {1096, kHalf, 0x18, 2, 6}, {1100, kHalf, 0x19, 2, 6},
    {1104, kHalf, 0x1A, 2, 6}, {1108, kHalf, 0x1B, 2, 6}, {1112, kHalf, 0x1C, 2, 6}, {1116, kHalf, 0x1D, 2, 6},
    {1120, kHalf, 0x1E, 2, 6}, {1124, kHalf, 0x1F, 2, 6}, {1128, kHalf, 0x20, 2, 6}, {1132, kHalf, 0x21, 2, 6},
    {1136, kHalf, 0x22, 2, 6}, {1140, kHalf, 0x23, 2, 6}, {1144, kHalf, 0x24, 2, 6}, {1148, kHalf, 0x25, 2, 6},
    {1152, kHalf, 0x26, 2, 6}, {1156, kHalf, 0x27, 2, 6}, {1160, kHalf, 0x28, 2, 6}, {1164, kHalf, 0x29, 2, 6},
    {1168, kHalf, 0x2A, 2, 6}, {1172, kHalf, 0x2B, 2, 6}, {1176, kHalf, 0x2C, 2, 6}, {1180, kHalf, 0x2D, 2, 6},
    {1184, kHalf, 0x2E, 2, 6}, {1188, kHalf, 0x2F, 2, 6}, {1192, kHalf, 0x30, 2, 6}, {1196, kHalf, 0x31, 2, 6},
    {1200, kHalf, 0x32, 2, 6}, {1204, kHalf, 0x33, 2, 6}, {1208, kHalf, 0x34, 2, 6}, {1212, kHalf, 0x35, 2, 6},
    {1216, kHalf, 0x36, 2, 6}, {1220, kHalf, 0x37, 2, 6}, {1224, kHalf, 0x38, 2, 6}, {1228, kHalf, 0x39, 2, 6},
    {1232, kHalf, 0x3A, 2, 6}, {1236, kHalf, 0x3B, 2, 6}, {1240, kHalf, 0x3C, 2, 6}, {1244, kHalf, 0x3D, 2, 6},
    {1248, kHalf, 0x3E, 2, 6}, {1252, kHalf, 0x3F, 2, 6}, {1256, kHalf, 0x40}, {1260, kHalf, 0x0, 2, 8},
    {1256, kHalf, 0xFFFF, 2, 8}, {1266, kHalf, 0xFFFF}, {1262, kHalf, 0xFFFF}, {1000, kWord, 0x0},
    {1538, kHalf, 0x0}, {1536, kHalf, 0x1, 2, 6}, {1540, kHalf, 0x2, 2, 6}, {1544, kHalf, 0x3, 2, 6},
    {1548, kHalf, 0x4, 2, 6}, {1552, kHalf, 0x5, 2, 6}, {1556, kHalf, 0x6, 2, 6}, {1560, kHalf, 0x7, 2, 6},
    {1564, kHalf, 0x8, 2, 6}, {1568, kHalf, 0x9, 2, 6}, {1572, kHalf, 0xA, 2, 6}, {1576, kHalf, 0xB, 2, 6},
    {1580, kHalf, 0xC, 2, 6}, {1584, kHalf, 0xD, 2, 6}, {1588, kHalf, 0xE, 2, 6}, {1592, kHalf, 0xF, 2, 6},
    {1596, kHalf, 0x10, 2, 6}, {1600, kHalf, 0x11, 2, 6}, {1604, kHalf, 0x12, 2, 6}, {1608, kHalf, 0x13, 2, 6},
    {1612, kHalf, 0x14, 2, 6}, {1616, kHalf, 0x15, 2, 6}, {1620, kHalf, 0x16, 2, 6}, {1624, kHalf, 0x17, 2, 6},
    {1628, kHalf, 0x18, 2, 6}, {1632, kHalf, 0x19, 2, 6}, {1636, kHalf, 0x1A, 2, 6}, {1640, kHalf, 0x1B, 2, 6},
    {1644, kHalf, 0x1C, 2, 6}, {1648, kHalf, 0x1D, 2, 6}, {1652, kHalf, 0x1E, 2, 6}, {1656, kHalf, 0x1F, 2, 6},
    {1660, kHalf, 0x20, 2, 6}, {1664, kHalf, 0x21, 2, 6}, {1668, kHalf, 0x22, 2, 6}, {1672, kHalf, 0x23, 2, 6},
    {1676, kHalf, 0x24, 2, 6}, {1680, kHalf, 0x25, 2, 6}, {1684, kHalf, 0x26, 2, 6}, {1688, kHalf, 0x27, 2, 6},
    {1692, kHalf, 0x28, 2, 6}, {1696, kHalf, 0x29, 2, 6}, {1700, kHalf, 0x2A, 2, 6}, {1704, kHalf, 0x2B, 2, 6},
    {1708, kHalf, 0x2C, 2, 6}, {1712, kHalf, 0x2D, 2, 6}, {1716, kHalf, 0x2E, 2, 6}, {1720, kHalf, 0x2F, 2, 6},
    {1724, kHalf, 0x30, 2, 6}, {1728, kHalf, 0x31, 2, 6}, {1732, kHalf, 0x32, 2, 6}, {1736, kHalf, 0x33, 2, 6},
    {1740, kHalf, 0x34, 2, 6}, {1744, kHalf, 0x35, 2, 6}, {1748, kHalf, 0x36, 2, 6}, {1752, kHalf, 0x37, 2, 6},
    {1756, kHalf, 0x38, 2, 6}, {1760, kHalf, 0x39, 2, 6}, {1764, kHalf, 0x3A, 2, 6}, {1768, kHalf, 0x3B, 2, 6},
    {1772, kHalf, 0x3C, 2, 6}, {1776, kHalf, 0x3D, 2, 6}, {1780, kHalf, 0x3E, 2, 6}, {1784, kHalf, 0x3F, 2, 6},
    {1788, kHalf, 0x40}, {1792, kHalf, 0x0, 2, 8}, {1788, kHalf, 0xFFFF, 2, 8}, {1798, kHalf, 0xFFFF},
    {1794, kHalf, 0xFFFF}, {1532, kWord, 0x0}, {2066, kHalf, 0x0}, {2064, kHalf, 0x1, 2, 6},
    {2068, kHalf, 0x2, 2, 6}, {2072, kHalf, 0x3, 2, 6}, {2076, kHalf, 0x4, 2, 6}, {2080, kHalf, 0x5, 2, 6},
    {2084, kHalf, 0x6, 2, 6}, {2088, kHalf, 0x7, 2, 6}, {2092, kHalf, 0x8, 2, 6}, {2096, kHalf, 0x9, 2, 6},
    {2100, kHalf, 0xA, 2, 6}, {2104, kHalf, 0xB, 2, 6}, {2108, kHalf, 0xC, 2, 6}, {2112, kHalf, 0xD, 2, 6},
    {2116, kHalf, 0xE, 2, 6}, {2120, kHalf, 0xF, 2, 6}, {2124, kHalf, 0x10, 2, 6}, {2128, kHalf, 0x11, 2, 6},
    {2132, kHalf, 0x12, 2, 6}, {2136, kHalf, 0x13, 2, 6}, {2140, kHalf, 0x14, 2, 6}, {2144, kHalf, 0x15, 2, 6},
    {2148, kHalf, 0x16, 2, 6}, {2152, kHalf, 0x17, 2, 6}, {2156, kHalf, 0x18, 2, 6}, {2160, kHalf, 0x19, 2, 6},
    {2164, kHalf, 0x1A, 2, 6}, {2168, kHalf, 0x1B, 2, 6}, {2172, kHalf, 0x1C, 2, 6}, {2176, kHalf, 0x1D, 2, 6},
    {2180, kHalf, 0x1E, 2, 6}, {2184, kHalf, 0x1F, 2, 6}, {2188, kHalf, 0x20, 2, 6}, {2192, kHalf, 0x21, 2, 6},
    {2196, kHalf, 0x22, 2, 6}, {2200, kHalf, 0x23, 2, 6}, {2204, kHalf, 0x24, 2, 6}, {2208, kHalf, 0x25, 2, 6},
    {2212, kHalf, 0x26, 2, 6}, {2216, kHalf, 0x27, 2, 6}, {2220, kHalf, 0x28, 2, 6}, {2224, kHalf, 0x29, 2, 6},
    {2228, kHalf, 0x2A, 2, 6}, {2232, kHalf, 0x2B, 2, 6}, {2236, kHalf, 0x2C, 2, 6}, {2240, kHalf, 0x2D, 2, 6},
    {2244, kHalf, 0x2E, 2, 6}, {2248, kHalf, 0x2F, 2, 6}, {2252, kHalf, 0x30, 2, 6}, {2256, kHalf, 0x31, 2, 6},
    {2260, kHalf, 0x32, 2, 6}, {2264, kHalf, 0x33, 2, 6}, {2268, kHalf, 0x34, 2, 6}, {2272, kHalf, 0x35, 2, 6},
    {2276, kHalf, 0x36, 2, 6}, {2280, kHalf, 0x37, 2, 6}, {2284, kHalf, 0x38, 2, 6}, {2288, kHalf, 0x39, 2, 6},
    {2292, kHalf, 0x3A, 2, 6}, {2296, kHalf, 0x3B, 2, 6}, {2300, kHalf, 0x3C, 2, 6}, {2304, kHalf, 0x3D, 2, 6},
    {2308, kHalf, 0x3E, 2, 6}, {2312, kHalf, 0x3F, 2, 6}, {2316, kHalf, 0x40}, {2320, kHalf, 0x0, 2, 8},
    {2316, kHalf, 0xFFFF, 2, 8}, {2326, kHalf, 0xFFFF}, {2322, kHalf, 0xFFFF}, {2060, kWord, 0x0},
    {2594, kHalf, 0x0}, {2592, kHalf, 0x1, 2, 6}, {2596, kHalf, 0x2, 2, 6}, {2600, kHalf, 0x3, 2, 6},
    {2604, kHalf, 0x4, 2, 6}, {2608, kHalf, 0x5, 2, 6}, {2612, kHalf, 0x6, 2, 6}, {2616, kHalf, 0x7, 2, 6},
    {2620, kHalf, 0x8, 2, 6}, {2624, kHalf, 0x9, 2, 6}, {2628, kHalf, 0xA, 2, 6}, {2632, kHalf, 0xB, 2, 6},
    {2636, kHalf, 0xC, 2, 6}, {2640, kHalf, 0xD, 2, 6}, {2644, kHalf, 0xE, 2, 6}, {2648, kHalf, 0xF, 2, 6},
    {2652, kHalf, 0x10, 2, 6}, {2656, kHalf, 0x11, 2, 6}, {2660, kHalf, 0x12, 2, 6}, {2664, kHalf, 0x13, 2, 6},
    {2668, kHalf, 0x14, 2, 6}, {2672, kHalf, 0x15, 2, 6}, {2676, kHalf, 0x16, 2, 6}, {2680, kHalf, 0x17, 2, 6},
    {2684, kHalf, 0x18, 2, 6}, {2688, kHalf, 0x19, 2, 6}, {2692, kHalf, 0x1A, 2, 6}, {2696, kHalf, 0x1B, 2, 6},
    {2700, kHalf, 0x1C, 2, 6}, {2704, kHalf, 0x1D, 2, 6}, {2708, kHalf, 0x1E, 2, 6}, {2712, kHalf, 0x1F, 2, 6},
    {2716, kHalf, 0x20, 2, 6}, {2720, kHalf, 0x21, 2, 6}, {2724, kHalf, 0x22, 2, 6}, {2728, kHalf, 0x23, 2, 6},
    {2732, kHalf, 0x24, 2, 6}, {2736, kHalf, 0x25, 2, 6}, {2740, kHalf, 0x26, 2, 6}, {2744, kHalf, 0x27, 2, 6},
    {2748, kHalf, 0x28, 2, 6}, {2752, kHalf, 0x29, 2, 6}, {2756, kHalf, 0x2A, 2, 6}, {2760, kHalf, 0x2B, 2, 6},
    {2764, kHalf, 0x2C, 2, 6}, {2768, kHalf, 0x2D, 2, 6}, {2772, kHalf, 0x2E, 2, 6}, {2776, kHalf, 0x2F, 2, 6},
    {2780, kHalf, 0x30, 2, 6}, {2784, kHalf, 0x31, 2, 6}, {2788, kHalf, 0x32, 2, 6}, {2792, kHalf, 0x33, 2, 6},
    {2796, kHalf, 0x34, 2, 6}, {2800, kHalf, 0x35, 2, 6}, {2804, kHalf, 0x36, 2, 6}, {2808, kHalf, 0x37, 2, 6},
    {2812, kHalf, 0x38, 2, 6}, {2816, kHalf, 0x39, 2, 6}, {2820, kHalf, 0x3A, 2, 6}, {2824, kHalf, 0x3B, 2, 6},
    {2828, kHalf, 0x3C, 2, 6}, {2832, kHalf, 0x3D, 2, 6}, {2836, kHalf, 0x3E, 2, 6}, {2840, kHalf, 0x3F, 2, 6},
    {2844, kHalf, 0x40}, {2848, kHalf, 0x0, 2, 8}, {2844, kHalf, 0xFFFF, 2, 8}, {2854, kHalf, 0xFFFF},
    {2850, kHalf, 0xFFFF}, {2588, kWord, 0x0}, {3122, kHalf, 0x0}, {3120, kHalf, 0x1, 2, 6},
    {3124, kHalf, 0x2, 2, 6}, {3128, kHalf, 0x3, 2, 6}, {3132, kHalf, 0x4, 2, 6}, {3136, kHalf, 0x5, 2, 6},
    {3140, kHalf, 0x6, 2, 6}, {3144, kHalf, 0x7, 2, 6}, {3148, kHalf, 0x8, 2, 6}, {3152, kHalf, 0x9, 2, 6},
    {3156, kHalf, 0xA, 2, 6}, {3160, kHalf, 0xB, 2, 6}, {3164, kHalf, 0xC, 2, 6}, {3168, kHalf, 0xD, 2, 6},
    {3172, kHalf, 0xE, 2, 6}, {3176, kHalf, 0xF, 2, 6}, {3180, kHalf, 0x10, 2, 6}, {3184, kHalf, 0x11, 2, 6},
    {3188, kHalf, 0x12, 2, 6}, {3192, kHalf, 0x13, 2, 6}, {3196, kHalf, 0x14, 2, 6}, {3200, kHalf, 0x15, 2, 6},
    {3204, kHalf, 0x16, 2, 6}, {3208, kHalf, 0x17, 2, 6}, {3212, kHalf, 0x18, 2, 6}, {3216, kHalf, 0x19, 2, 6},
    {3220, kHalf, 0x1A, 2, 6}, {3224, kHalf, 0x1B, 2, 6}, {3228, kHalf, 0x1C, 2, 6}, {3232, kHalf, 0x1D, 2, 6},
    {3236, kHalf, 0x1E, 2, 6}, {3240, kHalf, 0x1F, 2, 6}, {3244, kHalf, 0x20, 2, 6}, {3248, kHalf, 0x21, 2, 6},
    {3252, kHalf, 0x22, 2, 6}, {3256, kHalf, 0x23, 2, 6}, {3260, kHalf, 0x24, 2, 6}, {3264, kHalf, 0x25, 2, 6},
    {3268, kHalf, 0x26, 2, 6}, {3272, kHalf, 0x27, 2, 6}, {3276, kHalf, 0x28, 2, 6}, {3280, kHalf, 0x29, 2, 6},
    {3284, kHalf, 0x2A, 2, 6}, {3288, kHalf, 0x2B, 2, 6}, {3292, kHalf, 0x2C, 2, 6}, {3296, kHalf, 0x2D, 2, 6},
    {3300, kHalf, 0x2E, 2, 6}, {3304, kHalf, 0x2F, 2, 6}, {3308, kHalf, 0x30, 2, 6}, {3312, kHalf, 0x31, 2, 6},
    {3316, kHalf, 0x32, 2, 6}, {3320, kHalf, 0x33, 2, 6}, {3324, kHalf, 0x34, 2, 6}, {3328, kHalf, 0x35, 2, 6},
    {3332, kHalf, 0x36, 2, 6}, {3336, kHalf, 0x37, 2, 6}, {3340, kHalf, 0x38, 2, 6}, {3344, kHalf, 0x39, 2, 6},
    {3348, kHalf, 0x3A, 2, 6}, {3352, kHalf, 0x3B, 2, 6}, {3356, kHalf, 0x3C, 2, 6}, {3360, kHalf, 0x3D, 2, 6},
    {3364, kHalf, 0x3E, 2, 6}, {3368, kHalf, 0x3F, 2, 6}, {3372, kHalf, 0x40}, {3376, kHalf, 0x0, 2, 8},
    {3372, kHalf, 0xFFFF, 2, 8}, {3382, kHalf, 0xFFFF}, {3378, kHalf, 0xFFFF}, {3116, kWord, 0x0, 2, 608},
    {3728, kWord, 0x0, 3, 4},
};

// CourseSet::Reset (sub_820F0FC8).
// 3 stores
constexpr Fill kCourseSet_Reset0[] = {
    {32, kWord, 0x0}, {36, kWord, 0xFFFFFFFF}, {12, kWord, 0x0},
};

bool On() { return REXCVAR_GET(native_course_scenery); }

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kPoolObjectVtable = 0x82006820;
constexpr uint32_t kPhysicalBytes = 0x826B51F8;  // the scenery's physical memory in use

uint32_t Flags(uint8_t* base, uint32_t object) { return LoadU32(base, object + 32); }

// ObjectPools::Object's address: the pool r4's (24 bytes each from [+4])
// object r5.
uint32_t PoolObject(uint8_t* base, uint32_t pools, uint32_t pool, uint32_t index) {
  const uint32_t entry = LoadU32(base, pools + 4) + 24 * pool;
  return (LoadU32(base, entry + 8) + LoadU32(base, entry + 4)) * index +
         LoadU32(base, entry + 12);
}

// Animation::Advance (sub_82145350): +8 its frame (back to 0, with +4 the
// key, past +12), +4 the key, moved on when the frame reaches the next
// key's (+16 + 4 x key) and there is one (+0 the count).
void AdvanceAnimation(uint8_t* base, uint32_t a) {
  if (int32_t(LoadU32(base, a + 8)) > int32_t(LoadU32(base, a + 12))) {
    StoreU32(base, a + 8, 0);
    StoreU32(base, a + 4, 0);
  }
  const int32_t next = int32_t(LoadU32(base, a + 4)) + 1;
  if (next < int32_t(LoadU32(base, a + 0)) &&
      !(int32_t(LoadU32(base, a + 8)) < int32_t(LoadU32(base, a + 4 * uint32_t(next + 4))))) {
    StoreU32(base, a + 4, uint32_t(next));
  }
  StoreU32(base, a + 8, LoadU32(base, a + 8) + 1);
}

// The size of a physical block (sub_82294008).
uint32_t PhysicalSize(PPCContext& ctx, uint8_t* base, uint32_t block) {
  ctx.r3.u64 = block;
  sub_82294008(ctx, base);
  return ctx.r3.u32;
}

}  // namespace

// PoolObject::Reset.
extern "C" REX_FUNC(sub_820FB8B8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FB8B8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPoolObject_Reset0, args);
  ctx.r3.u64 = object;
}

// ModelList::Init.
extern "C" REX_FUNC(sub_820F18B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F18B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kModelList_Init0, args);
  ctx.r3.u64 = object;
}

// Scenery::ClearGlobal.
extern "C" REX_FUNC(sub_820F1310) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F1310(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kScenery_ClearGlobal0, args);
  ctx.r3.u64 = object;
}

// CourseSet::InitParts.
extern "C" REX_FUNC(sub_820F20C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F20C0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCourseSet_InitParts0, args);
  ctx.r3.u64 = object + 32;
  sub_8257A1C0(ctx, base);
  guest_memory::Apply(base, object, kCourseSet_InitParts1, args);
  ctx.r3.u64 = object;
}

// CourseSet::Reset.
extern "C" REX_FUNC(sub_820F0FC8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0FC8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 64;
  sub_8257C9C0(ctx, base);
  ctx.r3.u64 = object + 96;
  sub_8257A270(ctx, base);
  ctx.r3.u64 = object + 528;
  sub_82146848(ctx, base);
  ctx.r3.u64 = object + 1064;
  sub_82146848(ctx, base);
  ctx.r3.u64 = object + 1596;
  sub_82146848(ctx, base);
  ctx.r3.u64 = object + 2124;
  sub_82146848(ctx, base);
  ctx.r3.u64 = object + 2652;
  sub_82146848(ctx, base);
  ctx.r3.u64 = object + 3180;
  sub_82146848(ctx, base);
  guest_memory::Apply(base, object, kCourseSet_Reset0, args);
  ctx.r3.u64 = object;
}


// PoolObject::AllocData: r4 bytes (rounded up to 64) from the object's
// data; returns where they start.
extern "C" REX_FUNC(sub_820FB798) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FB798(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const uint32_t end = LoadU32(base, object + 8);
  ctx.r3.u64 = end + LoadU32(base, object + 0);
  StoreU32(base, object + 8, ((ctx.r4.u32 + 63) & ~63u) + end);
}

// PoolObject::IsLoaded (0x9), IsVisible (0x19), IsExpiring (0x29): all of
// the bits set.
#define SBX_FLAGS_SET(address, bits)                                       \
  extern "C" REX_FUNC(sub_##address) {                                     \
    REX_FUNC_PROLOGUE();                                                   \
    if (!On()) {                                                           \
      __imp__sub_##address(ctx, base);                                     \
      return;                                                              \
    }                                                                      \
    ctx.r3.u64 = (Flags(base, ctx.r3.u32) & bits) == bits ? 1 : 0;         \
  }

SBX_FLAGS_SET(820FB7D0, 0x9u)
SBX_FLAGS_SET(820FB7E8, 0x19u)
SBX_FLAGS_SET(820FB800, 0x29u)

#undef SBX_FLAGS_SET

// PoolObject::SetVisible (0x10) and SetExpiring (0x20): r4 0 clears the
// bit; else, once loaded, the two bits 0x30 become the one.
#define SBX_FLAG_SETTER(address, bit)                                      \
  extern "C" REX_FUNC(sub_##address) {                                     \
    REX_FUNC_PROLOGUE();                                                   \
    if (!On()) {                                                           \
      __imp__sub_##address(ctx, base);                                     \
      return;                                                              \
    }                                                                      \
    const uint32_t object = ctx.r3.u32;                                    \
    uint32_t flags = Flags(base, object);                                  \
    if (int32_t(ctx.r4.u32) == 0) {                                        \
      flags &= ~bit;                                                       \
    } else {                                                               \
      if ((flags & 0x9u) != 0x9u) {                                        \
        return;                                                            \
      }                                                                    \
      flags = (flags & ~0x30u) | bit;                                      \
    }                                                                      \
    StoreU32(base, object + 32, flags);                                    \
  }

SBX_FLAG_SETTER(820FB818, 0x10u)
SBX_FLAG_SETTER(820FB848, 0x20u)

#undef SBX_FLAG_SETTER

// PoolObject::ClearLoadFlags: 0x3C cleared.
extern "C" REX_FUNC(sub_820FB878) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FB878(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + 32, Flags(base, ctx.r3.u32) & ~0x3Cu);
}

// PoolObject::MarkLoaded: 0x2 cleared, 0x8 set.
extern "C" REX_FUNC(sub_820FB888) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FB888(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + 32, (Flags(base, ctx.r3.u32) & ~0x2u) | 0x8u);
}

// The keys (each class's vtable slot 0): the key's size into [r4], its
// address in r3. PoolObject::Key (shared): none.
extern "C" REX_FUNC(sub_824A7668) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_824A7668(ctx, base);
    return;
  }
  StoreU32(base, ctx.r4.u32, 0);
  ctx.r3.u64 = 0;
}

// AreaLights::Key: 4 bytes at +84.
extern "C" REX_FUNC(sub_820F1AB0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F1AB0(ctx, base);
    return;
  }
  StoreU32(base, ctx.r4.u32, 4);
  ctx.r3.u64 = ctx.r3.u32 + 84;
}

// AreaModel::Key: 4 bytes at +64.
extern "C" REX_FUNC(sub_820F1B68) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F1B68(ctx, base);
    return;
  }
  StoreU32(base, ctx.r4.u32, 4);
  ctx.r3.u64 = ctx.r3.u32 + 64;
}

// PoolObject::PoolObject: r4 its data's offset from r3, r5 its size, r6 the
// pool.
extern "C" REX_FUNC(sub_820FB9F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FB9F0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const uint32_t data = ctx.r4.u32 + object;
  StoreU32(base, object + 0, kPoolObjectVtable);
  StoreU32(base, object + 8, ctx.r5.u32);
  StoreU32(base, object + 12, 0);
  StoreU32(base, object + 4, data);
  StoreU32(base, object + 28, 0);
  StoreU32(base, object + 16, ctx.r6.u32);
  StoreU32(base, object + 24, data + ctx.r5.u32);
  StoreU32(base, object + 32, 0);
  StoreU32(base, object + 36, 0xFFFFFFFFu);
  StoreU32(base, object + 12, 0);
}

// PoolObject::SetKey: r4 the key's source, copied (memcpy) to where the
// object's Key slot says, for the size it gives (in the original's
// 112-byte frame at +80).
extern "C" REX_FUNC(sub_820FB8D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FB8D0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32, source = ctx.r4.u32;
  guest::StackFrame frame(ctx, 112);
  const uint32_t size_at = frame.Local(80);
  StoreU32(base, size_at, 0);
  ctx.r3.u64 = object;
  ctx.r4.u64 = size_at;
  REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, object)));
  const uint32_t size = LoadU32(base, size_at);
  if (int32_t(size) != 0 && ctx.r3.u32 != 0) {
    guest_memory::Memcpy(ctx, base, ctx.r3.u32, source, size);
  }
}

// ObjectPools::Pool: r4's entry (24 bytes from [+4]).
extern "C" REX_FUNC(sub_820FB9E0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FB9E0(ctx, base);
    return;
  }
  ctx.r3.u64 = 24 * ctx.r4.u32 + LoadU32(base, ctx.r3.u32 + 4);
}

extern "C" REX_FUNC(sub_820FBFB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FBFB8(ctx, base);
    return;
  }
  ctx.r3.u64 = PoolObject(base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);
}

// ObjectPools::Load: the object, its vtable slot 1 (+4) when there is one;
// returns it.
extern "C" REX_FUNC(sub_820FBEA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820FBEA0(ctx, base);
    return;
  }
  const uint32_t object = PoolObject(base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);
  if (object != 0) {
    ctx.r3.u64 = object;
    REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, object) + 4));
  }
  ctx.r3.u64 = object;
}

// SceneryConfig::ModelAsset: r4 the model, r5 the variant: the model's
// asset range ([+28]'s entry: +24 the first, +28 the last), its r5-th
// asset, or -1.
extern "C" REX_FUNC(sub_820F13C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F13C0(ctx, base);
    return;
  }
  const uint32_t config = LoadU32(base, ctx.r3.u32 + 28);
  const uint32_t model = ctx.r4.u32, variant = ctx.r5.u32;
  ctx.r3.u64 = 0xFFFFFFFFFFFFFFFFull;
  if (model >= LoadU32(base, config + 16)) {
    return;
  }
  const uint32_t entry = config + 12 * model;
  const uint32_t first = LoadU32(base, entry + 24), last = LoadU32(base, entry + 28);
  if (first > last) {
    return;
  }
  for (uint32_t asset = first, i = 0; asset <= last; asset++, i++) {
    if (i == variant) {
      ctx.r3.u64 = asset;
      return;
    }
  }
}

// SceneryList::Clear: r4's +4 and +8 cleared.
extern "C" REX_FUNC(sub_820F1418) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F1418(ctx, base);
    return;
  }
  StoreU32(base, ctx.r4.u32 + 4, 0);
  StoreU32(base, ctx.r4.u32 + 8, 0);
}

// Scenery::AllocPhysical: r3 bytes of physical memory (sub_82293F10: -1,
// 0, PAGE_READWRITE), counted in.
extern "C" REX_FUNC(sub_820F1320) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F1320(ctx, base);
    return;
  }
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r5.u64 = 0;
  ctx.r6.u64 = 4;
  sub_82293F10(ctx, base);
  const uint32_t block = ctx.r3.u32;
  const uint32_t size = PhysicalSize(ctx, base, block);
  StoreU32(base, kPhysicalBytes, size + LoadU32(base, kPhysicalBytes));
  ctx.r3.u64 = block;
}

// Scenery::FreePhysical: r3's size counted out, then freed (sub_82294010).
extern "C" REX_FUNC(sub_820F1378) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F1378(ctx, base);
    return;
  }
  const uint32_t block = ctx.r3.u32;
  const uint32_t size = PhysicalSize(ctx, base, block);
  StoreU32(base, kPhysicalBytes, LoadU32(base, kPhysicalBytes) - size);
  ctx.r3.u64 = block;
  sub_82294010(ctx, base);
}

extern "C" REX_FUNC(sub_82145350) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82145350(ctx, base);
    return;
  }
  AdvanceAnimation(base, ctx.r3.u32);
}

// AnimationSet::Advance: each animation (+0 the count, +4 their offsets
// from the end of that table) one step on.
extern "C" REX_FUNC(sub_821453B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821453B0(ctx, base);
    return;
  }
  const uint32_t set = ctx.r3.u32;
  for (int32_t i = 0; i < int32_t(LoadU32(base, set)); i++) {
    const uint32_t count = LoadU32(base, set);
    AdvanceAnimation(base, set + ((count + 1) << 2) + LoadU32(base, set + 4 + 4 * uint32_t(i)));
  }
}

// TextureAnimations::Advance: r3 the course set; its animation set (+1060)
// advanced (AnimationSet::Advance, sub_821453B0, a tail call) when it has
// one. Called from subtick's override (subtick/per_frame_logic.cpp), which
// skips it on render-only passes.
void hooks::NativeCourseScenery_AdvanceTextureAnimations(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_820F1038(ctx, base);
    return;
  }
  ctx.r3.u64 = guest::LoadU32(base, ctx.r3.u32 + 1060);
  if (ctx.r3.u32 == 0) {
    return;
  }
  sub_821453B0(ctx, base);
}
