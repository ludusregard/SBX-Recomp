// The scene models' records and links as our own C++
// (--native_scene_models, on by default): the parts' and interior's links,
// the records' defaults and copies, the accessories' getter, the sound
// loads' callback, the car model's pages, SceneModel::Update (every frame)
// and the Apply steps a model's record goes through.
//
// Written from SBX's generated code. The records' defaults and copies are
// long runs of stores; they're written as tables here, read from SBX's
// code in its order (the floats are the image's constants, copied by their
// bits as the originals' lfs/stfs do). memcpy (sub_8258CB90, wrapped by the
// native renderer) and the calls into other parts of the game stay
// recompiled.
//
// SBX's layouts differ from ITC-Recomp's here: CarRecord::Copy copies
// +3632 with a 44-byte memcpy (ITC 48) and its other fields at SBX's own
// offsets (the tables below); SceneModel::Update tests the scene's +5000.
//
// A SceneModelRecord (4,244 bytes): +0 its kind, +16 the car record (3,696
// bytes), +3712 the parts record (60), +3772 the interior's (120), +3892
// the cockpit's (144), +4036 the wheels' (156), +4192 pool 4's (52).

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scene_models, true, "SBX",
                    "Our own C++ for the scene models (the records' defaults and copies, the "
                    "parts' links, SceneModel::Update and the Apply steps) instead of the "
                    "recompiled functions; memcpy and the models' builds stay recompiled");

namespace {

using guest::LoadU32;
using guest::StoreU32;
using guest_memory::Fill;
using guest_memory::Run;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kWord;

constexpr uint32_t kAccessories = 0x826B51F0;  // [it] the accessories' set
constexpr uint32_t kSoundLoadsDone = 0x826C7DF8;  // a byte a slot
constexpr uint32_t kSoundLoaded = 0x826C7E54;     // a word a slot
constexpr uint32_t kRecord12Value = 0x82001BD8;
constexpr uint32_t kOneBits = 0x3F800000;  // 1.0f

// The records' defaults, in the originals' order. LayerReset (sub_820D07D0)
// is a 64-byte vinyl layer or sticker; CarRecordReset (sub_820D0848) has
// its 54 LayerResets inline, as the original's loops leave them.
constexpr Fill kLayerReset0[] = {
    {4, kWord, 0x0}, {8, kWord, 0x8}, {0, kWord, 0xFFFFFFFF}, {16, kImage, 0x82000A24},
    {20, kImage, 0x82000A24}, {24, kImage, 0x82000A24}, {28, kImage, 0x82000A20}, {32, kImage, 0x82000A24},
    {48, kImage, 0x82000A20}, {52, kImage, 0x82000A20}, {56, kImage, 0x82000A20}, {60, kImage, 0x82000A20},
};
constexpr Fill kCarRecordReset0[] = {
    {0, kWord, 0xFFFFFFFF}, {16, kImage, 0x82000A24}, {20, kImage, 0x82000A24}, {24, kImage, 0x82000A20},
    {28, kImage, 0x82000A20}, {32, kImage, 0x82000A24}, {36, kImage, 0x82000A24}, {40, kImage, 0x82000A20},
    {44, kImage, 0x82000A20}, {48, kImage, 0x82000A24}, {52, kImage, 0x82000A24}, {56, kImage, 0x820029E8},
    {60, kImage, 0x82000A20}, {64, kImage, 0x82000A24}, {68, kImage, 0x82000A24}, {72, kImage, 0x82000A20},
    {76, kImage, 0x82000A20}, {80, kImage, 0x82000A24}, {84, kImage, 0x82000A24}, {88, kImage, 0x82000A20},
    {92, kImage, 0x82000A20}, {96, kImage, 0x82000A24}, {100, kImage, 0x82000A24}, {104, kImage, 0x82002A68},
    {108, kImage, 0x82000A20}, {112, kImage, 0x82000A24}, {116, kImage, 0x82000A24}, {120, kImage, 0x82000A20},
    {124, kImage, 0x82000A20}, {128, kImage, 0x82000A24}, {132, kImage, 0x82000A24}, {136, kImage, 0x82000A20},
    {140, kImage, 0x82000A20}, {144, kImage, 0x82000A24}, {148, kImage, 0x82000A24}, {152, kImage, 0x82002A68},
    {156, kImage, 0x82000A20}, {160, kWord, 0x0}, {164, kWord, 0x0}, {180, kWord, 0x0},
    {184, kWord, 0x8}, {176, kWord, 0xFFFFFFFF}, {192, kImage, 0x82000A24}, {196, kImage, 0x82000A24},
    {200, kImage, 0x82000A24}, {204, kImage, 0x82000A20}, {208, kImage, 0x82000A24}, {224, kImage, 0x82000A20},
    {228, kImage, 0x82000A20}, {232, kImage, 0x82000A20}, {236, kImage, 0x82000A20}, {184, kWord, 0x8},
    {372, kWord, 0x0}, {376, kWord, 0x8}, {368, kWord, 0xFFFFFFFF}, {384, kImage, 0x82000A24},
    {388, kImage, 0x82000A24}, {392, kImage, 0x82000A24}, {396, kImage, 0x82000A20}, {400, kImage, 0x82000A24},
    {416, kImage, 0x82000A20}, {420, kImage, 0x82000A20}, {424, kImage, 0x82000A20}, {428, kImage, 0x82000A20},
    {376, kWord, 0x0}, {244, kWord, 0x0}, {248, kWord, 0x8}, {240, kWord, 0xFFFFFFFF},
    {256, kImage, 0x82000A24}, {260, kImage, 0x82000A24}, {264, kImage, 0x82000A24}, {268, kImage, 0x82000A20},
    {272, kImage, 0x82000A24}, {288, kImage, 0x82000A20}, {292, kImage, 0x82000A20}, {296, kImage, 0x82000A20},
    {300, kImage, 0x82000A20}, {248, kWord, 0x8}, {436, kWord, 0x0}, {440, kWord, 0x8},
    {432, kWord, 0xFFFFFFFF}, {448, kImage, 0x82000A24}, {452, kImage, 0x82000A24}, {456, kImage, 0x82000A24},
    {460, kImage, 0x82000A20}, {464, kImage, 0x82000A24}, {480, kImage, 0x82000A20}, {484, kImage, 0x82000A20},
    {488, kImage, 0x82000A20}, {492, kImage, 0x82000A20}, {440, kWord, 0x0}, {308, kWord, 0x0},
    {312, kWord, 0x8}, {304, kWord, 0xFFFFFFFF}, {320, kImage, 0x82000A24}, {324, kImage, 0x82000A24},
    {328, kImage, 0x82000A24}, {332, kImage, 0x82000A20}, {336, kImage, 0x82000A24}, {352, kImage, 0x82000A20},
    {356, kImage, 0x82000A20}, {360, kImage, 0x82000A20}, {364, kImage, 0x82000A20}, {312, kWord, 0x8},
    {500, kWord, 0x0}, {504, kWord, 0x8}, {496, kWord, 0xFFFFFFFF}, {512, kImage, 0x82000A24},
    {516, kImage, 0x82000A24}, {520, kImage, 0x82000A24}, {524, kImage, 0x82000A20}, {528, kImage, 0x82000A24},
    {544, kImage, 0x82000A20}, {548, kImage, 0x82000A20}, {552, kImage, 0x82000A20}, {556, kImage, 0x82000A20},
    {504, kWord, 0x0}, {564, kWord, 0x0}, {568, kWord, 0x8}, {560, kWord, 0xFFFFFFFF},
    {576, kImage, 0x82000A24}, {580, kImage, 0x82000A24}, {584, kImage, 0x82000A24}, {588, kImage, 0x82000A20},
    {592, kImage, 0x82000A24}, {608, kImage, 0x82000A20}, {612, kImage, 0x82000A20}, {616, kImage, 0x82000A20},
    {620, kImage, 0x82000A20}, {568, kWord, 0x8}, {948, kWord, 0x0}, {952, kWord, 0x8},
    {944, kWord, 0xFFFFFFFF}, {960, kImage, 0x82000A24}, {964, kImage, 0x82000A24}, {968, kImage, 0x82000A24},
    {972, kImage, 0x82000A20}, {976, kImage, 0x82000A24}, {992, kImage, 0x82000A20}, {996, kImage, 0x82000A20},
    {1000, kImage, 0x82000A20}, {1004, kImage, 0x82000A20}, {952, kWord, 0x2}, {1332, kWord, 0x0},
    {1336, kWord, 0x8}, {1328, kWord, 0xFFFFFFFF}, {1344, kImage, 0x82000A24}, {1348, kImage, 0x82000A24},
    {1352, kImage, 0x82000A24}, {1356, kImage, 0x82000A20}, {1360, kImage, 0x82000A24}, {1376, kImage, 0x82000A20},
    {1380, kImage, 0x82000A20}, {1384, kImage, 0x82000A20}, {1388, kImage, 0x82000A20}, {1336, kWord, 0x1},
    {1332, kWord, 0x1}, {1716, kWord, 0x0}, {1720, kWord, 0x8}, {1712, kWord, 0xFFFFFFFF},
    {1728, kImage, 0x82000A24}, {1732, kImage, 0x82000A24}, {1736, kImage, 0x82000A24}, {1740, kImage, 0x82000A20},
    {1744, kImage, 0x82000A24}, {1760, kImage, 0x82000A20}, {1764, kImage, 0x82000A20}, {1768, kImage, 0x82000A20},
    {1772, kImage, 0x82000A20}, {1720, kWord, 0x3}, {2100, kWord, 0x0}, {2104, kWord, 0x8},
    {2096, kWord, 0xFFFFFFFF}, {2112, kImage, 0x82000A24}, {2116, kImage, 0x82000A24}, {2120, kImage, 0x82000A24},
    {2124, kImage, 0x82000A20}, {2128, kImage, 0x82000A24}, {2144, kImage, 0x82000A20}, {2148, kImage, 0x82000A20},
    {2152, kImage, 0x82000A20}, {2156, kImage, 0x82000A20}, {2104, kWord, 0x4}, {2484, kWord, 0x0},
    {2488, kWord, 0x8}, {2480, kWord, 0xFFFFFFFF}, {2496, kImage, 0x82000A24}, {2500, kImage, 0x82000A24},
    {2504, kImage, 0x82000A24}, {2508, kImage, 0x82000A20}, {2512, kImage, 0x82000A24}, {2528, kImage, 0x82000A20},
    {2532, kImage, 0x82000A20}, {2536, kImage, 0x82000A20}, {2540, kImage, 0x82000A20}, {2488, kWord, 0x7},
    {2868, kWord, 0x0}, {2872, kWord, 0x8}, {2864, kWord, 0xFFFFFFFF}, {2880, kImage, 0x82000A24},
    {2884, kImage, 0x82000A24}, {2888, kImage, 0x82000A24}, {2892, kImage, 0x82000A20}, {2896, kImage, 0x82000A24},
    {2912, kImage, 0x82000A20}, {2916, kImage, 0x82000A20}, {2920, kImage, 0x82000A20}, {2924, kImage, 0x82000A20},
    {2872, kWord, 0x6}, {3252, kWord, 0x0}, {3256, kWord, 0x8}, {3248, kWord, 0xFFFFFFFF},
    {3264, kImage, 0x82000A24}, {3268, kImage, 0x82000A24}, {3272, kImage, 0x82000A24}, {3276, kImage, 0x82000A20},
    {3280, kImage, 0x82000A24}, {3296, kImage, 0x82000A20}, {3300, kImage, 0x82000A20}, {3304, kImage, 0x82000A20},
    {3308, kImage, 0x82000A20}, {3256, kWord, 0x5}, {3252, kWord, 0x1}, {628, kWord, 0x0},
    {632, kWord, 0x8}, {624, kWord, 0xFFFFFFFF}, {640, kImage, 0x82000A24}, {644, kImage, 0x82000A24},
    {648, kImage, 0x82000A24}, {652, kImage, 0x82000A20}, {656, kImage, 0x82000A24}, {672, kImage, 0x82000A20},
    {676, kImage, 0x82000A20}, {680, kImage, 0x82000A20}, {684, kImage, 0x82000A20}, {632, kWord, 0x8},
    {1012, kWord, 0x0}, {1016, kWord, 0x8}, {1008, kWord, 0xFFFFFFFF}, {1024, kImage, 0x82000A24},
    {1028, kImage, 0x82000A24}, {1032, kImage, 0x82000A24}, {1036, kImage, 0x82000A20}, {1040, kImage, 0x82000A24},
    {1056, kImage, 0x82000A20}, {1060, kImage, 0x82000A20}, {1064, kImage, 0x82000A20}, {1068, kImage, 0x82000A20},
    {1016, kWord, 0x2}, {1396, kWord, 0x0}, {1400, kWord, 0x8}, {1392, kWord, 0xFFFFFFFF},
    {1408, kImage, 0x82000A24}, {1412, kImage, 0x82000A24}, {1416, kImage, 0x82000A24}, {1420, kImage, 0x82000A20},
    {1424, kImage, 0x82000A24}, {1440, kImage, 0x82000A20}, {1444, kImage, 0x82000A20}, {1448, kImage, 0x82000A20},
    {1452, kImage, 0x82000A20}, {1400, kWord, 0x1}, {1396, kWord, 0x1}, {1780, kWord, 0x0},
    {1784, kWord, 0x8}, {1776, kWord, 0xFFFFFFFF}, {1792, kImage, 0x82000A24}, {1796, kImage, 0x82000A24},
    {1800, kImage, 0x82000A24}, {1804, kImage, 0x82000A20}, {1808, kImage, 0x82000A24}, {1824, kImage, 0x82000A20},
    {1828, kImage, 0x82000A20}, {1832, kImage, 0x82000A20}, {1836, kImage, 0x82000A20}, {1784, kWord, 0x3},
    {2164, kWord, 0x0}, {2168, kWord, 0x8}, {2160, kWord, 0xFFFFFFFF}, {2176, kImage, 0x82000A24},
    {2180, kImage, 0x82000A24}, {2184, kImage, 0x82000A24}, {2188, kImage, 0x82000A20}, {2192, kImage, 0x82000A24},
    {2208, kImage, 0x82000A20}, {2212, kImage, 0x82000A20}, {2216, kImage, 0x82000A20}, {2220, kImage, 0x82000A20},
    {2168, kWord, 0x4}, {2548, kWord, 0x0}, {2552, kWord, 0x8}, {2544, kWord, 0xFFFFFFFF},
    {2560, kImage, 0x82000A24}, {2564, kImage, 0x82000A24}, {2568, kImage, 0x82000A24}, {2572, kImage, 0x82000A20},
    {2576, kImage, 0x82000A24}, {2592, kImage, 0x82000A20}, {2596, kImage, 0x82000A20}, {2600, kImage, 0x82000A20},
    {2604, kImage, 0x82000A20}, {2552, kWord, 0x7}, {2932, kWord, 0x0}, {2936, kWord, 0x8},
    {2928, kWord, 0xFFFFFFFF}, {2944, kImage, 0x82000A24}, {2948, kImage, 0x82000A24}, {2952, kImage, 0x82000A24},
    {2956, kImage, 0x82000A20}, {2960, kImage, 0x82000A24}, {2976, kImage, 0x82000A20}, {2980, kImage, 0x82000A20},
    {2984, kImage, 0x82000A20}, {2988, kImage, 0x82000A20}, {2936, kWord, 0x6}, {3316, kWord, 0x0},
    {3320, kWord, 0x8}, {3312, kWord, 0xFFFFFFFF}, {3328, kImage, 0x82000A24}, {3332, kImage, 0x82000A24},
    {3336, kImage, 0x82000A24}, {3340, kImage, 0x82000A20}, {3344, kImage, 0x82000A24}, {3360, kImage, 0x82000A20},
    {3364, kImage, 0x82000A20}, {3368, kImage, 0x82000A20}, {3372, kImage, 0x82000A20}, {3320, kWord, 0x5},
    {3316, kWord, 0x1}, {692, kWord, 0x0}, {696, kWord, 0x8}, {688, kWord, 0xFFFFFFFF},
    {704, kImage, 0x82000A24}, {708, kImage, 0x82000A24}, {712, kImage, 0x82000A24}, {716, kImage, 0x82000A20},
    {720, kImage, 0x82000A24}, {736, kImage, 0x82000A20}, {740, kImage, 0x82000A20}, {744, kImage, 0x82000A20},
    {748, kImage, 0x82000A20}, {696, kWord, 0x8}, {1076, kWord, 0x0}, {1080, kWord, 0x8},
    {1072, kWord, 0xFFFFFFFF}, {1088, kImage, 0x82000A24}, {1092, kImage, 0x82000A24}, {1096, kImage, 0x82000A24},
    {1100, kImage, 0x82000A20}, {1104, kImage, 0x82000A24}, {1120, kImage, 0x82000A20}, {1124, kImage, 0x82000A20},
    {1128, kImage, 0x82000A20}, {1132, kImage, 0x82000A20}, {1080, kWord, 0x2}, {1460, kWord, 0x0},
    {1464, kWord, 0x8}, {1456, kWord, 0xFFFFFFFF}, {1472, kImage, 0x82000A24}, {1476, kImage, 0x82000A24},
    {1480, kImage, 0x82000A24}, {1484, kImage, 0x82000A20}, {1488, kImage, 0x82000A24}, {1504, kImage, 0x82000A20},
    {1508, kImage, 0x82000A20}, {1512, kImage, 0x82000A20}, {1516, kImage, 0x82000A20}, {1464, kWord, 0x1},
    {1460, kWord, 0x1}, {1844, kWord, 0x0}, {1848, kWord, 0x8}, {1840, kWord, 0xFFFFFFFF},
    {1856, kImage, 0x82000A24}, {1860, kImage, 0x82000A24}, {1864, kImage, 0x82000A24}, {1868, kImage, 0x82000A20},
    {1872, kImage, 0x82000A24}, {1888, kImage, 0x82000A20}, {1892, kImage, 0x82000A20}, {1896, kImage, 0x82000A20},
    {1900, kImage, 0x82000A20}, {1848, kWord, 0x3}, {2228, kWord, 0x0}, {2232, kWord, 0x8},
    {2224, kWord, 0xFFFFFFFF}, {2240, kImage, 0x82000A24}, {2244, kImage, 0x82000A24}, {2248, kImage, 0x82000A24},
    {2252, kImage, 0x82000A20}, {2256, kImage, 0x82000A24}, {2272, kImage, 0x82000A20}, {2276, kImage, 0x82000A20},
    {2280, kImage, 0x82000A20}, {2284, kImage, 0x82000A20}, {2232, kWord, 0x4}, {2612, kWord, 0x0},
    {2616, kWord, 0x8}, {2608, kWord, 0xFFFFFFFF}, {2624, kImage, 0x82000A24}, {2628, kImage, 0x82000A24},
    {2632, kImage, 0x82000A24}, {2636, kImage, 0x82000A20}, {2640, kImage, 0x82000A24}, {2656, kImage, 0x82000A20},
    {2660, kImage, 0x82000A20}, {2664, kImage, 0x82000A20}, {2668, kImage, 0x82000A20}, {2616, kWord, 0x7},
    {2996, kWord, 0x0}, {3000, kWord, 0x8}, {2992, kWord, 0xFFFFFFFF}, {3008, kImage, 0x82000A24},
    {3012, kImage, 0x82000A24}, {3016, kImage, 0x82000A24}, {3020, kImage, 0x82000A20}, {3024, kImage, 0x82000A24},
    {3040, kImage, 0x82000A20}, {3044, kImage, 0x82000A20}, {3048, kImage, 0x82000A20}, {3052, kImage, 0x82000A20},
    {3000, kWord, 0x6}, {3380, kWord, 0x0}, {3384, kWord, 0x8}, {3376, kWord, 0xFFFFFFFF},
    {3392, kImage, 0x82000A24}, {3396, kImage, 0x82000A24}, {3400, kImage, 0x82000A24}, {3404, kImage, 0x82000A20},
    {3408, kImage, 0x82000A24}, {3424, kImage, 0x82000A20}, {3428, kImage, 0x82000A20}, {3432, kImage, 0x82000A20},
    {3436, kImage, 0x82000A20}, {3384, kWord, 0x5}, {3380, kWord, 0x1}, {756, kWord, 0x0},
    {760, kWord, 0x8}, {752, kWord, 0xFFFFFFFF}, {768, kImage, 0x82000A24}, {772, kImage, 0x82000A24},
    {776, kImage, 0x82000A24}, {780, kImage, 0x82000A20}, {784, kImage, 0x82000A24}, {800, kImage, 0x82000A20},
    {804, kImage, 0x82000A20}, {808, kImage, 0x82000A20}, {812, kImage, 0x82000A20}, {760, kWord, 0x8},
    {1140, kWord, 0x0}, {1144, kWord, 0x8}, {1136, kWord, 0xFFFFFFFF}, {1152, kImage, 0x82000A24},
    {1156, kImage, 0x82000A24}, {1160, kImage, 0x82000A24}, {1164, kImage, 0x82000A20}, {1168, kImage, 0x82000A24},
    {1184, kImage, 0x82000A20}, {1188, kImage, 0x82000A20}, {1192, kImage, 0x82000A20}, {1196, kImage, 0x82000A20},
    {1144, kWord, 0x2}, {1524, kWord, 0x0}, {1528, kWord, 0x8}, {1520, kWord, 0xFFFFFFFF},
    {1536, kImage, 0x82000A24}, {1540, kImage, 0x82000A24}, {1544, kImage, 0x82000A24}, {1548, kImage, 0x82000A20},
    {1552, kImage, 0x82000A24}, {1568, kImage, 0x82000A20}, {1572, kImage, 0x82000A20}, {1576, kImage, 0x82000A20},
    {1580, kImage, 0x82000A20}, {1528, kWord, 0x1}, {1524, kWord, 0x1}, {1908, kWord, 0x0},
    {1912, kWord, 0x8}, {1904, kWord, 0xFFFFFFFF}, {1920, kImage, 0x82000A24}, {1924, kImage, 0x82000A24},
    {1928, kImage, 0x82000A24}, {1932, kImage, 0x82000A20}, {1936, kImage, 0x82000A24}, {1952, kImage, 0x82000A20},
    {1956, kImage, 0x82000A20}, {1960, kImage, 0x82000A20}, {1964, kImage, 0x82000A20}, {1912, kWord, 0x3},
    {2292, kWord, 0x0}, {2296, kWord, 0x8}, {2288, kWord, 0xFFFFFFFF}, {2304, kImage, 0x82000A24},
    {2308, kImage, 0x82000A24}, {2312, kImage, 0x82000A24}, {2316, kImage, 0x82000A20}, {2320, kImage, 0x82000A24},
    {2336, kImage, 0x82000A20}, {2340, kImage, 0x82000A20}, {2344, kImage, 0x82000A20}, {2348, kImage, 0x82000A20},
    {2296, kWord, 0x4}, {2676, kWord, 0x0}, {2680, kWord, 0x8}, {2672, kWord, 0xFFFFFFFF},
    {2688, kImage, 0x82000A24}, {2692, kImage, 0x82000A24}, {2696, kImage, 0x82000A24}, {2700, kImage, 0x82000A20},
    {2704, kImage, 0x82000A24}, {2720, kImage, 0x82000A20}, {2724, kImage, 0x82000A20}, {2728, kImage, 0x82000A20},
    {2732, kImage, 0x82000A20}, {2680, kWord, 0x7}, {3060, kWord, 0x0}, {3064, kWord, 0x8},
    {3056, kWord, 0xFFFFFFFF}, {3072, kImage, 0x82000A24}, {3076, kImage, 0x82000A24}, {3080, kImage, 0x82000A24},
    {3084, kImage, 0x82000A20}, {3088, kImage, 0x82000A24}, {3104, kImage, 0x82000A20}, {3108, kImage, 0x82000A20},
    {3112, kImage, 0x82000A20}, {3116, kImage, 0x82000A20}, {3064, kWord, 0x6}, {3444, kWord, 0x0},
    {3448, kWord, 0x8}, {3440, kWord, 0xFFFFFFFF}, {3456, kImage, 0x82000A24}, {3460, kImage, 0x82000A24},
    {3464, kImage, 0x82000A24}, {3468, kImage, 0x82000A20}, {3472, kImage, 0x82000A24}, {3488, kImage, 0x82000A20},
    {3492, kImage, 0x82000A20}, {3496, kImage, 0x82000A20}, {3500, kImage, 0x82000A20}, {3448, kWord, 0x5},
    {3444, kWord, 0x1}, {820, kWord, 0x0}, {824, kWord, 0x8}, {816, kWord, 0xFFFFFFFF},
    {832, kImage, 0x82000A24}, {836, kImage, 0x82000A24}, {840, kImage, 0x82000A24}, {844, kImage, 0x82000A20},
    {848, kImage, 0x82000A24}, {864, kImage, 0x82000A20}, {868, kImage, 0x82000A20}, {872, kImage, 0x82000A20},
    {876, kImage, 0x82000A20}, {824, kWord, 0x8}, {1204, kWord, 0x0}, {1208, kWord, 0x8},
    {1200, kWord, 0xFFFFFFFF}, {1216, kImage, 0x82000A24}, {1220, kImage, 0x82000A24}, {1224, kImage, 0x82000A24},
    {1228, kImage, 0x82000A20}, {1232, kImage, 0x82000A24}, {1248, kImage, 0x82000A20}, {1252, kImage, 0x82000A20},
    {1256, kImage, 0x82000A20}, {1260, kImage, 0x82000A20}, {1208, kWord, 0x2}, {1588, kWord, 0x0},
    {1592, kWord, 0x8}, {1584, kWord, 0xFFFFFFFF}, {1600, kImage, 0x82000A24}, {1604, kImage, 0x82000A24},
    {1608, kImage, 0x82000A24}, {1612, kImage, 0x82000A20}, {1616, kImage, 0x82000A24}, {1632, kImage, 0x82000A20},
    {1636, kImage, 0x82000A20}, {1640, kImage, 0x82000A20}, {1644, kImage, 0x82000A20}, {1592, kWord, 0x1},
    {1588, kWord, 0x1}, {1972, kWord, 0x0}, {1976, kWord, 0x8}, {1968, kWord, 0xFFFFFFFF},
    {1984, kImage, 0x82000A24}, {1988, kImage, 0x82000A24}, {1992, kImage, 0x82000A24}, {1996, kImage, 0x82000A20},
    {2000, kImage, 0x82000A24}, {2016, kImage, 0x82000A20}, {2020, kImage, 0x82000A20}, {2024, kImage, 0x82000A20},
    {2028, kImage, 0x82000A20}, {1976, kWord, 0x3}, {2356, kWord, 0x0}, {2360, kWord, 0x8},
    {2352, kWord, 0xFFFFFFFF}, {2368, kImage, 0x82000A24}, {2372, kImage, 0x82000A24}, {2376, kImage, 0x82000A24},
    {2380, kImage, 0x82000A20}, {2384, kImage, 0x82000A24}, {2400, kImage, 0x82000A20}, {2404, kImage, 0x82000A20},
    {2408, kImage, 0x82000A20}, {2412, kImage, 0x82000A20}, {2360, kWord, 0x4}, {2740, kWord, 0x0},
    {2744, kWord, 0x8}, {2736, kWord, 0xFFFFFFFF}, {2752, kImage, 0x82000A24}, {2756, kImage, 0x82000A24},
    {2760, kImage, 0x82000A24}, {2764, kImage, 0x82000A20}, {2768, kImage, 0x82000A24}, {2784, kImage, 0x82000A20},
    {2788, kImage, 0x82000A20}, {2792, kImage, 0x82000A20}, {2796, kImage, 0x82000A20}, {2744, kWord, 0x7},
    {3124, kWord, 0x0}, {3128, kWord, 0x8}, {3120, kWord, 0xFFFFFFFF}, {3136, kImage, 0x82000A24},
    {3140, kImage, 0x82000A24}, {3144, kImage, 0x82000A24}, {3148, kImage, 0x82000A20}, {3152, kImage, 0x82000A24},
    {3168, kImage, 0x82000A20}, {3172, kImage, 0x82000A20}, {3176, kImage, 0x82000A20}, {3180, kImage, 0x82000A20},
    {3128, kWord, 0x6}, {3508, kWord, 0x0}, {3512, kWord, 0x8}, {3504, kWord, 0xFFFFFFFF},
    {3520, kImage, 0x82000A24}, {3524, kImage, 0x82000A24}, {3528, kImage, 0x82000A24}, {3532, kImage, 0x82000A20},
    {3536, kImage, 0x82000A24}, {3552, kImage, 0x82000A20}, {3556, kImage, 0x82000A20}, {3560, kImage, 0x82000A20},
    {3564, kImage, 0x82000A20}, {3512, kWord, 0x5}, {3508, kWord, 0x1}, {884, kWord, 0x0},
    {888, kWord, 0x8}, {880, kWord, 0xFFFFFFFF}, {896, kImage, 0x82000A24}, {900, kImage, 0x82000A24},
    {904, kImage, 0x82000A24}, {908, kImage, 0x82000A20}, {912, kImage, 0x82000A24}, {928, kImage, 0x82000A20},
    {932, kImage, 0x82000A20}, {936, kImage, 0x82000A20}, {940, kImage, 0x82000A20}, {888, kWord, 0x8},
    {1268, kWord, 0x0}, {1272, kWord, 0x8}, {1264, kWord, 0xFFFFFFFF}, {1280, kImage, 0x82000A24},
    {1284, kImage, 0x82000A24}, {1288, kImage, 0x82000A24}, {1292, kImage, 0x82000A20}, {1296, kImage, 0x82000A24},
    {1312, kImage, 0x82000A20}, {1316, kImage, 0x82000A20}, {1320, kImage, 0x82000A20}, {1324, kImage, 0x82000A20},
    {1272, kWord, 0x2}, {1652, kWord, 0x0}, {1656, kWord, 0x8}, {1648, kWord, 0xFFFFFFFF},
    {1664, kImage, 0x82000A24}, {1668, kImage, 0x82000A24}, {1672, kImage, 0x82000A24}, {1676, kImage, 0x82000A20},
    {1680, kImage, 0x82000A24}, {1696, kImage, 0x82000A20}, {1700, kImage, 0x82000A20}, {1704, kImage, 0x82000A20},
    {1708, kImage, 0x82000A20}, {1656, kWord, 0x1}, {1652, kWord, 0x1}, {2036, kWord, 0x0},
    {2040, kWord, 0x8}, {2032, kWord, 0xFFFFFFFF}, {2048, kImage, 0x82000A24}, {2052, kImage, 0x82000A24},
    {2056, kImage, 0x82000A24}, {2060, kImage, 0x82000A20}, {2064, kImage, 0x82000A24}, {2080, kImage, 0x82000A20},
    {2084, kImage, 0x82000A20}, {2088, kImage, 0x82000A20}, {2092, kImage, 0x82000A20}, {2040, kWord, 0x3},
    {2420, kWord, 0x0}, {2424, kWord, 0x8}, {2416, kWord, 0xFFFFFFFF}, {2432, kImage, 0x82000A24},
    {2436, kImage, 0x82000A24}, {2440, kImage, 0x82000A24}, {2444, kImage, 0x82000A20}, {2448, kImage, 0x82000A24},
    {2464, kImage, 0x82000A20}, {2468, kImage, 0x82000A20}, {2472, kImage, 0x82000A20}, {2476, kImage, 0x82000A20},
    {2424, kWord, 0x4}, {2804, kWord, 0x0}, {2808, kWord, 0x8}, {2800, kWord, 0xFFFFFFFF},
    {2816, kImage, 0x82000A24}, {2820, kImage, 0x82000A24}, {2824, kImage, 0x82000A24}, {2828, kImage, 0x82000A20},
    {2832, kImage, 0x82000A24}, {2848, kImage, 0x82000A20}, {2852, kImage, 0x82000A20}, {2856, kImage, 0x82000A20},
    {2860, kImage, 0x82000A20}, {2808, kWord, 0x7}, {3188, kWord, 0x0}, {3192, kWord, 0x8},
    {3184, kWord, 0xFFFFFFFF}, {3200, kImage, 0x82000A24}, {3204, kImage, 0x82000A24}, {3208, kImage, 0x82000A24},
    {3212, kImage, 0x82000A20}, {3216, kImage, 0x82000A24}, {3232, kImage, 0x82000A20}, {3236, kImage, 0x82000A20},
    {3240, kImage, 0x82000A20}, {3244, kImage, 0x82000A20}, {3192, kWord, 0x6}, {3572, kWord, 0x0},
    {3576, kWord, 0x8}, {3568, kWord, 0xFFFFFFFF}, {3584, kImage, 0x82000A24}, {3588, kImage, 0x82000A24},
    {3592, kImage, 0x82000A24}, {3596, kImage, 0x82000A20}, {3600, kImage, 0x82000A24}, {3616, kImage, 0x82000A20},
    {3620, kImage, 0x82000A20}, {3624, kImage, 0x82000A20}, {3628, kImage, 0x82000A20}, {3576, kWord, 0x5},
    {3572, kWord, 0x1}, {3632, kWord, 0x0}, {3636, kWord, 0x0}, {3640, kWord, 0x0},
    {3644, kWord, 0x1}, {3648, kWord, 0x2}, {3652, kWord, 0x3}, {3656, kWord, 0x4},
    {3660, kWord, 0x5}, {3664, kWord, 0x0}, {3668, kWord, 0x6}, {3672, kWord, 0x7},
    {3676, kWord, 0x0}, {3680, kWord, 0x0}, {3684, kWord, 0x0}, {3688, kWord, 0x0},
};
constexpr Fill kPartsRecordReset0[] = {
    {0, kWord, 0x0}, {4, kWord, 0x0}, {8, kWord, 0x0}, {12, kWord, 0x0},
    {16, kWord, 0x0}, {20, kWord, 0x0}, {24, kWord, 0x0}, {28, kWord, 0x0},
    {32, kWord, 0x0}, {36, kWord, 0x0}, {40, kWord, 0x0}, {44, kWord, 0x0},
    {48, kWord, 0x0}, {52, kWord, 0x3}, {56, kWord, 0x0},
};
constexpr Fill kInteriorRecordReset0[] = {
    {0, kWord, 0x0}, {4, kWord, 0x0}, {92, kWord, 0x0}, {96, kWord, 0x0},
    {100, kWord, 0x0}, {104, kWord, 0x0}, {108, kWord, 0x0}, {112, kWord, 0x3},
    {116, kWord, 0x0}, {8, kWord, 0x0}, {12, kWord, 0x0}, {16, kWord, 0x0},
    {20, kWord, 0x0}, {24, kWord, 0x0}, {28, kWord, 0x1}, {32, kWord, 0x0},
    {36, kWord, 0x0}, {40, kWord, 0x2}, {44, kWord, 0x0}, {48, kWord, 0x0},
    {52, kWord, 0x3}, {56, kWord, 0x0}, {60, kWord, 0x0}, {64, kWord, 0x4},
    {68, kWord, 0x0}, {72, kWord, 0x1}, {76, kWord, 0x0}, {80, kWord, 0x0},
    {84, kWord, 0x1}, {88, kWord, 0x1},
};
constexpr Fill kWheelsRecordReset0[] = {
    {0, kWord, 0x1}, {20, kImage, 0x82002520}, {16, kWord, 0xFFFFFFFF}, {36, kImage, 0x82002520},
    {32, kWord, 0xFFFFFFFF}, {24, kImage, 0x82001BDC}, {40, kImage, 0x82001BDC}, {4, kWord, 0x0},
    {8, kWord, 0x0}, {12, kWord, 0x0}, {28, kWord, 0x0}, {48, kImage, 0x82001BD8},
    {52, kImage, 0x82001BD8}, {44, kWord, 0xFFFFFFFF}, {60, kImage, 0x82001BD8}, {56, kWord, 0xFFFFFFFF},
    {64, kImage, 0x82001BD8}, {68, kWord, 0x0}, {128, kImage, 0x82000A24}, {124, kWord, 0xFFFFFFFF},
    {132, kImage, 0x82000A24}, {72, kWord, 0x0}, {136, kImage, 0x82000A24}, {76, kWord, 0x0},
    {140, kImage, 0x82000A24}, {80, kWord, 0x0}, {144, kImage, 0x82000A24}, {84, kWord, 0x0},
    {148, kImage, 0x82000A24}, {88, kWord, 0x0}, {152, kImage, 0x82000A24}, {100, kImage, 0x82000A24},
    {92, kWord, 0x0}, {116, kWord, 0x0}, {108, kWord, 0xB}, {104, kImage, 0x82000A24},
    {96, kWord, 0x0}, {120, kWord, 0x0}, {112, kWord, 0xB},
};
constexpr Fill kPool4RecordReset0[] = {
    {0, kWord, 0x0}, {8, kImage, 0x82000A20}, {12, kImage, 0x82000A20}, {4, kWord, 0x0},
    {16, kImage, 0x82000A20}, {20, kWord, 0x0}, {24, kImage, 0x82000A20}, {28, kImage, 0x82000A20},
    {32, kImage, 0x82000A20}, {36, kWord, 0x0}, {40, kImage, 0x82000A20}, {44, kImage, 0x82000A20},
    {48, kImage, 0x82000A20},
};
constexpr Fill kCarRecordSetW0[] = {
    {28, kImage, 0x82000A20}, {44, kImage, 0x82000A20}, {60, kImage, 0x82000A20}, {76, kImage, 0x82000A20},
    {92, kImage, 0x82000A20}, {108, kImage, 0x82000A20}, {124, kImage, 0x82000A20}, {140, kImage, 0x82000A20},
    {156, kImage, 0x82000A20}, {204, kImage, 0x82000A20}, {236, kImage, 0x82000A20}, {268, kImage, 0x82000A20},
    {300, kImage, 0x82000A20}, {332, kImage, 0x82000A20}, {364, kImage, 0x82000A20}, {396, kImage, 0x82000A20},
    {428, kImage, 0x82000A20}, {460, kImage, 0x82000A20}, {492, kImage, 0x82000A20}, {524, kImage, 0x82000A20},
    {556, kImage, 0x82000A20}, {588, kImage, 0x82000A20}, {620, kImage, 0x82000A20}, {652, kImage, 0x82000A20},
    {684, kImage, 0x82000A20}, {716, kImage, 0x82000A20}, {748, kImage, 0x82000A20}, {780, kImage, 0x82000A20},
    {812, kImage, 0x82000A20}, {844, kImage, 0x82000A20}, {876, kImage, 0x82000A20}, {908, kImage, 0x82000A20},
    {940, kImage, 0x82000A20}, {972, kImage, 0x82000A20}, {1004, kImage, 0x82000A20}, {1036, kImage, 0x82000A20},
    {1068, kImage, 0x82000A20}, {1100, kImage, 0x82000A20}, {1132, kImage, 0x82000A20}, {1164, kImage, 0x82000A20},
    {1196, kImage, 0x82000A20}, {1228, kImage, 0x82000A20}, {1260, kImage, 0x82000A20}, {1292, kImage, 0x82000A20},
    {1324, kImage, 0x82000A20}, {1356, kImage, 0x82000A20}, {1388, kImage, 0x82000A20}, {1420, kImage, 0x82000A20},
    {1452, kImage, 0x82000A20}, {1484, kImage, 0x82000A20}, {1516, kImage, 0x82000A20}, {1548, kImage, 0x82000A20},
    {1580, kImage, 0x82000A20}, {1612, kImage, 0x82000A20}, {1644, kImage, 0x82000A20}, {1676, kImage, 0x82000A20},
    {1708, kImage, 0x82000A20}, {1740, kImage, 0x82000A20}, {1772, kImage, 0x82000A20}, {1804, kImage, 0x82000A20},
    {1836, kImage, 0x82000A20}, {1868, kImage, 0x82000A20}, {1900, kImage, 0x82000A20}, {1932, kImage, 0x82000A20},
    {1964, kImage, 0x82000A20}, {1996, kImage, 0x82000A20}, {2028, kImage, 0x82000A20}, {2060, kImage, 0x82000A20},
    {2092, kImage, 0x82000A20}, {2124, kImage, 0x82000A20}, {2156, kImage, 0x82000A20}, {2188, kImage, 0x82000A20},
    {2220, kImage, 0x82000A20}, {2252, kImage, 0x82000A20}, {2284, kImage, 0x82000A20}, {2316, kImage, 0x82000A20},
    {2348, kImage, 0x82000A20}, {2380, kImage, 0x82000A20}, {2412, kImage, 0x82000A20}, {2444, kImage, 0x82000A20},
    {2476, kImage, 0x82000A20}, {2508, kImage, 0x82000A20}, {2540, kImage, 0x82000A20}, {2572, kImage, 0x82000A20},
    {2604, kImage, 0x82000A20}, {2636, kImage, 0x82000A20}, {2668, kImage, 0x82000A20}, {2700, kImage, 0x82000A20},
    {2732, kImage, 0x82000A20}, {2764, kImage, 0x82000A20}, {2796, kImage, 0x82000A20}, {2828, kImage, 0x82000A20},
    {2860, kImage, 0x82000A20}, {2892, kImage, 0x82000A20}, {2924, kImage, 0x82000A20}, {2956, kImage, 0x82000A20},
    {2988, kImage, 0x82000A20}, {3020, kImage, 0x82000A20}, {3052, kImage, 0x82000A20}, {3084, kImage, 0x82000A20},
    {3116, kImage, 0x82000A20}, {3148, kImage, 0x82000A20}, {3180, kImage, 0x82000A20}, {3212, kImage, 0x82000A20},
    {3244, kImage, 0x82000A20}, {3276, kImage, 0x82000A20}, {3308, kImage, 0x82000A20}, {3340, kImage, 0x82000A20},
    {3372, kImage, 0x82000A20}, {3404, kImage, 0x82000A20}, {3436, kImage, 0x82000A20}, {3468, kImage, 0x82000A20},
    {3500, kImage, 0x82000A20}, {3532, kImage, 0x82000A20}, {3564, kImage, 0x82000A20}, {3596, kImage, 0x82000A20},
    {3628, kImage, 0x82000A20},
};

// CarRecord::Copy (sub_820D7B98): the words copied, in the original's
// order, before and after its memcpy of +3632 (44 bytes).
constexpr Run kCarRecordBefore[] = {
    {0, 4}, {16, 152}, {176, 12}, {192, 20}, {224, 28}, {256, 20},
    {288, 28}, {320, 20}, {352, 28}, {384, 20}, {416, 28}, {448, 20},
    {480, 28}, {512, 20}, {544, 28}, {576, 20}, {608, 28}, {640, 20},
    {672, 28}, {704, 20}, {736, 28}, {768, 20}, {800, 28}, {832, 20},
    {864, 28}, {896, 20}, {928, 28}, {960, 20}, {992, 28}, {1024, 20},
    {1056, 28}, {1088, 20}, {1120, 28}, {1152, 20}, {1184, 28}, {1216, 20},
    {1248, 28}, {1280, 20}, {1312, 28}, {1344, 20}, {1376, 28}, {1408, 20},
    {1440, 28}, {1472, 20}, {1504, 28}, {1536, 20}, {1568, 28}, {1600, 20},
    {1632, 28}, {1664, 20}, {1696, 28}, {1728, 20}, {1760, 28}, {1792, 20},
    {1824, 28}, {1856, 20}, {1888, 28}, {1920, 20}, {1952, 28}, {1984, 20},
    {2016, 28}, {2048, 20}, {2080, 28}, {2112, 20}, {2144, 28}, {2176, 20},
    {2208, 28}, {2240, 20}, {2272, 28}, {2304, 20}, {2336, 28}, {2368, 20},
    {2400, 28}, {2432, 20}, {2464, 28}, {2496, 20}, {2528, 28}, {2560, 20},
    {2592, 28}, {2624, 20}, {2656, 28}, {2688, 20}, {2720, 28}, {2752, 20},
    {2784, 28}, {2816, 20}, {2848, 28}, {2880, 20}, {2912, 28}, {2944, 20},
    {2976, 28}, {3008, 20}, {3040, 28}, {3072, 20}, {3104, 28}, {3136, 20},
    {3168, 28}, {3200, 20}, {3232, 28}, {3264, 20}, {3296, 28}, {3328, 20},
    {3360, 28}, {3392, 20}, {3424, 28}, {3456, 20}, {3488, 28}, {3520, 20},
    {3552, 28}, {3584, 20}, {3616, 16},
};
constexpr Run kCarRecordAfter[] = {
    {3676, 16},
};

constexpr uint32_t kCarRecordCopied = 3632;
constexpr uint32_t kCarRecordCopiedBytes = 44;

void Memcpy(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t from, uint32_t bytes) {
  guest_memory::Memcpy(ctx, base, to, from, bytes);
}

// Record12::Reset (sub_820D3780): +4 = the image's value, +0 = -1, +8 =
// the value.
void Record12Reset(uint8_t* base, uint32_t record) {
  const uint32_t value = LoadU32(base, kRecord12Value);
  StoreU32(base, record + 4, value);
  StoreU32(base, record + 0, 0xFFFFFFFFu);
  StoreU32(base, record + 8, value);
}

// CarRecord::SetW (sub_820D4250): 1.0 into the car record's rows and
// layers (the table).
void CarRecordSetW(uint8_t* base, uint32_t record) {
  guest_memory::Apply(base, record, kCarRecordSetW0);
}

// The records cleared (memset) and then filled.
void PartsRecordReset(uint8_t* base, uint32_t record) {
  guest_memory::Clear(base, record, 60);
  guest_memory::Apply(base, record, kPartsRecordReset0);
}

void InteriorRecordReset(uint8_t* base, uint32_t record) {
  guest_memory::Clear(base, record, 120);
  guest_memory::Apply(base, record, kInteriorRecordReset0);
}

void Pool4RecordReset(uint8_t* base, uint32_t record) {
  guest_memory::Clear(base, record, 52);
  guest_memory::Apply(base, record, kPool4RecordReset0);
}

// CockpitRecord::Reset (sub_820D3100): +0 to +16 = 0, then the interior
// record at +24.
void CockpitRecordReset(uint8_t* base, uint32_t record) {
  for (uint32_t i = 0; i < 20; i += 4) {
    StoreU32(base, record + i, 0);
  }
  InteriorRecordReset(base, record + 24);
}

// SceneModelRecord::Reset (sub_820D94D8).
void SceneModelRecordReset(uint8_t* base, uint32_t record) {
  StoreU32(base, record + 0, 1);
  guest_memory::Apply(base, record + 16, kCarRecordReset0);
  PartsRecordReset(base, record + 3712);
  InteriorRecordReset(base, record + 3772);
  CockpitRecordReset(base, record + 3892);
  guest_memory::Apply(base, record + 4036, kWheelsRecordReset0);
  Pool4RecordReset(base, record + 4192);
}

// CarRecord::Copy (sub_820D7B98): `from` into `to`.
void CarRecordCopy(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t from) {
  for (const Run& run : kCarRecordBefore) {
    guest_memory::CopyRun(base, to, from, run);
  }
  Memcpy(ctx, base, to + kCarRecordCopied, from + kCarRecordCopied, kCarRecordCopiedBytes);
  for (const Run& run : kCarRecordAfter) {
    guest_memory::CopyRun(base, to, from, run);
  }
}

// SceneModelRecord::Copy (sub_820DAF58): +0, the car record, then the
// other records (memcpy).
void SceneModelRecordCopy(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t from) {
  StoreU32(base, to + 0, LoadU32(base, from + 0));
  CarRecordCopy(ctx, base, to + 16, from + 16);
  Memcpy(ctx, base, to + 3712, from + 3712, 60);
  Memcpy(ctx, base, to + 3772, from + 3772, 120);
  Memcpy(ctx, base, to + 3892, from + 3892, 144);
  Memcpy(ctx, base, to + 4036, from + 4036, 156);
  Memcpy(ctx, base, to + 4192, from + 4192, 52);
}

// SceneModel::Accessory (sub_820D9480): the interior's +140 (3: none)
// picks the set's entry [set + 4 + 4i]; the entry + 48, or 0.
uint32_t Accessory(uint8_t* base, uint32_t interior) {
  const uint32_t set = LoadU32(base, kAccessories);
  const int32_t index = int32_t(LoadU32(base, interior + 140));
  if (index == 3) {
    return 0;
  }
  const uint32_t entry = LoadU32(base, set + 4 * uint32_t(index + 1));
  return entry != 0 ? entry + 48 : 0;
}

// SceneModel::Update (sub_820D9E48): r3 a scene model. The deferred
// build: +5044 counts down while above 0, and on reaching 0 with +5040 set
// SceneModel::Build (sub_820D9538) runs and +5040 = 0. Then, when the
// scene's (+5116) +5000 is 0 and the interior is ready (+5072, its car
// model +5048, and +4296 when the scene's +4 is 0, else the cockpit
// +5076), the interior's accessory ([[+5068] + 36]): when there is one,
// its anchor is the identity with row 3 the car model's accessory node
// ([[+5048] + 24] + 7216) times the model's +4304 (Matrix::Multiply), then
// Accessory::SetAnchor (sub_820EF288) and Step (sub_820EF0C0, r4 = 0), in
// the original's 240-byte frame.
void Update(PPCContext& ctx, uint8_t* base, uint32_t model) {
  int32_t countdown = int32_t(LoadU32(base, model + 5044));
  if (countdown > 0) {
    countdown--;
    StoreU32(base, model + 5044, uint32_t(countdown));
    if (LoadU32(base, model + 5040) != 0 && countdown == 0) {
      ctx.r3.u64 = model;
      sub_820D9538(ctx, base);
      StoreU32(base, model + 5040, 0);
    }
  }
  const uint32_t scene = LoadU32(base, model + 5116);
  if (LoadU32(base, scene + 5000) != 0) {
    return;
  }
  if (LoadU32(base, model + 5072) == 0 || LoadU32(base, model + 5048) == 0) {
    return;
  }
  const uint32_t ready = LoadU32(base, scene + 4) == 0 ? LoadU32(base, model + 4296)
                                                       : LoadU32(base, model + 5076);
  if (ready == 0) {
    return;
  }
  const uint32_t accessory = Accessory(base, LoadU32(base, LoadU32(base, model + 5068) + 36));
  if (accessory == 0) {
    return;
  }
  guest::StackFrame frame(ctx, 240);
  const uint32_t matrix = frame.Local(144);
  for (uint32_t row = 0; row < 4; row++) {
    for (uint32_t column = 0; column < 4; column++) {
      StoreU32(base, matrix + 16 * row + 4 * column, row == column ? kOneBits : 0);
    }
  }
  const uint32_t node = LoadU32(base, LoadU32(base, model + 5048) + 24) + 7216;
  for (uint32_t i = 0; i < 16; i += 4) {
    StoreU32(base, matrix + 48 + i, LoadU32(base, node + i));
  }
  ctx.r3.u64 = matrix;
  ctx.r4.u64 = matrix;
  ctx.r5.u64 = model + 4304;
  sub_8209BE70(ctx, base);
  ctx.r3.u64 = accessory;
  ctx.r4.u64 = matrix;
  sub_820EF288(ctx, base);
  ctx.r3.u64 = accessory;
  ctx.r4.u64 = 0;
  sub_820EF0C0(ctx, base);
}

bool On() { return REXCVAR_GET(native_scene_models); }

}  // namespace

// The getters and links.
extern "C" REX_FUNC(sub_820F0CE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0CE0(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, kAccessories);
}

extern "C" REX_FUNC(sub_820D9480) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D9480(ctx, base);
    return;
  }
  ctx.r3.u64 = Accessory(base, ctx.r3.u32);
}

// sub_82173DA0: Archive::Load's callback for SceneModel::SetUp's two sound
// assets, r6 the context (+0 the slot): the slot's byte up by one, and
// when it's then 2 (the slot read again) its word = 1.
extern "C" REX_FUNC(sub_82173DA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82173DA0(ctx, base);
    return;
  }
  const uint32_t context = ctx.r6.u32;
  uint32_t slot = LoadU32(base, context);
  guest::StoreU8(base, kSoundLoadsDone + slot,
                 uint8_t(guest::LoadU8(base, kSoundLoadsDone + slot) + 1));
  slot = LoadU32(base, context);
  if (guest::LoadU8(base, kSoundLoadsDone + slot) == 2) {
    StoreU32(base, kSoundLoaded + 4 * slot, 1);
  }
}

// sub_820E6F60: the wheels' +16776 = r4.
extern "C" REX_FUNC(sub_820E6F60) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E6F60(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + 16776, ctx.r4.u32);
}

// sub_820DC710: +24 = the car model.
extern "C" REX_FUNC(sub_820DC710) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DC710(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + 24, ctx.r4.u32);
}

// SceneParts::Link: +24 = the car model, +28 = [model + 24] + 3760.
extern "C" REX_FUNC(sub_820DEF70) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DEF70(ctx, base);
    return;
  }
  const uint32_t parts = ctx.r3.u32, model = ctx.r4.u32;
  StoreU32(base, parts + 24, model);
  StoreU32(base, parts + 28, LoadU32(base, model + 24) + 3760);
}

// sub_820DEF88: the parts' +37248 = r4.
extern "C" REX_FUNC(sub_820DEF88) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DEF88(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + 37248, ctx.r4.u32);
}

// SceneInterior::Link: +24 = the car model, +28 = [model + 24] + 3760, +32
// = [model + 24] + 4048.
extern "C" REX_FUNC(sub_820E3398) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E3398(ctx, base);
    return;
  }
  const uint32_t interior = ctx.r3.u32, model = ctx.r4.u32;
  StoreU32(base, interior + 24, model);
  StoreU32(base, interior + 28, LoadU32(base, model + 24) + 3760);
  StoreU32(base, interior + 32, LoadU32(base, model + 24) + 4048);
}

// sub_820E3220: +11452 = r4.
extern "C" REX_FUNC(sub_820E3220) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E3220(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + 11452, ctx.r4.u32);
}

// CarModel::SetRecord: memcpy of the car record (3,696 bytes) to +32; the
// copy's r3 left.
extern "C" REX_FUNC(sub_820D3910) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3910(ctx, base);
    return;
  }
  Memcpy(ctx, base, ctx.r3.u32 + 32, ctx.r4.u32, 3696);
}

// The records' defaults. Each leaves r3 as the original does.
extern "C" REX_FUNC(sub_820D3780) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3780(ctx, base);
    return;
  }
  Record12Reset(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820D07D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D07D0(ctx, base);
    return;
  }
  guest_memory::Apply(base, ctx.r3.u32, kLayerReset0);
}

extern "C" REX_FUNC(sub_820D0848) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D0848(ctx, base);
    return;
  }
  const uint32_t record = ctx.r3.u32;
  guest_memory::Apply(base, record, kCarRecordReset0);
  ctx.r3.u64 = record + 3568;
}

extern "C" REX_FUNC(sub_820D31A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D31A0(ctx, base);
    return;
  }
  PartsRecordReset(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820D3488) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3488(ctx, base);
    return;
  }
  InteriorRecordReset(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820D3100) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3100(ctx, base);
    return;
  }
  const uint32_t record = ctx.r3.u32;
  CockpitRecordReset(base, record);
  ctx.r3.u64 = record + 24;
}

extern "C" REX_FUNC(sub_820D3548) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3548(ctx, base);
    return;
  }
  guest_memory::Apply(base, ctx.r3.u32, kWheelsRecordReset0);
}

extern "C" REX_FUNC(sub_820D3128) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3128(ctx, base);
    return;
  }
  Pool4RecordReset(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820D4250) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D4250(ctx, base);
    return;
  }
  CarRecordSetW(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820D94D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D94D8(ctx, base);
    return;
  }
  const uint32_t record = ctx.r3.u32;
  SceneModelRecordReset(base, record);
  ctx.r3.u64 = record + 4192;
}

// SceneModelRecord::SceneModelRecord: CarRecord::SetW on +16, Record12::
// Reset on +4080 and +4092; returns the record.
extern "C" REX_FUNC(sub_820DAF08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DAF08(ctx, base);
    return;
  }
  const uint32_t record = ctx.r3.u32;
  CarRecordSetW(base, record + 16);
  Record12Reset(base, record + 4080);
  Record12Reset(base, record + 4092);
  ctx.r3.u64 = record;
}

// The copies: r3 to, r4 from; each returns `to`.
extern "C" REX_FUNC(sub_820D7B98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D7B98(ctx, base);
    return;
  }
  const uint32_t to = ctx.r3.u32;
  CarRecordCopy(ctx, base, to, ctx.r4.u32);
  ctx.r3.u64 = to;
}

extern "C" REX_FUNC(sub_820DAF58) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820DAF58(ctx, base);
    return;
  }
  const uint32_t to = ctx.r3.u32;
  SceneModelRecordCopy(ctx, base, to, ctx.r4.u32);
  ctx.r3.u64 = to;
}

// CarModel::AllocPages: VirtualAlloc (sub_82292DD8: 0, r5 bytes,
// MEM_COMMIT, PAGE_READWRITE) into [r4]; the pages left in r3.
extern "C" REX_FUNC(sub_820C3AB0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C3AB0(ctx, base);
    return;
  }
  const uint32_t out = ctx.r4.u32;
  ctx.r3.u64 = 0;
  ctx.r4.u64 = ctx.r5.u32;
  ctx.r5.u64 = 4096;
  ctx.r6.u64 = 4;
  sub_82292DD8(ctx, base);
  StoreU32(base, out, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820D9E48) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D9E48(ctx, base);
    return;
  }
  Update(ctx, base, ctx.r3.u32);
}

// SceneParts::Apply (sub_820E0458): unless the parts' record ([+37284] +
// 4) is kind 1, the record (r4) copied to +108 (60 bytes) and the parts
// rebuilt (sub_820E0188).
extern "C" REX_FUNC(sub_820E0458) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E0458(ctx, base);
    return;
  }
  const uint32_t parts = ctx.r3.u32;
  if (int32_t(LoadU32(base, LoadU32(base, parts + 37284) + 4)) == 1) {
    return;
  }
  Memcpy(ctx, base, parts + 108, ctx.r4.u32, 60);
  ctx.r3.u64 = parts;
  sub_820E0188(ctx, base);
}

// SceneInterior::Apply (sub_820E3E20): the record (r4, 120 bytes) copied
// to +40 and to [+36] + 28; with +140 set, +9656 = [+11668] + 464 x [+140]
// + 784; each of the six lights ([+11632 + 4i]) +452 = sub_820D4228 of
// [[+24] + 24]; then sub_820E3500.
extern "C" REX_FUNC(sub_820E3E20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820E3E20(ctx, base);
    return;
  }
  const uint32_t interior = ctx.r3.u32, record = ctx.r4.u32;
  Memcpy(ctx, base, interior + 40, record, 120);
  Memcpy(ctx, base, LoadU32(base, interior + 36) + 28, record, 120);
  const int32_t index = int32_t(LoadU32(base, interior + 140));
  if (index != 0) {
    StoreU32(base, interior + 9656,
             LoadU32(base, interior + 11668) + uint32_t(index * 464) + 784);
  }
  for (uint32_t i = 0; i < 6; i++) {
    ctx.r3.u64 = LoadU32(base, LoadU32(base, interior + 24) + 24);
    sub_820D4228(ctx, base);
    StoreU32(base, LoadU32(base, interior + 11632 + 4 * i) + 452, ctx.r3.u32);
  }
  ctx.r3.u64 = interior;
  sub_820E3500(ctx, base);
}

// SceneModel::ReloadDecals (sub_820D9A10): with +5036 set and the model
// not kind 3 (+32), +5036 and +5056 = 0, the car model's record set from
// +48 (CarModel::SetRecord) and its decals rebuilt (sub_820D59C8).
extern "C" REX_FUNC(sub_820D9A10) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D9A10(ctx, base);
    return;
  }
  const uint32_t model = ctx.r3.u32;
  if (LoadU32(base, model + 5036) == 0 || int32_t(LoadU32(base, model + 32)) == 3) {
    return;
  }
  StoreU32(base, model + 5036, 0);
  StoreU32(base, model + 5056, 0);
  ctx.r3.u64 = LoadU32(base, LoadU32(base, model + 5048) + 24);
  ctx.r4.u64 = model + 48;
  sub_820D3910(ctx, base);
  ctx.r3.u64 = LoadU32(base, LoadU32(base, model + 5048) + 24);
  sub_820D59C8(ctx, base);
}

// SceneModel::AddSecondWheels (sub_820D9AC8): once (+5032), the scene's
// (+5116) pool 5 entry made (sub_820C5868, sub_820C59D8 into +5108), the
// car model linked (sub_820DC710) and its record set from +4068
// (sub_820C58E0); returns 1, or 0 when done before.
extern "C" REX_FUNC(sub_820D9AC8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D9AC8(ctx, base);
    return;
  }
  const uint32_t model = ctx.r3.u32;
  if (LoadU32(base, model + 5032) != 0) {
    ctx.r3.u64 = 0;
    return;
  }
  StoreU32(base, model + 5032, 1);
  StoreU32(base, model + 5112, 0);
  ctx.r3.u64 = LoadU32(base, model + 5116);
  ctx.r4.u64 = 5;
  ctx.r5.u64 = 0;
  sub_820C5868(ctx, base);
  const uint32_t entry = ctx.r3.u32;
  ctx.r3.u64 = LoadU32(base, model + 5116);
  ctx.r4.u64 = 5;
  ctx.r5.u64 = entry;
  sub_820C59D8(ctx, base);
  StoreU32(base, model + 5108, ctx.r3.u32);
  ctx.r4.u64 = LoadU32(base, model + 5048);
  sub_820DC710(ctx, base);
  ctx.r3.u64 = LoadU32(base, model + 5116);
  ctx.r4.u64 = 5;
  ctx.r5.u64 = entry;
  ctx.r6.u64 = model + 4068;
  sub_820C58E0(ctx, base);
  ctx.r3.u64 = 1;
}
