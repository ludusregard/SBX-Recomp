// The scene loaders as our own C++ (--native_scene_loaders, on by
// default): the car model's, cockpit's, traffic's, wheels' and parts'
// loads' constructors and destructors, the pool 4 objects' and wheels'
// resets and records, their releases, and the load checks (a car model, a
// pool 4 object, an XPR's count).
//
// Written from SBX's generated code: each constructor's stores as a table
// in its order, its calls in their place. The loads, the models' builds,
// the palettes and the searches stay recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scene_loaders, true, "SBX",
                    "Our own C++ for the scene loaders' constructors, destructors, resets and load checks instead of the recompiled functions; the loads themselves stay recompiled");

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

// CarModelLoads::CarModelLoads (sub_820D3A80).
// 12 stores
constexpr Fill kCarModelLoads_CarModelLoads0[] = {
    {272, kWord, 0x0, 12, 4},
};

// CockpitLoads::CockpitLoads (sub_820D5CB8).
// 12 stores
constexpr Fill kCockpitLoads_CockpitLoads0[] = {
    {272, kWord, 0x0, 12, 4},
};

// TrafficLoads::TrafficLoads (sub_820D78F0).
// 12 stores
constexpr Fill kTrafficLoads_TrafficLoads0[] = {
    {288, kWord, 0x0, 12, 4},
};

// WheelsLoads::WheelsLoads (sub_820D8C28).
// 12 stores
constexpr Fill kWheelsLoads_WheelsLoads0[] = {
    {2000, kWord, 0x0, 12, 4},
};

// CarPartsLoads::CarPartsLoads (sub_820D6C38).
// 30 stores
constexpr Fill kCarPartsLoads_CarPartsLoads0[] = {
    {105436, kImage, 0x82000A20, 30, 16},
};

// CarParts::Release (sub_820D69B0).
// 8 stores
constexpr Fill kCarParts_Release0[] = {
    {5724, kWord, 0x0, 5, 4}, {5712, kWord, 0x0}, {5716, kWord, 0xFFFFFFFF}, {5720, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release1[] = {
    {6188, kWord, 0x0, 5, 4}, {6176, kWord, 0x0}, {6180, kWord, 0xFFFFFFFF}, {6184, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release2[] = {
    {6652, kWord, 0x0, 5, 4}, {6640, kWord, 0x0}, {6644, kWord, 0xFFFFFFFF}, {6648, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release3[] = {
    {7116, kWord, 0x0, 5, 4}, {7104, kWord, 0x0}, {7108, kWord, 0xFFFFFFFF}, {7112, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release4[] = {
    {7580, kWord, 0x0, 5, 4}, {7568, kWord, 0x0}, {7572, kWord, 0xFFFFFFFF}, {7576, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release5[] = {
    {8044, kWord, 0x0, 5, 4}, {8032, kWord, 0x0}, {8036, kWord, 0xFFFFFFFF}, {8040, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release6[] = {
    {8508, kWord, 0x0, 5, 4}, {8496, kWord, 0x0}, {8500, kWord, 0xFFFFFFFF}, {8504, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release7[] = {
    {8972, kWord, 0x0, 5, 4}, {8960, kWord, 0x0}, {8964, kWord, 0xFFFFFFFF}, {8968, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release8[] = {
    {9436, kWord, 0x0, 5, 4}, {9424, kWord, 0x0}, {9428, kWord, 0xFFFFFFFF}, {9432, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release9[] = {
    {9900, kWord, 0x0, 5, 4}, {9888, kWord, 0x0}, {9892, kWord, 0xFFFFFFFF}, {9896, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release10[] = {
    {10364, kWord, 0x0, 5, 4}, {10352, kWord, 0x0}, {10356, kWord, 0xFFFFFFFF}, {10360, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release11[] = {
    {10828, kWord, 0x0, 5, 4}, {10816, kWord, 0x0}, {10820, kWord, 0xFFFFFFFF}, {10824, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release12[] = {
    {11292, kWord, 0x0, 5, 4}, {11280, kWord, 0x0}, {11284, kWord, 0xFFFFFFFF}, {11288, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release13[] = {
    {11756, kWord, 0x0, 5, 4}, {11744, kWord, 0x0}, {11748, kWord, 0xFFFFFFFF}, {11752, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release14[] = {
    {12220, kWord, 0x0, 5, 4}, {12208, kWord, 0x0}, {12212, kWord, 0xFFFFFFFF}, {12216, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release15[] = {
    {12684, kWord, 0x0, 5, 4}, {12672, kWord, 0x0}, {12676, kWord, 0xFFFFFFFF}, {12680, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release16[] = {
    {13148, kWord, 0x0, 5, 4}, {13136, kWord, 0x0}, {13140, kWord, 0xFFFFFFFF}, {13144, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release17[] = {
    {13612, kWord, 0x0, 5, 4}, {13600, kWord, 0x0}, {13604, kWord, 0xFFFFFFFF}, {13608, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release18[] = {
    {14076, kWord, 0x0, 5, 4}, {14064, kWord, 0x0}, {14068, kWord, 0xFFFFFFFF}, {14072, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release19[] = {
    {14540, kWord, 0x0, 5, 4}, {14528, kWord, 0x0}, {14532, kWord, 0xFFFFFFFF}, {14536, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release20[] = {
    {15004, kWord, 0x0, 5, 4}, {14992, kWord, 0x0}, {14996, kWord, 0xFFFFFFFF}, {15000, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release21[] = {
    {15468, kWord, 0x0, 5, 4}, {15456, kWord, 0x0}, {15460, kWord, 0xFFFFFFFF}, {15464, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release22[] = {
    {15932, kWord, 0x0, 5, 4}, {15920, kWord, 0x0}, {15924, kWord, 0xFFFFFFFF}, {15928, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release23[] = {
    {16396, kWord, 0x0, 5, 4}, {16384, kWord, 0x0}, {16388, kWord, 0xFFFFFFFF}, {16392, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release24[] = {
    {16860, kWord, 0x0, 5, 4}, {16848, kWord, 0x0}, {16852, kWord, 0xFFFFFFFF}, {16856, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release25[] = {
    {17324, kWord, 0x0, 5, 4}, {17312, kWord, 0x0}, {17316, kWord, 0xFFFFFFFF}, {17320, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release26[] = {
    {17788, kWord, 0x0, 5, 4}, {17776, kWord, 0x0}, {17780, kWord, 0xFFFFFFFF}, {17784, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release27[] = {
    {18252, kWord, 0x0, 5, 4}, {18240, kWord, 0x0}, {18244, kWord, 0xFFFFFFFF}, {18248, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release28[] = {
    {18716, kWord, 0x0, 5, 4}, {18704, kWord, 0x0}, {18708, kWord, 0xFFFFFFFF}, {18712, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release29[] = {
    {19180, kWord, 0x0, 5, 4}, {19168, kWord, 0x0}, {19172, kWord, 0xFFFFFFFF}, {19176, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release30[] = {
    {19644, kWord, 0x0, 5, 4}, {19632, kWord, 0x0}, {19636, kWord, 0xFFFFFFFF}, {19640, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release31[] = {
    {20108, kWord, 0x0, 5, 4}, {20096, kWord, 0x0}, {20100, kWord, 0xFFFFFFFF}, {20104, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release32[] = {
    {20572, kWord, 0x0, 5, 4}, {20560, kWord, 0x0}, {20564, kWord, 0xFFFFFFFF}, {20568, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release33[] = {
    {21036, kWord, 0x0, 5, 4}, {21024, kWord, 0x0}, {21028, kWord, 0xFFFFFFFF}, {21032, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release34[] = {
    {21500, kWord, 0x0, 5, 4}, {21488, kWord, 0x0}, {21492, kWord, 0xFFFFFFFF}, {21496, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release35[] = {
    {21964, kWord, 0x0, 5, 4}, {21952, kWord, 0x0}, {21956, kWord, 0xFFFFFFFF}, {21960, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release36[] = {
    {22428, kWord, 0x0, 5, 4}, {22416, kWord, 0x0}, {22420, kWord, 0xFFFFFFFF}, {22424, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release37[] = {
    {22892, kWord, 0x0, 5, 4}, {22880, kWord, 0x0}, {22884, kWord, 0xFFFFFFFF}, {22888, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release38[] = {
    {23356, kWord, 0x0, 5, 4}, {23344, kWord, 0x0}, {23348, kWord, 0xFFFFFFFF}, {23352, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release39[] = {
    {23820, kWord, 0x0, 5, 4}, {23808, kWord, 0x0}, {23812, kWord, 0xFFFFFFFF}, {23816, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release40[] = {
    {24284, kWord, 0x0, 5, 4}, {24272, kWord, 0x0}, {24276, kWord, 0xFFFFFFFF}, {24280, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release41[] = {
    {24748, kWord, 0x0, 5, 4}, {24736, kWord, 0x0}, {24740, kWord, 0xFFFFFFFF}, {24744, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release42[] = {
    {25212, kWord, 0x0, 5, 4}, {25200, kWord, 0x0}, {25204, kWord, 0xFFFFFFFF}, {25208, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release43[] = {
    {25676, kWord, 0x0, 5, 4}, {25664, kWord, 0x0}, {25668, kWord, 0xFFFFFFFF}, {25672, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release44[] = {
    {26140, kWord, 0x0, 5, 4}, {26128, kWord, 0x0}, {26132, kWord, 0xFFFFFFFF}, {26136, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release45[] = {
    {26604, kWord, 0x0, 5, 4}, {26592, kWord, 0x0}, {26596, kWord, 0xFFFFFFFF}, {26600, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release46[] = {
    {27068, kWord, 0x0, 5, 4}, {27056, kWord, 0x0}, {27060, kWord, 0xFFFFFFFF}, {27064, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release47[] = {
    {27532, kWord, 0x0, 5, 4}, {27520, kWord, 0x0}, {27524, kWord, 0xFFFFFFFF}, {27528, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release48[] = {
    {27996, kWord, 0x0, 5, 4}, {27984, kWord, 0x0}, {27988, kWord, 0xFFFFFFFF}, {27992, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release49[] = {
    {28460, kWord, 0x0, 5, 4}, {28448, kWord, 0x0}, {28452, kWord, 0xFFFFFFFF}, {28456, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release50[] = {
    {28924, kWord, 0x0, 5, 4}, {28912, kWord, 0x0}, {28916, kWord, 0xFFFFFFFF}, {28920, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release51[] = {
    {29388, kWord, 0x0, 5, 4}, {29376, kWord, 0x0}, {29380, kWord, 0xFFFFFFFF}, {29384, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release52[] = {
    {29852, kWord, 0x0, 5, 4}, {29840, kWord, 0x0}, {29844, kWord, 0xFFFFFFFF}, {29848, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release53[] = {
    {30316, kWord, 0x0, 5, 4}, {30304, kWord, 0x0}, {30308, kWord, 0xFFFFFFFF}, {30312, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release54[] = {
    {30780, kWord, 0x0, 5, 4}, {30768, kWord, 0x0}, {30772, kWord, 0xFFFFFFFF}, {30776, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release55[] = {
    {31244, kWord, 0x0, 5, 4}, {31232, kWord, 0x0}, {31236, kWord, 0xFFFFFFFF}, {31240, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release56[] = {
    {31708, kWord, 0x0, 5, 4}, {31696, kWord, 0x0}, {31700, kWord, 0xFFFFFFFF}, {31704, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release57[] = {
    {32172, kWord, 0x0, 5, 4}, {32160, kWord, 0x0}, {32164, kWord, 0xFFFFFFFF}, {32168, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release58[] = {
    {32636, kWord, 0x0, 5, 4}, {32624, kWord, 0x0}, {32628, kWord, 0xFFFFFFFF}, {32632, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release59[] = {
    {33100, kWord, 0x0, 5, 4}, {33088, kWord, 0x0}, {33092, kWord, 0xFFFFFFFF}, {33096, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release60[] = {
    {33564, kWord, 0x0, 5, 4}, {33552, kWord, 0x0}, {33556, kWord, 0xFFFFFFFF}, {33560, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release61[] = {
    {34028, kWord, 0x0, 5, 4}, {34016, kWord, 0x0}, {34020, kWord, 0xFFFFFFFF}, {34024, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release62[] = {
    {34492, kWord, 0x0, 5, 4}, {34480, kWord, 0x0}, {34484, kWord, 0xFFFFFFFF}, {34488, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release63[] = {
    {34956, kWord, 0x0, 5, 4}, {34944, kWord, 0x0}, {34948, kWord, 0xFFFFFFFF}, {34952, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release64[] = {
    {35420, kWord, 0x0, 5, 4}, {35408, kWord, 0x0}, {35412, kWord, 0xFFFFFFFF}, {35416, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release65[] = {
    {35884, kWord, 0x0, 5, 4}, {35872, kWord, 0x0}, {35876, kWord, 0xFFFFFFFF}, {35880, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release66[] = {
    {36348, kWord, 0x0, 5, 4}, {36336, kWord, 0x0}, {36340, kWord, 0xFFFFFFFF}, {36344, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release67[] = {
    {36812, kWord, 0x0, 5, 4}, {36800, kWord, 0x0}, {36804, kWord, 0xFFFFFFFF}, {36808, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release68[] = {
    {37276, kWord, 0x0, 5, 4}, {37264, kWord, 0x0}, {37268, kWord, 0xFFFFFFFF}, {37272, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release69[] = {
    {37740, kWord, 0x0, 5, 4}, {37728, kWord, 0x0}, {37732, kWord, 0xFFFFFFFF}, {37736, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release70[] = {
    {38204, kWord, 0x0, 5, 4}, {38192, kWord, 0x0}, {38196, kWord, 0xFFFFFFFF}, {38200, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release71[] = {
    {38668, kWord, 0x0, 5, 4}, {38656, kWord, 0x0}, {38660, kWord, 0xFFFFFFFF}, {38664, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release72[] = {
    {39132, kWord, 0x0, 5, 4}, {39120, kWord, 0x0}, {39124, kWord, 0xFFFFFFFF}, {39128, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release73[] = {
    {39596, kWord, 0x0, 5, 4}, {39584, kWord, 0x0}, {39588, kWord, 0xFFFFFFFF}, {39592, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release74[] = {
    {40060, kWord, 0x0, 5, 4}, {40048, kWord, 0x0}, {40052, kWord, 0xFFFFFFFF}, {40056, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release75[] = {
    {40524, kWord, 0x0, 5, 4}, {40512, kWord, 0x0}, {40516, kWord, 0xFFFFFFFF}, {40520, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release76[] = {
    {40988, kWord, 0x0, 5, 4}, {40976, kWord, 0x0}, {40980, kWord, 0xFFFFFFFF}, {40984, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release77[] = {
    {41452, kWord, 0x0, 5, 4}, {41440, kWord, 0x0}, {41444, kWord, 0xFFFFFFFF}, {41448, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release78[] = {
    {41916, kWord, 0x0, 5, 4}, {41904, kWord, 0x0}, {41908, kWord, 0xFFFFFFFF}, {41912, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release79[] = {
    {42380, kWord, 0x0, 5, 4}, {42368, kWord, 0x0}, {42372, kWord, 0xFFFFFFFF}, {42376, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release80[] = {
    {42844, kWord, 0x0, 5, 4}, {42832, kWord, 0x0}, {42836, kWord, 0xFFFFFFFF}, {42840, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release81[] = {
    {43308, kWord, 0x0, 5, 4}, {43296, kWord, 0x0}, {43300, kWord, 0xFFFFFFFF}, {43304, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release82[] = {
    {43772, kWord, 0x0, 5, 4}, {43760, kWord, 0x0}, {43764, kWord, 0xFFFFFFFF}, {43768, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release83[] = {
    {44236, kWord, 0x0, 5, 4}, {44224, kWord, 0x0}, {44228, kWord, 0xFFFFFFFF}, {44232, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release84[] = {
    {44700, kWord, 0x0, 5, 4}, {44688, kWord, 0x0}, {44692, kWord, 0xFFFFFFFF}, {44696, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release85[] = {
    {45164, kWord, 0x0, 5, 4}, {45152, kWord, 0x0}, {45156, kWord, 0xFFFFFFFF}, {45160, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release86[] = {
    {45628, kWord, 0x0, 5, 4}, {45616, kWord, 0x0}, {45620, kWord, 0xFFFFFFFF}, {45624, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release87[] = {
    {46092, kWord, 0x0, 5, 4}, {46080, kWord, 0x0}, {46084, kWord, 0xFFFFFFFF}, {46088, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release88[] = {
    {46556, kWord, 0x0, 5, 4}, {46544, kWord, 0x0}, {46548, kWord, 0xFFFFFFFF}, {46552, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release89[] = {
    {47020, kWord, 0x0, 5, 4}, {47008, kWord, 0x0}, {47012, kWord, 0xFFFFFFFF}, {47016, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release90[] = {
    {47484, kWord, 0x0, 5, 4}, {47472, kWord, 0x0}, {47476, kWord, 0xFFFFFFFF}, {47480, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release91[] = {
    {47948, kWord, 0x0, 5, 4}, {47936, kWord, 0x0}, {47940, kWord, 0xFFFFFFFF}, {47944, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release92[] = {
    {48412, kWord, 0x0, 5, 4}, {48400, kWord, 0x0}, {48404, kWord, 0xFFFFFFFF}, {48408, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release93[] = {
    {48876, kWord, 0x0, 5, 4}, {48864, kWord, 0x0}, {48868, kWord, 0xFFFFFFFF}, {48872, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release94[] = {
    {49340, kWord, 0x0, 5, 4}, {49328, kWord, 0x0}, {49332, kWord, 0xFFFFFFFF}, {49336, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release95[] = {
    {49804, kWord, 0x0, 5, 4}, {49792, kWord, 0x0}, {49796, kWord, 0xFFFFFFFF}, {49800, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release96[] = {
    {50268, kWord, 0x0, 5, 4}, {50256, kWord, 0x0}, {50260, kWord, 0xFFFFFFFF}, {50264, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release97[] = {
    {50732, kWord, 0x0, 5, 4}, {50720, kWord, 0x0}, {50724, kWord, 0xFFFFFFFF}, {50728, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release98[] = {
    {51196, kWord, 0x0, 5, 4}, {51184, kWord, 0x0}, {51188, kWord, 0xFFFFFFFF}, {51192, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release99[] = {
    {51660, kWord, 0x0, 5, 4}, {51648, kWord, 0x0}, {51652, kWord, 0xFFFFFFFF}, {51656, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release100[] = {
    {52124, kWord, 0x0, 5, 4}, {52112, kWord, 0x0}, {52116, kWord, 0xFFFFFFFF}, {52120, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release101[] = {
    {52588, kWord, 0x0, 5, 4}, {52576, kWord, 0x0}, {52580, kWord, 0xFFFFFFFF}, {52584, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release102[] = {
    {53052, kWord, 0x0, 5, 4}, {53040, kWord, 0x0}, {53044, kWord, 0xFFFFFFFF}, {53048, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release103[] = {
    {53516, kWord, 0x0, 5, 4}, {53504, kWord, 0x0}, {53508, kWord, 0xFFFFFFFF}, {53512, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release104[] = {
    {53980, kWord, 0x0, 5, 4}, {53968, kWord, 0x0}, {53972, kWord, 0xFFFFFFFF}, {53976, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release105[] = {
    {54444, kWord, 0x0, 5, 4}, {54432, kWord, 0x0}, {54436, kWord, 0xFFFFFFFF}, {54440, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release106[] = {
    {54908, kWord, 0x0, 5, 4}, {54896, kWord, 0x0}, {54900, kWord, 0xFFFFFFFF}, {54904, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release107[] = {
    {55372, kWord, 0x0, 5, 4}, {55360, kWord, 0x0}, {55364, kWord, 0xFFFFFFFF}, {55368, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release108[] = {
    {55836, kWord, 0x0, 5, 4}, {55824, kWord, 0x0}, {55828, kWord, 0xFFFFFFFF}, {55832, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release109[] = {
    {56300, kWord, 0x0, 5, 4}, {56288, kWord, 0x0}, {56292, kWord, 0xFFFFFFFF}, {56296, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release110[] = {
    {56764, kWord, 0x0, 5, 4}, {56752, kWord, 0x0}, {56756, kWord, 0xFFFFFFFF}, {56760, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release111[] = {
    {57228, kWord, 0x0, 5, 4}, {57216, kWord, 0x0}, {57220, kWord, 0xFFFFFFFF}, {57224, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release112[] = {
    {57692, kWord, 0x0, 5, 4}, {57680, kWord, 0x0}, {57684, kWord, 0xFFFFFFFF}, {57688, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release113[] = {
    {58156, kWord, 0x0, 5, 4}, {58144, kWord, 0x0}, {58148, kWord, 0xFFFFFFFF}, {58152, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release114[] = {
    {58620, kWord, 0x0, 5, 4}, {58608, kWord, 0x0}, {58612, kWord, 0xFFFFFFFF}, {58616, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release115[] = {
    {59084, kWord, 0x0, 5, 4}, {59072, kWord, 0x0}, {59076, kWord, 0xFFFFFFFF}, {59080, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release116[] = {
    {59548, kWord, 0x0, 5, 4}, {59536, kWord, 0x0}, {59540, kWord, 0xFFFFFFFF}, {59544, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release117[] = {
    {60012, kWord, 0x0, 5, 4}, {60000, kWord, 0x0}, {60004, kWord, 0xFFFFFFFF}, {60008, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release118[] = {
    {60476, kWord, 0x0, 5, 4}, {60464, kWord, 0x0}, {60468, kWord, 0xFFFFFFFF}, {60472, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release119[] = {
    {60940, kWord, 0x0, 5, 4}, {60928, kWord, 0x0}, {60932, kWord, 0xFFFFFFFF}, {60936, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release120[] = {
    {61404, kWord, 0x0, 5, 4}, {61392, kWord, 0x0}, {61396, kWord, 0xFFFFFFFF}, {61400, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release121[] = {
    {61868, kWord, 0x0, 5, 4}, {61856, kWord, 0x0}, {61860, kWord, 0xFFFFFFFF}, {61864, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release122[] = {
    {62332, kWord, 0x0, 5, 4}, {62320, kWord, 0x0}, {62324, kWord, 0xFFFFFFFF}, {62328, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release123[] = {
    {62796, kWord, 0x0, 5, 4}, {62784, kWord, 0x0}, {62788, kWord, 0xFFFFFFFF}, {62792, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release124[] = {
    {63260, kWord, 0x0, 5, 4}, {63248, kWord, 0x0}, {63252, kWord, 0xFFFFFFFF}, {63256, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release125[] = {
    {63724, kWord, 0x0, 5, 4}, {63712, kWord, 0x0}, {63716, kWord, 0xFFFFFFFF}, {63720, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release126[] = {
    {64188, kWord, 0x0, 5, 4}, {64176, kWord, 0x0}, {64180, kWord, 0xFFFFFFFF}, {64184, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release127[] = {
    {64652, kWord, 0x0, 5, 4}, {64640, kWord, 0x0}, {64644, kWord, 0xFFFFFFFF}, {64648, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release128[] = {
    {65116, kWord, 0x0, 5, 4}, {65104, kWord, 0x0}, {65108, kWord, 0xFFFFFFFF}, {65112, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release129[] = {
    {65580, kWord, 0x0, 5, 4}, {65568, kWord, 0x0}, {65572, kWord, 0xFFFFFFFF}, {65576, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release130[] = {
    {66044, kWord, 0x0, 5, 4}, {66032, kWord, 0x0}, {66036, kWord, 0xFFFFFFFF}, {66040, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release131[] = {
    {66508, kWord, 0x0, 5, 4}, {66496, kWord, 0x0}, {66500, kWord, 0xFFFFFFFF}, {66504, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release132[] = {
    {66972, kWord, 0x0, 5, 4}, {66960, kWord, 0x0}, {66964, kWord, 0xFFFFFFFF}, {66968, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release133[] = {
    {67436, kWord, 0x0, 5, 4}, {67424, kWord, 0x0}, {67428, kWord, 0xFFFFFFFF}, {67432, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release134[] = {
    {67900, kWord, 0x0, 5, 4}, {67888, kWord, 0x0}, {67892, kWord, 0xFFFFFFFF}, {67896, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release135[] = {
    {68364, kWord, 0x0, 5, 4}, {68352, kWord, 0x0}, {68356, kWord, 0xFFFFFFFF}, {68360, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release136[] = {
    {68828, kWord, 0x0, 5, 4}, {68816, kWord, 0x0}, {68820, kWord, 0xFFFFFFFF}, {68824, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release137[] = {
    {69292, kWord, 0x0, 5, 4}, {69280, kWord, 0x0}, {69284, kWord, 0xFFFFFFFF}, {69288, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release138[] = {
    {69756, kWord, 0x0, 5, 4}, {69744, kWord, 0x0}, {69748, kWord, 0xFFFFFFFF}, {69752, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release139[] = {
    {70220, kWord, 0x0, 5, 4}, {70208, kWord, 0x0}, {70212, kWord, 0xFFFFFFFF}, {70216, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release140[] = {
    {70684, kWord, 0x0, 5, 4}, {70672, kWord, 0x0}, {70676, kWord, 0xFFFFFFFF}, {70680, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release141[] = {
    {71148, kWord, 0x0, 5, 4}, {71136, kWord, 0x0}, {71140, kWord, 0xFFFFFFFF}, {71144, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release142[] = {
    {71612, kWord, 0x0, 5, 4}, {71600, kWord, 0x0}, {71604, kWord, 0xFFFFFFFF}, {71608, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release143[] = {
    {72076, kWord, 0x0, 5, 4}, {72064, kWord, 0x0}, {72068, kWord, 0xFFFFFFFF}, {72072, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release144[] = {
    {72540, kWord, 0x0, 5, 4}, {72528, kWord, 0x0}, {72532, kWord, 0xFFFFFFFF}, {72536, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release145[] = {
    {73004, kWord, 0x0, 5, 4}, {72992, kWord, 0x0}, {72996, kWord, 0xFFFFFFFF}, {73000, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release146[] = {
    {73468, kWord, 0x0, 5, 4}, {73456, kWord, 0x0}, {73460, kWord, 0xFFFFFFFF}, {73464, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release147[] = {
    {73932, kWord, 0x0, 5, 4}, {73920, kWord, 0x0}, {73924, kWord, 0xFFFFFFFF}, {73928, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release148[] = {
    {74396, kWord, 0x0, 5, 4}, {74384, kWord, 0x0}, {74388, kWord, 0xFFFFFFFF}, {74392, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release149[] = {
    {74860, kWord, 0x0, 5, 4}, {74848, kWord, 0x0}, {74852, kWord, 0xFFFFFFFF}, {74856, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release150[] = {
    {75324, kWord, 0x0, 5, 4}, {75312, kWord, 0x0}, {75316, kWord, 0xFFFFFFFF}, {75320, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release151[] = {
    {75788, kWord, 0x0, 5, 4}, {75776, kWord, 0x0}, {75780, kWord, 0xFFFFFFFF}, {75784, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release152[] = {
    {76252, kWord, 0x0, 5, 4}, {76240, kWord, 0x0}, {76244, kWord, 0xFFFFFFFF}, {76248, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release153[] = {
    {76716, kWord, 0x0, 5, 4}, {76704, kWord, 0x0}, {76708, kWord, 0xFFFFFFFF}, {76712, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release154[] = {
    {77180, kWord, 0x0, 5, 4}, {77168, kWord, 0x0}, {77172, kWord, 0xFFFFFFFF}, {77176, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release155[] = {
    {77644, kWord, 0x0, 5, 4}, {77632, kWord, 0x0}, {77636, kWord, 0xFFFFFFFF}, {77640, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release156[] = {
    {78108, kWord, 0x0, 5, 4}, {78096, kWord, 0x0}, {78100, kWord, 0xFFFFFFFF}, {78104, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release157[] = {
    {78572, kWord, 0x0, 5, 4}, {78560, kWord, 0x0}, {78564, kWord, 0xFFFFFFFF}, {78568, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release158[] = {
    {79036, kWord, 0x0, 5, 4}, {79024, kWord, 0x0}, {79028, kWord, 0xFFFFFFFF}, {79032, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release159[] = {
    {79500, kWord, 0x0, 5, 4}, {79488, kWord, 0x0}, {79492, kWord, 0xFFFFFFFF}, {79496, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release160[] = {
    {79964, kWord, 0x0, 5, 4}, {79952, kWord, 0x0}, {79956, kWord, 0xFFFFFFFF}, {79960, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release161[] = {
    {80428, kWord, 0x0, 5, 4}, {80416, kWord, 0x0}, {80420, kWord, 0xFFFFFFFF}, {80424, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release162[] = {
    {80892, kWord, 0x0, 5, 4}, {80880, kWord, 0x0}, {80884, kWord, 0xFFFFFFFF}, {80888, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release163[] = {
    {81356, kWord, 0x0, 5, 4}, {81344, kWord, 0x0}, {81348, kWord, 0xFFFFFFFF}, {81352, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release164[] = {
    {81820, kWord, 0x0, 5, 4}, {81808, kWord, 0x0}, {81812, kWord, 0xFFFFFFFF}, {81816, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release165[] = {
    {82284, kWord, 0x0, 5, 4}, {82272, kWord, 0x0}, {82276, kWord, 0xFFFFFFFF}, {82280, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release166[] = {
    {82748, kWord, 0x0, 5, 4}, {82736, kWord, 0x0}, {82740, kWord, 0xFFFFFFFF}, {82744, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release167[] = {
    {83212, kWord, 0x0, 5, 4}, {83200, kWord, 0x0}, {83204, kWord, 0xFFFFFFFF}, {83208, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release168[] = {
    {83676, kWord, 0x0, 5, 4}, {83664, kWord, 0x0}, {83668, kWord, 0xFFFFFFFF}, {83672, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release169[] = {
    {84140, kWord, 0x0, 5, 4}, {84128, kWord, 0x0}, {84132, kWord, 0xFFFFFFFF}, {84136, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release170[] = {
    {84604, kWord, 0x0, 5, 4}, {84592, kWord, 0x0}, {84596, kWord, 0xFFFFFFFF}, {84600, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release171[] = {
    {85068, kWord, 0x0, 5, 4}, {85056, kWord, 0x0}, {85060, kWord, 0xFFFFFFFF}, {85064, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release172[] = {
    {85532, kWord, 0x0, 5, 4}, {85520, kWord, 0x0}, {85524, kWord, 0xFFFFFFFF}, {85528, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release173[] = {
    {85996, kWord, 0x0, 5, 4}, {85984, kWord, 0x0}, {85988, kWord, 0xFFFFFFFF}, {85992, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release174[] = {
    {86460, kWord, 0x0, 5, 4}, {86448, kWord, 0x0}, {86452, kWord, 0xFFFFFFFF}, {86456, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release175[] = {
    {86924, kWord, 0x0, 5, 4}, {86912, kWord, 0x0}, {86916, kWord, 0xFFFFFFFF}, {86920, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release176[] = {
    {87388, kWord, 0x0, 5, 4}, {87376, kWord, 0x0}, {87380, kWord, 0xFFFFFFFF}, {87384, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release177[] = {
    {87852, kWord, 0x0, 5, 4}, {87840, kWord, 0x0}, {87844, kWord, 0xFFFFFFFF}, {87848, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release178[] = {
    {88316, kWord, 0x0, 5, 4}, {88304, kWord, 0x0}, {88308, kWord, 0xFFFFFFFF}, {88312, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release179[] = {
    {88780, kWord, 0x0, 5, 4}, {88768, kWord, 0x0}, {88772, kWord, 0xFFFFFFFF}, {88776, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release180[] = {
    {89244, kWord, 0x0, 5, 4}, {89232, kWord, 0x0}, {89236, kWord, 0xFFFFFFFF}, {89240, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release181[] = {
    {89708, kWord, 0x0, 5, 4}, {89696, kWord, 0x0}, {89700, kWord, 0xFFFFFFFF}, {89704, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release182[] = {
    {90172, kWord, 0x0, 5, 4}, {90160, kWord, 0x0}, {90164, kWord, 0xFFFFFFFF}, {90168, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release183[] = {
    {90636, kWord, 0x0, 5, 4}, {90624, kWord, 0x0}, {90628, kWord, 0xFFFFFFFF}, {90632, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release184[] = {
    {91100, kWord, 0x0, 5, 4}, {91088, kWord, 0x0}, {91092, kWord, 0xFFFFFFFF}, {91096, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release185[] = {
    {91564, kWord, 0x0, 5, 4}, {91552, kWord, 0x0}, {91556, kWord, 0xFFFFFFFF}, {91560, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release186[] = {
    {92028, kWord, 0x0, 5, 4}, {92016, kWord, 0x0}, {92020, kWord, 0xFFFFFFFF}, {92024, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release187[] = {
    {92492, kWord, 0x0, 5, 4}, {92480, kWord, 0x0}, {92484, kWord, 0xFFFFFFFF}, {92488, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release188[] = {
    {92956, kWord, 0x0, 5, 4}, {92944, kWord, 0x0}, {92948, kWord, 0xFFFFFFFF}, {92952, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release189[] = {
    {93420, kWord, 0x0, 5, 4}, {93408, kWord, 0x0}, {93412, kWord, 0xFFFFFFFF}, {93416, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release190[] = {
    {93884, kWord, 0x0, 5, 4}, {93872, kWord, 0x0}, {93876, kWord, 0xFFFFFFFF}, {93880, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release191[] = {
    {94348, kWord, 0x0, 5, 4}, {94336, kWord, 0x0}, {94340, kWord, 0xFFFFFFFF}, {94344, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release192[] = {
    {94812, kWord, 0x0, 5, 4}, {94800, kWord, 0x0}, {94804, kWord, 0xFFFFFFFF}, {94808, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release193[] = {
    {95276, kWord, 0x0, 5, 4}, {95264, kWord, 0x0}, {95268, kWord, 0xFFFFFFFF}, {95272, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release194[] = {
    {95740, kWord, 0x0, 5, 4}, {95728, kWord, 0x0}, {95732, kWord, 0xFFFFFFFF}, {95736, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release195[] = {
    {96204, kWord, 0x0, 5, 4}, {96192, kWord, 0x0}, {96196, kWord, 0xFFFFFFFF}, {96200, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release196[] = {
    {96668, kWord, 0x0, 5, 4}, {96656, kWord, 0x0}, {96660, kWord, 0xFFFFFFFF}, {96664, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release197[] = {
    {97132, kWord, 0x0, 5, 4}, {97120, kWord, 0x0}, {97124, kWord, 0xFFFFFFFF}, {97128, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release198[] = {
    {97596, kWord, 0x0, 5, 4}, {97584, kWord, 0x0}, {97588, kWord, 0xFFFFFFFF}, {97592, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release199[] = {
    {98060, kWord, 0x0, 5, 4}, {98048, kWord, 0x0}, {98052, kWord, 0xFFFFFFFF}, {98056, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release200[] = {
    {98524, kWord, 0x0, 5, 4}, {98512, kWord, 0x0}, {98516, kWord, 0xFFFFFFFF}, {98520, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release201[] = {
    {98988, kWord, 0x0, 5, 4}, {98976, kWord, 0x0}, {98980, kWord, 0xFFFFFFFF}, {98984, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release202[] = {
    {99452, kWord, 0x0, 5, 4}, {99440, kWord, 0x0}, {99444, kWord, 0xFFFFFFFF}, {99448, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release203[] = {
    {99916, kWord, 0x0, 5, 4}, {99904, kWord, 0x0}, {99908, kWord, 0xFFFFFFFF}, {99912, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release204[] = {
    {100380, kWord, 0x0, 5, 4}, {100368, kWord, 0x0}, {100372, kWord, 0xFFFFFFFF}, {100376, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release205[] = {
    {100844, kWord, 0x0, 5, 4}, {100832, kWord, 0x0}, {100836, kWord, 0xFFFFFFFF}, {100840, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release206[] = {
    {101308, kWord, 0x0, 5, 4}, {101296, kWord, 0x0}, {101300, kWord, 0xFFFFFFFF}, {101304, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release207[] = {
    {101772, kWord, 0x0, 5, 4}, {101760, kWord, 0x0}, {101764, kWord, 0xFFFFFFFF}, {101768, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release208[] = {
    {102236, kWord, 0x0, 5, 4}, {102224, kWord, 0x0}, {102228, kWord, 0xFFFFFFFF}, {102232, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release209[] = {
    {102700, kWord, 0x0, 5, 4}, {102688, kWord, 0x0}, {102692, kWord, 0xFFFFFFFF}, {102696, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release210[] = {
    {103164, kWord, 0x0, 5, 4}, {103152, kWord, 0x0}, {103156, kWord, 0xFFFFFFFF}, {103160, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release211[] = {
    {103628, kWord, 0x0, 5, 4}, {103616, kWord, 0x0}, {103620, kWord, 0xFFFFFFFF}, {103624, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release212[] = {
    {104092, kWord, 0x0, 5, 4}, {104080, kWord, 0x0}, {104084, kWord, 0xFFFFFFFF}, {104088, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release213[] = {
    {104556, kWord, 0x0, 5, 4}, {104544, kWord, 0x0}, {104548, kWord, 0xFFFFFFFF}, {104552, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release214[] = {
    {105020, kWord, 0x0, 5, 4}, {105008, kWord, 0x0}, {105012, kWord, 0xFFFFFFFF}, {105016, kWord, 0x0},
};
// 8 stores
constexpr Fill kCarParts_Release215[] = {
    {105484, kWord, 0x0, 5, 4}, {105472, kWord, 0x0}, {105476, kWord, 0xFFFFFFFF}, {105480, kWord, 0x0},
};

// Wheels::Reset (sub_820D8810).
// 41 stores
constexpr Fill kWheels_Reset0[] = {
    {24, kWord, 0x0}, {3268, kImage, 0x826B2B20}, {28, kWord, 0x1}, {48, kImage, 0x82002520},
    {44, kWord, 0xFFFFFFFF}, {64, kImage, 0x82002520}, {60, kWord, 0xFFFFFFFF}, {52, kImage, 0x82001BDC, 2, 16},
    {32, kWord, 0x0, 3, 4}, {56, kWord, 0x0}, {76, kImage, 0x82001BD8, 2, 4}, {72, kWord, 0xFFFFFFFF},
    {88, kImage, 0x82001BD8}, {84, kWord, 0xFFFFFFFF}, {92, kImage, 0x82001BD8}, {96, kWord, 0x0},
    {156, kImage, 0x82000A24}, {152, kWord, 0xFFFFFFFF}, {160, kImage, 0x82000A24}, {100, kWord, 0x0},
    {164, kImage, 0x82000A24}, {104, kWord, 0x0}, {168, kImage, 0x82000A24}, {108, kWord, 0x0},
    {172, kImage, 0x82000A24}, {112, kWord, 0x0}, {176, kImage, 0x82000A24}, {116, kWord, 0x0},
    {180, kImage, 0x82000A24}, {128, kImage, 0x82000A24}, {120, kWord, 0x0, 2, 24}, {136, kWord, 0xB},
    {132, kImage, 0x82000A24}, {124, kWord, 0x0, 2, 24}, {140, kWord, 0xB},
};

// Pool4Object::Reset (sub_820D65F8).
// 1 stores
constexpr Fill kPool4Object_Reset0[] = {
    {24, kWord, 0x0},
};
// 6 stores
constexpr Fill kPool4Object_Reset1[] = {
    {80, kWord, 0x0}, {84, kWord, 0xFFFFFFFF}, {88, kWord, 0x0, 2, 8}, {92, kWord, 0x0, 2, 8},
};
// 9 stores
constexpr Fill kPool4Object_Reset2[] = {
    {556, kWord, 0x0, 5, 4}, {544, kWord, 0x0}, {548, kWord, 0xFFFFFFFF}, {552, kWord, 0x0},
    {576, kImage, 0x826B2B20},
};

bool On() { return REXCVAR_GET(native_scene_loaders); }

using guest::LoadU32;

}  // namespace

// CarModelLoads::CarModelLoads.
extern "C" REX_FUNC(sub_820D3A80) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3A80(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 32;
  sub_8257EEF0(ctx, base);
  guest_memory::Apply(base, object, kCarModelLoads_CarModelLoads0, args);
  ctx.r3.u64 = object + 320;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 340;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object;
}

// CarModelLoads::~CarModelLoads.
extern "C" REX_FUNC(sub_820D3AE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3AE0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 340;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 320;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 296;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 272;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 32;
  sub_8257F180(ctx, base);
}

// CockpitLoads::CockpitLoads.
extern "C" REX_FUNC(sub_820D5CB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D5CB8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 32;
  sub_8257EEF0(ctx, base);
  guest_memory::Apply(base, object, kCockpitLoads_CockpitLoads0, args);
  ctx.r3.u64 = object + 320;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 340;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 512;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 976;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 1904;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 2368;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 2832;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object;
}

// CockpitLoads::~CockpitLoads.
extern "C" REX_FUNC(sub_820D5D38) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D5D38(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 2832;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 2368;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1904;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1440;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 976;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 512;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 340;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 320;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 296;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 272;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 32;
  sub_8257F180(ctx, base);
}

// TrafficLoads::TrafficLoads.
extern "C" REX_FUNC(sub_820D78F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D78F0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 48;
  sub_8257EEF0(ctx, base);
  guest_memory::Apply(base, object, kTrafficLoads_TrafficLoads0, args);
  ctx.r3.u64 = object + 336;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 356;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 528;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 992;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 1456;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 1920;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 2384;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 2848;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object;
}

// TrafficLoads::~TrafficLoads.
extern "C" REX_FUNC(sub_820D7970) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D7970(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 2848;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 2384;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1920;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1456;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 992;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 528;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 356;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 336;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 312;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 288;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 48;
  sub_8257F180(ctx, base);
}

// WheelsLoads::WheelsLoads.
extern "C" REX_FUNC(sub_820D8C28) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D8C28(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 48;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 512;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 1024;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 1488;
  sub_820CE6E8(ctx, base);
  guest_memory::Apply(base, object, kWheelsLoads_WheelsLoads0, args);
  ctx.r3.u64 = object + 2048;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 2068;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 2144;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 2608;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object;
}

// WheelsLoads::~WheelsLoads.
extern "C" REX_FUNC(sub_820D8CD8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D8CD8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 2608;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 2144;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 2068;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 2048;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 2024;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 2000;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 1488;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1024;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 512;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 48;
  sub_820CE750(ctx, base);
}

// CarPartsLoads::CarPartsLoads.
extern "C" REX_FUNC(sub_820D6C38) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D6C38(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 5184;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 5648;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 6112;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 6576;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 7040;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 7504;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 7968;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 8432;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 8896;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 9360;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 9824;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 10288;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 10752;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 11216;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 11680;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 12144;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 12608;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 13072;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 13536;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 14000;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 14464;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 14928;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 15392;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 15856;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 16320;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 16784;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 17248;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 17712;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 18176;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 18640;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 19104;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 19568;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 20032;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 20496;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 20960;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 21424;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 21888;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 22352;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 22816;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 23280;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 23744;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 24208;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 24672;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 25136;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 25600;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 26064;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 26528;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 26992;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 27456;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 27920;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 28384;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 28848;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 29312;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 29776;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 30240;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 30704;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 31168;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 31632;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 32096;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 32560;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 33024;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 33488;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 33952;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 34416;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 34880;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 35344;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 35808;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 36272;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 36736;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 37200;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 37664;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 38128;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 38592;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 39056;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 39520;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 39984;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 40448;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 40912;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 41376;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 41840;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 42304;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 42768;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 43232;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 43696;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 44160;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 44624;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 45088;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 45552;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 46016;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 46480;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 46944;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 47408;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 47872;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 48336;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 48800;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 49264;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 49728;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 50192;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 50656;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 51120;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 51584;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 52048;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 52512;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 52976;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 53440;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 53904;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 54368;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 54832;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 55296;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 55760;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 56224;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 56688;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 57152;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 57616;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 58080;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 58544;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 59008;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 59472;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 59936;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 60400;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 60864;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 61328;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 61792;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 62256;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 62720;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 63184;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 63648;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 64112;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 64576;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 65040;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 65504;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 65968;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 66432;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 66896;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 67360;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 67824;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 68288;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 68752;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 69216;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 69680;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 70144;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 70608;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 71072;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 71536;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 72000;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 72464;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 72928;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 73392;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 73856;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 74320;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 74784;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 75248;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 75712;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 76176;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 76640;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 77104;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 77568;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 78032;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 78496;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 78960;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 79424;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 79888;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 80352;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 80816;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 81280;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 81744;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 82208;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 82672;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 83136;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 83600;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 84064;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 84528;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 84992;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 85456;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 85920;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 86384;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 86848;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 87312;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 87776;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 88240;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 88704;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 89168;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 89632;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 90096;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 90560;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 91024;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 91488;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 91952;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 92416;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 92880;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 93344;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 93808;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 94272;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 94736;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 95200;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 95664;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 96128;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 96592;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 97056;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 97520;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 97984;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 98448;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 98912;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 99376;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 99840;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 100304;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 100768;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 101232;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 101696;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 102160;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 102624;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 103088;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 103552;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 104016;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 104480;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 104944;
  sub_820CE6E8(ctx, base);
  guest_memory::Apply(base, object, kCarPartsLoads_CarPartsLoads0, args);
  ctx.r3.u64 = object;
}

// CarParts::Release.
extern "C" REX_FUNC(sub_820D69B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D69B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 96;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 5280;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release0, args);
  ctx.r3.u64 = object + 120;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 5744;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release1, args);
  ctx.r3.u64 = object + 144;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 6208;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release2, args);
  ctx.r3.u64 = object + 168;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 6672;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release3, args);
  ctx.r3.u64 = object + 192;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 7136;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release4, args);
  ctx.r3.u64 = object + 216;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 7600;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release5, args);
  ctx.r3.u64 = object + 240;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 8064;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release6, args);
  ctx.r3.u64 = object + 264;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 8528;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release7, args);
  ctx.r3.u64 = object + 288;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 8992;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release8, args);
  ctx.r3.u64 = object + 312;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 9456;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release9, args);
  ctx.r3.u64 = object + 336;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 9920;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release10, args);
  ctx.r3.u64 = object + 360;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 10384;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release11, args);
  ctx.r3.u64 = object + 384;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 10848;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release12, args);
  ctx.r3.u64 = object + 408;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 11312;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release13, args);
  ctx.r3.u64 = object + 432;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 11776;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release14, args);
  ctx.r3.u64 = object + 456;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 12240;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release15, args);
  ctx.r3.u64 = object + 480;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 12704;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release16, args);
  ctx.r3.u64 = object + 504;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 13168;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release17, args);
  ctx.r3.u64 = object + 528;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 13632;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release18, args);
  ctx.r3.u64 = object + 552;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 14096;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release19, args);
  ctx.r3.u64 = object + 576;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 14560;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release20, args);
  ctx.r3.u64 = object + 600;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 15024;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release21, args);
  ctx.r3.u64 = object + 624;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 15488;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release22, args);
  ctx.r3.u64 = object + 648;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 15952;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release23, args);
  ctx.r3.u64 = object + 672;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 16416;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release24, args);
  ctx.r3.u64 = object + 696;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 16880;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release25, args);
  ctx.r3.u64 = object + 720;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 17344;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release26, args);
  ctx.r3.u64 = object + 744;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 17808;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release27, args);
  ctx.r3.u64 = object + 768;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 18272;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release28, args);
  ctx.r3.u64 = object + 792;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 18736;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release29, args);
  ctx.r3.u64 = object + 816;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 19200;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release30, args);
  ctx.r3.u64 = object + 840;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 19664;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release31, args);
  ctx.r3.u64 = object + 864;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 20128;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release32, args);
  ctx.r3.u64 = object + 888;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 20592;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release33, args);
  ctx.r3.u64 = object + 912;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 21056;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release34, args);
  ctx.r3.u64 = object + 936;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 21520;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release35, args);
  ctx.r3.u64 = object + 960;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 21984;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release36, args);
  ctx.r3.u64 = object + 984;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 22448;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release37, args);
  ctx.r3.u64 = object + 1008;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 22912;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release38, args);
  ctx.r3.u64 = object + 1032;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 23376;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release39, args);
  ctx.r3.u64 = object + 1056;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 23840;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release40, args);
  ctx.r3.u64 = object + 1080;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 24304;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release41, args);
  ctx.r3.u64 = object + 1104;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 24768;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release42, args);
  ctx.r3.u64 = object + 1128;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 25232;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release43, args);
  ctx.r3.u64 = object + 1152;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 25696;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release44, args);
  ctx.r3.u64 = object + 1176;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 26160;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release45, args);
  ctx.r3.u64 = object + 1200;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 26624;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release46, args);
  ctx.r3.u64 = object + 1224;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 27088;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release47, args);
  ctx.r3.u64 = object + 1248;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 27552;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release48, args);
  ctx.r3.u64 = object + 1272;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 28016;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release49, args);
  ctx.r3.u64 = object + 1296;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 28480;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release50, args);
  ctx.r3.u64 = object + 1320;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 28944;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release51, args);
  ctx.r3.u64 = object + 1344;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 29408;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release52, args);
  ctx.r3.u64 = object + 1368;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 29872;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release53, args);
  ctx.r3.u64 = object + 1392;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 30336;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release54, args);
  ctx.r3.u64 = object + 1416;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 30800;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release55, args);
  ctx.r3.u64 = object + 1440;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 31264;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release56, args);
  ctx.r3.u64 = object + 1464;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 31728;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release57, args);
  ctx.r3.u64 = object + 1488;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 32192;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release58, args);
  ctx.r3.u64 = object + 1512;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 32656;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release59, args);
  ctx.r3.u64 = object + 1536;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 33120;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release60, args);
  ctx.r3.u64 = object + 1560;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 33584;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release61, args);
  ctx.r3.u64 = object + 1584;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 34048;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release62, args);
  ctx.r3.u64 = object + 1608;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 34512;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release63, args);
  ctx.r3.u64 = object + 1632;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 34976;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release64, args);
  ctx.r3.u64 = object + 1656;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 35440;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release65, args);
  ctx.r3.u64 = object + 1680;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 35904;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release66, args);
  ctx.r3.u64 = object + 1704;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 36368;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release67, args);
  ctx.r3.u64 = object + 1728;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 36832;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release68, args);
  ctx.r3.u64 = object + 1752;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 37296;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release69, args);
  ctx.r3.u64 = object + 1776;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 37760;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release70, args);
  ctx.r3.u64 = object + 1800;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 38224;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release71, args);
  ctx.r3.u64 = object + 1824;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 38688;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release72, args);
  ctx.r3.u64 = object + 1848;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 39152;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release73, args);
  ctx.r3.u64 = object + 1872;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 39616;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release74, args);
  ctx.r3.u64 = object + 1896;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 40080;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release75, args);
  ctx.r3.u64 = object + 1920;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 40544;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release76, args);
  ctx.r3.u64 = object + 1944;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 41008;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release77, args);
  ctx.r3.u64 = object + 1968;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 41472;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release78, args);
  ctx.r3.u64 = object + 1992;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 41936;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release79, args);
  ctx.r3.u64 = object + 2016;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 42400;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release80, args);
  ctx.r3.u64 = object + 2040;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 42864;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release81, args);
  ctx.r3.u64 = object + 2064;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 43328;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release82, args);
  ctx.r3.u64 = object + 2088;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 43792;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release83, args);
  ctx.r3.u64 = object + 2112;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 44256;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release84, args);
  ctx.r3.u64 = object + 2136;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 44720;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release85, args);
  ctx.r3.u64 = object + 2160;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 45184;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release86, args);
  ctx.r3.u64 = object + 2184;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 45648;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release87, args);
  ctx.r3.u64 = object + 2208;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 46112;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release88, args);
  ctx.r3.u64 = object + 2232;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 46576;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release89, args);
  ctx.r3.u64 = object + 2256;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 47040;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release90, args);
  ctx.r3.u64 = object + 2280;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 47504;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release91, args);
  ctx.r3.u64 = object + 2304;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 47968;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release92, args);
  ctx.r3.u64 = object + 2328;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 48432;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release93, args);
  ctx.r3.u64 = object + 2352;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 48896;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release94, args);
  ctx.r3.u64 = object + 2376;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 49360;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release95, args);
  ctx.r3.u64 = object + 2400;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 49824;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release96, args);
  ctx.r3.u64 = object + 2424;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 50288;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release97, args);
  ctx.r3.u64 = object + 2448;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 50752;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release98, args);
  ctx.r3.u64 = object + 2472;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 51216;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release99, args);
  ctx.r3.u64 = object + 2496;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 51680;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release100, args);
  ctx.r3.u64 = object + 2520;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 52144;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release101, args);
  ctx.r3.u64 = object + 2544;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 52608;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release102, args);
  ctx.r3.u64 = object + 2568;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 53072;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release103, args);
  ctx.r3.u64 = object + 2592;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 53536;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release104, args);
  ctx.r3.u64 = object + 2616;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 54000;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release105, args);
  ctx.r3.u64 = object + 2640;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 54464;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release106, args);
  ctx.r3.u64 = object + 2664;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 54928;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release107, args);
  ctx.r3.u64 = object + 2688;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 55392;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release108, args);
  ctx.r3.u64 = object + 2712;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 55856;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release109, args);
  ctx.r3.u64 = object + 2736;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 56320;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release110, args);
  ctx.r3.u64 = object + 2760;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 56784;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release111, args);
  ctx.r3.u64 = object + 2784;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 57248;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release112, args);
  ctx.r3.u64 = object + 2808;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 57712;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release113, args);
  ctx.r3.u64 = object + 2832;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 58176;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release114, args);
  ctx.r3.u64 = object + 2856;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 58640;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release115, args);
  ctx.r3.u64 = object + 2880;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 59104;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release116, args);
  ctx.r3.u64 = object + 2904;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 59568;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release117, args);
  ctx.r3.u64 = object + 2928;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 60032;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release118, args);
  ctx.r3.u64 = object + 2952;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 60496;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release119, args);
  ctx.r3.u64 = object + 2976;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 60960;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release120, args);
  ctx.r3.u64 = object + 3000;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 61424;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release121, args);
  ctx.r3.u64 = object + 3024;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 61888;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release122, args);
  ctx.r3.u64 = object + 3048;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 62352;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release123, args);
  ctx.r3.u64 = object + 3072;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 62816;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release124, args);
  ctx.r3.u64 = object + 3096;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 63280;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release125, args);
  ctx.r3.u64 = object + 3120;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 63744;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release126, args);
  ctx.r3.u64 = object + 3144;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 64208;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release127, args);
  ctx.r3.u64 = object + 3168;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 64672;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release128, args);
  ctx.r3.u64 = object + 3192;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 65136;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release129, args);
  ctx.r3.u64 = object + 3216;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 65600;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release130, args);
  ctx.r3.u64 = object + 3240;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 66064;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release131, args);
  ctx.r3.u64 = object + 3264;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 66528;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release132, args);
  ctx.r3.u64 = object + 3288;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 66992;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release133, args);
  ctx.r3.u64 = object + 3312;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 67456;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release134, args);
  ctx.r3.u64 = object + 3336;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 67920;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release135, args);
  ctx.r3.u64 = object + 3360;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 68384;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release136, args);
  ctx.r3.u64 = object + 3384;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 68848;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release137, args);
  ctx.r3.u64 = object + 3408;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 69312;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release138, args);
  ctx.r3.u64 = object + 3432;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 69776;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release139, args);
  ctx.r3.u64 = object + 3456;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 70240;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release140, args);
  ctx.r3.u64 = object + 3480;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 70704;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release141, args);
  ctx.r3.u64 = object + 3504;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 71168;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release142, args);
  ctx.r3.u64 = object + 3528;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 71632;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release143, args);
  ctx.r3.u64 = object + 3552;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 72096;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release144, args);
  ctx.r3.u64 = object + 3576;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 72560;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release145, args);
  ctx.r3.u64 = object + 3600;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 73024;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release146, args);
  ctx.r3.u64 = object + 3624;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 73488;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release147, args);
  ctx.r3.u64 = object + 3648;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 73952;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release148, args);
  ctx.r3.u64 = object + 3672;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 74416;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release149, args);
  ctx.r3.u64 = object + 3696;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 74880;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release150, args);
  ctx.r3.u64 = object + 3720;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 75344;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release151, args);
  ctx.r3.u64 = object + 3744;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 75808;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release152, args);
  ctx.r3.u64 = object + 3768;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 76272;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release153, args);
  ctx.r3.u64 = object + 3792;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 76736;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release154, args);
  ctx.r3.u64 = object + 3816;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 77200;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release155, args);
  ctx.r3.u64 = object + 3840;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 77664;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release156, args);
  ctx.r3.u64 = object + 3864;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 78128;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release157, args);
  ctx.r3.u64 = object + 3888;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 78592;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release158, args);
  ctx.r3.u64 = object + 3912;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 79056;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release159, args);
  ctx.r3.u64 = object + 3936;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 79520;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release160, args);
  ctx.r3.u64 = object + 3960;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 79984;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release161, args);
  ctx.r3.u64 = object + 3984;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 80448;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release162, args);
  ctx.r3.u64 = object + 4008;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 80912;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release163, args);
  ctx.r3.u64 = object + 4032;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 81376;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release164, args);
  ctx.r3.u64 = object + 4056;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 81840;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release165, args);
  ctx.r3.u64 = object + 4080;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 82304;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release166, args);
  ctx.r3.u64 = object + 4104;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 82768;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release167, args);
  ctx.r3.u64 = object + 4128;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 83232;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release168, args);
  ctx.r3.u64 = object + 4152;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 83696;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release169, args);
  ctx.r3.u64 = object + 4176;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 84160;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release170, args);
  ctx.r3.u64 = object + 4200;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 84624;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release171, args);
  ctx.r3.u64 = object + 4224;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 85088;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release172, args);
  ctx.r3.u64 = object + 4248;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 85552;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release173, args);
  ctx.r3.u64 = object + 4272;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 86016;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release174, args);
  ctx.r3.u64 = object + 4296;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 86480;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release175, args);
  ctx.r3.u64 = object + 4320;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 86944;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release176, args);
  ctx.r3.u64 = object + 4344;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 87408;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release177, args);
  ctx.r3.u64 = object + 4368;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 87872;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release178, args);
  ctx.r3.u64 = object + 4392;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 88336;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release179, args);
  ctx.r3.u64 = object + 4416;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 88800;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release180, args);
  ctx.r3.u64 = object + 4440;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 89264;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release181, args);
  ctx.r3.u64 = object + 4464;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 89728;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release182, args);
  ctx.r3.u64 = object + 4488;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 90192;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release183, args);
  ctx.r3.u64 = object + 4512;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 90656;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release184, args);
  ctx.r3.u64 = object + 4536;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 91120;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release185, args);
  ctx.r3.u64 = object + 4560;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 91584;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release186, args);
  ctx.r3.u64 = object + 4584;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 92048;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release187, args);
  ctx.r3.u64 = object + 4608;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 92512;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release188, args);
  ctx.r3.u64 = object + 4632;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 92976;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release189, args);
  ctx.r3.u64 = object + 4656;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 93440;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release190, args);
  ctx.r3.u64 = object + 4680;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 93904;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release191, args);
  ctx.r3.u64 = object + 4704;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 94368;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release192, args);
  ctx.r3.u64 = object + 4728;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 94832;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release193, args);
  ctx.r3.u64 = object + 4752;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 95296;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release194, args);
  ctx.r3.u64 = object + 4776;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 95760;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release195, args);
  ctx.r3.u64 = object + 4800;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 96224;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release196, args);
  ctx.r3.u64 = object + 4824;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 96688;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release197, args);
  ctx.r3.u64 = object + 4848;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 97152;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release198, args);
  ctx.r3.u64 = object + 4872;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 97616;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release199, args);
  ctx.r3.u64 = object + 4896;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 98080;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release200, args);
  ctx.r3.u64 = object + 4920;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 98544;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release201, args);
  ctx.r3.u64 = object + 4944;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 99008;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release202, args);
  ctx.r3.u64 = object + 4968;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 99472;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release203, args);
  ctx.r3.u64 = object + 4992;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 99936;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release204, args);
  ctx.r3.u64 = object + 5016;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 100400;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release205, args);
  ctx.r3.u64 = object + 5040;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 100864;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release206, args);
  ctx.r3.u64 = object + 5064;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 101328;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release207, args);
  ctx.r3.u64 = object + 5088;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 101792;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release208, args);
  ctx.r3.u64 = object + 5112;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 102256;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release209, args);
  ctx.r3.u64 = object + 5136;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 102720;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release210, args);
  ctx.r3.u64 = object + 5160;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 103184;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release211, args);
  ctx.r3.u64 = object + 5184;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 103648;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release212, args);
  ctx.r3.u64 = object + 5208;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 104112;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release213, args);
  ctx.r3.u64 = object + 5232;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 104576;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release214, args);
  ctx.r3.u64 = object + 5256;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 105040;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kCarParts_Release215, args);
  ctx.r3.u64 = object + 105040;
}

// CarPartsLoads::~CarPartsLoads.
extern "C" REX_FUNC(sub_820D6C98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D6C98(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 104944;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 104480;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 104016;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 103552;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 103088;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 102624;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 102160;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 101696;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 101232;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 100768;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 100304;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 99840;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 99376;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 98912;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 98448;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 97984;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 97520;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 97056;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 96592;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 96128;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 95664;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 95200;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 94736;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 94272;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 93808;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 93344;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 92880;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 92416;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 91952;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 91488;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 91024;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 90560;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 90096;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 89632;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 89168;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 88704;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 88240;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 87776;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 87312;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 86848;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 86384;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 85920;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 85456;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 84992;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 84528;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 84064;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 83600;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 83136;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 82672;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 82208;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 81744;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 81280;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 80816;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 80352;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 79888;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 79424;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 78960;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 78496;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 78032;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 77568;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 77104;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 76640;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 76176;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 75712;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 75248;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 74784;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 74320;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 73856;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 73392;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 72928;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 72464;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 72000;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 71536;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 71072;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 70608;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 70144;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 69680;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 69216;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 68752;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 68288;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 67824;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 67360;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 66896;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 66432;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 65968;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 65504;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 65040;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 64576;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 64112;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 63648;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 63184;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 62720;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 62256;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 61792;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 61328;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 60864;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 60400;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 59936;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 59472;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 59008;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 58544;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 58080;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 57616;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 57152;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 56688;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 56224;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 55760;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 55296;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 54832;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 54368;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 53904;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 53440;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 52976;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 52512;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 52048;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 51584;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 51120;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 50656;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 50192;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 49728;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 49264;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 48800;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 48336;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 47872;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 47408;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 46944;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 46480;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 46016;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 45552;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 45088;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 44624;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 44160;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 43696;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 43232;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 42768;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 42304;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 41840;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 41376;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 40912;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 40448;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 39984;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 39520;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 39056;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 38592;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 38128;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 37664;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 37200;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 36736;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 36272;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 35808;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 35344;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 34880;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 34416;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 33952;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 33488;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 33024;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 32560;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 32096;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 31632;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 31168;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 30704;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 30240;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 29776;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 29312;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 28848;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 28384;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 27920;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 27456;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 26992;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 26528;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 26064;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 25600;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 25136;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 24672;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 24208;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 23744;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 23280;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 22816;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 22352;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 21888;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 21424;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 20960;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 20496;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 20032;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 19568;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 19104;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 18640;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 18176;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 17712;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 17248;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 16784;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 16320;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 15856;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 15392;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 14928;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 14464;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 14000;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 13536;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 13072;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 12608;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 12144;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 11680;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 11216;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 10752;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 10288;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 9824;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 9360;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 8896;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 8432;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 7968;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 7504;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 7040;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 6576;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 6112;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 5648;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 5184;
  sub_820CE750(ctx, base);
}

// Wheels::Reset.
extern "C" REX_FUNC(sub_820D8810) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D8810(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kWheels_Reset0, args);
  ctx.r3.u64 = object + 192;
  ctx.r8.u64 = 0x0u;
  sub_820D8760(ctx, base);
}

// Pool4Object::Reset.
extern "C" REX_FUNC(sub_820D65F8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D65F8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPool4Object_Reset0, args);
  ctx.r3.u64 = object + 28;
  sub_820D3128(ctx, base);
  guest_memory::Apply(base, object, kPool4Object_Reset1, args);
  ctx.r3.u64 = object + 112;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A270(ctx, base);
  guest_memory::Apply(base, object, kPool4Object_Reset2, args);
  ctx.r3.u64 = object + 112;
}

// Pool4Object::Release.
extern "C" REX_FUNC(sub_820D66D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D66D0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 80;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 112;
  sub_8257A270(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820D65F8(ctx, base);
}

// Wheels::Release.
extern "C" REX_FUNC(sub_820D8B30) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D8B30(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 192;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 1168;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 2144;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 2280;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 216;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 1192;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 2168;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 2304;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820D8810(ctx, base);
}

// Pool4Object::SetRecord.
extern "C" REX_FUNC(sub_820D6710) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D6710(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 28;
  ctx.r5.u64 = 0x34u;
  sub_8258CB90(ctx, base);
}


// CarModel::IsLoaded: 0 while either of its two loads (+3972, +3996) is
// 2; else 1 unless its own state (+3748) is 2.
extern "C" REX_FUNC(sub_820D38C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D38C8(ctx, base);
    return;
  }
  const uint32_t model = ctx.r3.u32;
  for (uint32_t i = 0; i < 2; i++) {
    if (int32_t(LoadU32(base, model + 3972 + 24 * i)) == 2) {
      ctx.r3.u64 = 0;
      return;
    }
  }
  ctx.r3.u64 = LoadU32(base, model + 3748) != 2 ? 1 : 0;
}

// Pool4Object::IsLoaded: 1 unless +100 is 2.
extern "C" REX_FUNC(sub_820D66B8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D66B8(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, ctx.r3.u32 + 100) != 2 ? 1 : 0;
}

// XPR::Count: +20, or 0 without a header (+16).
extern "C" REX_FUNC(sub_8257C838) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8257C838(ctx, base);
    return;
  }
  const uint32_t xpr = ctx.r3.u32;
  ctx.r3.u64 = LoadU32(base, xpr + 16) != 0 ? LoadU32(base, xpr + 20) : 0;
}
