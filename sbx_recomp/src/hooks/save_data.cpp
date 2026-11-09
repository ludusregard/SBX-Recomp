// The save data's defaults as our own C++ (--native_save_data, on by
// default): the stats' and progress' resets, the options' and control
// maps' defaults, the course records, a save slot's create and delete,
// the players' controllers, the menu lists' set-up and the title screen's
// player reset.
//
// Written from SBX's generated code. The resets are long runs of stores,
// written as tables here in the originals' order (the floats are the
// image's constants, copied by their bits); memset to 0 is done here.
// Progress::SetDefaults (sub_82113FB0) is unlock_cut_cars' override and is
// called, not replaced; the course record's own reset (sub_82105E20), the
// heap and VirtualAlloc stay recompiled.
//
// A save (129,200 bytes, from operator new): +48 the stats, +119908 the
// course records (22 of 408 bytes after a 20-byte header), +128904 and
// +128916 the options.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_save_data, true, "SBX",
                    "Our own C++ for the save data's defaults (stats, progress, options, "
                    "course records, a slot's create and delete, the menu lists' set-up) "
                    "instead of the recompiled functions; the heap and the progress defaults "
                    "stay recompiled");

namespace {

using guest::LoadU32;
using guest::StoreU32;
using guest_memory::Fill;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kWord;

constexpr uint32_t kSaves = 0x826B5838;         // 4 bytes a slot
constexpr uint32_t kControllers = 0x826AA884;   // the players' four controllers
constexpr uint32_t kControllersSet = 0x826B73F8;
constexpr uint32_t kMenuList = 0x8280DE40;      // [it] the title screen's menu list
constexpr uint32_t kTitleState = 0x8280DE44;    // [it] +36 cleared by ResetPlayer
constexpr uint32_t kPlayerFlags = 0x826AA820;   // four words set to 1
constexpr uint32_t kSaveBytes = 129200;

// The resets, in the originals' order.
constexpr Fill kStats0[] = {
    {4892, kImage, 0x82000A24}, {4896, kWord, 0x0}, {4960, kImage, 0x82000A24}, {4964, kWord, 0x0},
    {5008, kImage, 0x82000A24}, {5012, kWord, 0x0}, {5056, kImage, 0x82000A24}, {5060, kWord, 0x0},
    {5104, kImage, 0x82000A24}, {5108, kWord, 0x0}, {5152, kImage, 0x82000A24}, {5156, kWord, 0x0},
    {5200, kImage, 0x82000A24}, {5204, kWord, 0x0}, {5248, kImage, 0x82000A24}, {5252, kWord, 0x0},
    {5296, kImage, 0x82000A24}, {5300, kWord, 0x0}, {5344, kImage, 0x82000A24}, {5348, kWord, 0x0},
    {5392, kImage, 0x82000A24}, {5396, kWord, 0x0}, {5440, kImage, 0x82000A24}, {5444, kWord, 0x0},
    {5488, kImage, 0x82000A24}, {5492, kWord, 0x0}, {5536, kImage, 0x82000A24}, {5540, kWord, 0x0},
    {5584, kImage, 0x82000A24}, {5588, kWord, 0x0}, {5632, kImage, 0x82000A24}, {5636, kWord, 0x0},
    {5880, kImage, 0x82000A24}, {5884, kWord, 0x0}, {5888, kImage, 0x82000A24}, {5892, kWord, 0x0},
    {5896, kImage, 0x82000A24}, {5900, kWord, 0x0}, {5904, kImage, 0x82000A24}, {5908, kWord, 0x0},
    {5912, kImage, 0x82000A24}, {5916, kWord, 0x0}, {5920, kImage, 0x82000A24}, {5924, kWord, 0x0},
    {5928, kImage, 0x82000A24}, {5932, kWord, 0x0}, {5936, kImage, 0x82000A24}, {5940, kWord, 0x0},
    {5944, kImage, 0x82000A24}, {5948, kWord, 0x0},
};
constexpr Fill kStatsResetRecords0[] = {
    {4, kWord, 0x0, 2, 4}, {12, kImage, 0x82000A24}, {16, kWord, 0x0, 2, 4}, {24, kWord, 0xFFFFFFFF, 3, 4},
    {36, kWord, 0x0, 2, 4}, {48, kWord, 0x0, 3, 4}, {60, kImage, 0x82000A24}, {64, kWord, 0x0, 2, 4},
    {72, kWord, 0xFFFFFFFF, 3, 4}, {84, kWord, 0x0, 2, 4}, {96, kWord, 0x0, 3, 4}, {108, kImage, 0x82000A24},
    {112, kWord, 0x0, 2, 4}, {120, kWord, 0xFFFFFFFF, 3, 4}, {132, kWord, 0x0, 2, 4}, {144, kWord, 0x0, 3, 4},
    {156, kImage, 0x82000A24}, {160, kWord, 0x0, 2, 4}, {168, kWord, 0xFFFFFFFF, 3, 4}, {180, kWord, 0x0, 2, 4},
    {192, kWord, 0x0, 3, 4}, {204, kImage, 0x82000A24}, {208, kWord, 0x0, 2, 4}, {216, kWord, 0xFFFFFFFF, 3, 4},
    {228, kWord, 0x0, 2, 4}, {240, kWord, 0x0, 3, 4}, {252, kImage, 0x82000A24}, {256, kWord, 0x0, 2, 4},
    {264, kWord, 0xFFFFFFFF, 3, 4}, {276, kWord, 0x0, 2, 4}, {288, kWord, 0x0, 3, 4}, {300, kImage, 0x82000A24},
    {304, kWord, 0x0, 2, 4}, {312, kWord, 0xFFFFFFFF, 3, 4}, {324, kWord, 0x0, 2, 4}, {336, kWord, 0x0, 3, 4},
    {348, kImage, 0x82000A24}, {352, kWord, 0x0, 2, 4}, {360, kWord, 0xFFFFFFFF, 3, 4}, {372, kWord, 0x0, 2, 4},
    {384, kWord, 0x0, 3, 4}, {396, kImage, 0x82000A24}, {400, kWord, 0x0, 2, 4}, {408, kWord, 0xFFFFFFFF, 3, 4},
    {420, kWord, 0x0, 2, 4}, {432, kWord, 0x0, 3, 4}, {444, kImage, 0x82000A24}, {448, kWord, 0x0, 2, 4},
    {456, kWord, 0xFFFFFFFF, 3, 4}, {468, kWord, 0x0, 2, 4}, {480, kWord, 0x0, 3, 4}, {492, kImage, 0x82000A24},
    {496, kWord, 0x0, 2, 4}, {504, kWord, 0xFFFFFFFF, 3, 4}, {516, kWord, 0x0, 2, 4}, {528, kWord, 0x0, 3, 4},
    {540, kImage, 0x82000A24}, {544, kWord, 0x0, 2, 4}, {552, kWord, 0xFFFFFFFF, 3, 4}, {564, kWord, 0x0, 2, 4},
    {576, kWord, 0x0, 3, 4}, {588, kImage, 0x82000A24}, {592, kWord, 0x0, 2, 4}, {600, kWord, 0xFFFFFFFF, 3, 4},
    {612, kWord, 0x0, 2, 4}, {624, kWord, 0x0, 3, 4}, {636, kImage, 0x82000A24}, {640, kWord, 0x0, 2, 4},
    {648, kWord, 0xFFFFFFFF, 3, 4}, {660, kWord, 0x0, 2, 4}, {672, kWord, 0x0, 3, 4}, {684, kImage, 0x82000A24},
    {688, kWord, 0x0, 2, 4}, {696, kWord, 0xFFFFFFFF, 3, 4}, {708, kWord, 0x0, 2, 4}, {720, kWord, 0x0},
    {0, kWord, 0x0},
};
constexpr Fill kStatsReset0[] = {
    {4860, kImage, 0x82000A24}, {4854, kHalf, 0x0}, {4852, kHalf, 0x0}, {4856, kWord, 0x0},
};
constexpr Fill kStatsReset1[] = {
    {4888, kImage, 0x82000A24, 2, 4}, {4896, kWord, 0x0, 2, 4}, {4912, kWord, 0x0}, {4908, kWord, 0x0},
    {4916, kHalf, 0x0, 4, 2},
};
constexpr Fill kStatsReset2[] = {
    {6032, kWord, 0x0}, {6036, kByte, 0x0, 3, 1}, {5680, kImage, 0x82000A24}, {5676, kWord, 0x0, 2, 8},
    {5692, kImage, 0x82000A24}, {5688, kWord, 0x0, 2, 8}, {5704, kImage, 0x82000A24}, {5700, kWord, 0x0, 2, 8},
    {5716, kImage, 0x82000A24}, {5712, kWord, 0x0, 2, 8}, {5728, kImage, 0x82000A24}, {5724, kWord, 0x0, 2, 8},
    {5740, kImage, 0x82000A24}, {5736, kWord, 0x0, 2, 8}, {5752, kImage, 0x82000A24}, {5748, kWord, 0x0, 2, 8},
    {5764, kImage, 0x82000A24}, {5760, kWord, 0x0, 2, 8}, {5776, kImage, 0x82000A24}, {5772, kWord, 0x0, 2, 8},
    {5788, kImage, 0x82000A24}, {5784, kWord, 0x0, 2, 8}, {5672, kWord, 0x0}, {4952, kWord, 0x0, 2, 4},
    {4960, kImage, 0x82000A24}, {4964, kWord, 0x0, 2, 4}, {4972, kWord, 0xFFFFFFFF, 3, 4}, {4984, kWord, 0x0, 2, 4},
    {4996, kWord, 0x0, 3, 4}, {5008, kImage, 0x82000A24}, {5012, kWord, 0x0, 2, 4}, {5020, kWord, 0xFFFFFFFF, 3, 4},
    {5032, kWord, 0x0, 2, 4}, {5044, kWord, 0x0, 3, 4}, {5056, kImage, 0x82000A24}, {5060, kWord, 0x0, 2, 4},
    {5068, kWord, 0xFFFFFFFF, 3, 4}, {5080, kWord, 0x0, 2, 4}, {5092, kWord, 0x0, 3, 4}, {5104, kImage, 0x82000A24},
    {5108, kWord, 0x0, 2, 4}, {5116, kWord, 0xFFFFFFFF, 3, 4}, {5128, kWord, 0x0, 2, 4}, {5140, kWord, 0x0, 3, 4},
    {5152, kImage, 0x82000A24}, {5156, kWord, 0x0, 2, 4}, {5164, kWord, 0xFFFFFFFF, 3, 4}, {5176, kWord, 0x0, 2, 4},
    {5188, kWord, 0x0, 3, 4}, {5200, kImage, 0x82000A24}, {5204, kWord, 0x0, 2, 4}, {5212, kWord, 0xFFFFFFFF, 3, 4},
    {5224, kWord, 0x0, 2, 4}, {5236, kWord, 0x0, 3, 4}, {5248, kImage, 0x82000A24}, {5252, kWord, 0x0, 2, 4},
    {5260, kWord, 0xFFFFFFFF, 3, 4}, {5272, kWord, 0x0, 2, 4}, {5284, kWord, 0x0, 3, 4}, {5296, kImage, 0x82000A24},
    {5300, kWord, 0x0, 2, 4}, {5308, kWord, 0xFFFFFFFF, 3, 4}, {5320, kWord, 0x0, 2, 4}, {5332, kWord, 0x0, 3, 4},
    {5344, kImage, 0x82000A24}, {5348, kWord, 0x0, 2, 4}, {5356, kWord, 0xFFFFFFFF, 3, 4}, {5368, kWord, 0x0, 2, 4},
    {5380, kWord, 0x0, 3, 4}, {5392, kImage, 0x82000A24}, {5396, kWord, 0x0, 2, 4}, {5404, kWord, 0xFFFFFFFF, 3, 4},
    {5416, kWord, 0x0, 2, 4}, {5428, kWord, 0x0, 3, 4}, {5440, kImage, 0x82000A24}, {5444, kWord, 0x0, 2, 4},
    {5452, kWord, 0xFFFFFFFF, 3, 4}, {5464, kWord, 0x0, 2, 4}, {5476, kWord, 0x0, 3, 4}, {5488, kImage, 0x82000A24},
    {5492, kWord, 0x0, 2, 4}, {5500, kWord, 0xFFFFFFFF, 3, 4}, {5512, kWord, 0x0, 2, 4}, {5524, kWord, 0x0, 3, 4},
    {5536, kImage, 0x82000A24}, {5540, kWord, 0x0, 2, 4}, {5548, kWord, 0xFFFFFFFF, 3, 4}, {5560, kWord, 0x0, 2, 4},
    {5572, kWord, 0x0, 3, 4}, {5584, kImage, 0x82000A24}, {5588, kWord, 0x0, 2, 4}, {5596, kWord, 0xFFFFFFFF, 3, 4},
    {5608, kWord, 0x0, 2, 4}, {5620, kWord, 0x0, 3, 4}, {5632, kImage, 0x82000A24}, {5636, kWord, 0x0, 2, 4},
    {5644, kWord, 0xFFFFFFFF, 3, 4}, {5656, kWord, 0x0, 2, 4}, {5668, kWord, 0x0}, {4948, kWord, 0x0},
    {6008, kWord, 0x3}, {5796, kHalf, 0x0, 41, 2}, {5956, kWord, 0x1},
};
constexpr Fill kProgressClear0[] = {
    {0, kWord, 0x0, 2, 4}, {8, kByte, 0x0}, {12, kWord, 0x6}, {16, kHalf, 0x0},
    {18, kByte, 0x1}, {20, kWord, 0x6}, {24, kHalf, 0x0}, {26, kByte, 0x1},
    {28, kByte, 0x0}, {32, kWord, 0x6}, {36, kHalf, 0x0}, {38, kByte, 0x1},
    {40, kWord, 0x6}, {44, kHalf, 0x0}, {46, kByte, 0x1},
};
constexpr Fill kProgressClear1[] = {
    {6092, kWord, 0x0, 2, 4}, {6108, kWord, 0xE}, {6112, kWord, 0x30}, {109652, kWord, 0x4},
    {109648, kWord, 0x0}, {6100, kWord, 0x0}, {6116, kWord, 0xBB8}, {6122, kHalf, 0x0},
    {6120, kHalf, 0x1, 2, 6}, {6124, kHalf, 0x2, 2, 6}, {6128, kHalf, 0x3, 2, 6}, {6132, kHalf, 0x4, 2, 6},
    {6136, kHalf, 0x5, 2, 6}, {6140, kHalf, 0x6, 2, 6}, {6144, kHalf, 0x7, 2, 6}, {6148, kHalf, 0x8, 2, 6},
    {6152, kHalf, 0x9, 2, 6}, {6156, kHalf, 0xA, 2, 6}, {6160, kHalf, 0xB, 2, 6}, {6164, kHalf, 0xC, 2, 6},
    {6168, kHalf, 0xD, 2, 6}, {6172, kHalf, 0xE, 2, 6}, {6176, kHalf, 0xF, 2, 6}, {6180, kHalf, 0x10, 2, 6},
    {6184, kHalf, 0x11, 2, 6}, {6188, kHalf, 0x12, 2, 6}, {6192, kHalf, 0x13, 2, 6}, {6196, kHalf, 0x14, 2, 6},
    {6200, kHalf, 0x15, 2, 6}, {6204, kHalf, 0x16, 2, 6}, {6208, kHalf, 0x17, 2, 6}, {6212, kHalf, 0x18, 2, 6},
    {6216, kHalf, 0x19, 2, 6}, {6220, kHalf, 0x1A, 2, 6}, {6224, kHalf, 0x1B, 2, 6}, {6228, kHalf, 0x1C, 2, 6},
    {6232, kHalf, 0x1D, 2, 6}, {6236, kHalf, 0x1E, 2, 6}, {6240, kHalf, 0x1F, 2, 6}, {6244, kHalf, 0x20, 2, 6},
    {6248, kHalf, 0x21, 2, 6}, {6252, kHalf, 0x22, 2, 6}, {6256, kHalf, 0x23, 2, 6}, {6260, kHalf, 0x24, 2, 6},
    {6264, kHalf, 0x25, 2, 6}, {6268, kHalf, 0x26, 2, 6}, {6272, kHalf, 0x27, 2, 6}, {6276, kHalf, 0x28, 2, 6},
    {6280, kHalf, 0x29, 2, 6}, {6284, kHalf, 0x2A, 2, 6}, {6288, kHalf, 0x2B, 2, 6}, {6292, kHalf, 0x2C, 2, 6},
    {6296, kHalf, 0x2D, 2, 6}, {6300, kHalf, 0x2E, 2, 6}, {6304, kHalf, 0x2F, 2, 6}, {6308, kHalf, 0x30, 2, 6},
    {6312, kHalf, 0x31, 2, 6}, {6316, kHalf, 0x32}, {6320, kHalf, 0x0, 2, 8}, {6316, kHalf, 0xFFFF, 2, 8},
    {6326, kHalf, 0xFFFF}, {6322, kHalf, 0xFFFF}, {104532, kWord, 0x32}, {104536, kWord, 0x0, 19, 4},
    {104672, kWord, 0x0, 16, 4}, {104760, kWord, 0x0}, {104758, kHalf, 0x0}, {104756, kHalf, 0x0},
    {104754, kHalf, 0x0}, {104752, kHalf, 0x0}, {104772, kWord, 0x0}, {104770, kHalf, 0x0},
    {104768, kHalf, 0x0}, {104766, kHalf, 0x0}, {104764, kHalf, 0x0}, {104784, kWord, 0x0},
    {104782, kHalf, 0x0}, {104780, kHalf, 0x0}, {104778, kHalf, 0x0}, {104776, kHalf, 0x0},
    {104796, kWord, 0x0}, {104794, kHalf, 0x0}, {104792, kHalf, 0x0}, {104790, kHalf, 0x0},
    {104788, kHalf, 0x0}, {104808, kWord, 0x0}, {104806, kHalf, 0x0}, {104804, kHalf, 0x0},
    {104802, kHalf, 0x0}, {104800, kHalf, 0x0}, {104820, kWord, 0x0}, {104818, kHalf, 0x0},
    {104816, kHalf, 0x0}, {104814, kHalf, 0x0}, {104812, kHalf, 0x0}, {104832, kWord, 0x0},
    {104830, kHalf, 0x0}, {104828, kHalf, 0x0}, {104826, kHalf, 0x0}, {104824, kHalf, 0x0},
    {104844, kWord, 0x0}, {104842, kHalf, 0x0}, {104840, kHalf, 0x0}, {104838, kHalf, 0x0},
    {104836, kHalf, 0x0}, {104856, kWord, 0x0}, {104854, kHalf, 0x0}, {104852, kHalf, 0x0},
    {104850, kHalf, 0x0}, {104848, kHalf, 0x0}, {104868, kWord, 0x0}, {104866, kHalf, 0x0},
    {104864, kHalf, 0x0}, {104862, kHalf, 0x0}, {104860, kHalf, 0x0}, {104880, kWord, 0x0},
    {104878, kHalf, 0x0}, {104876, kHalf, 0x0}, {104874, kHalf, 0x0}, {104872, kHalf, 0x0},
    {104892, kWord, 0x0}, {104890, kHalf, 0x0}, {104888, kHalf, 0x0}, {104886, kHalf, 0x0},
    {104884, kHalf, 0x0}, {104904, kWord, 0x0}, {104902, kHalf, 0x0}, {104900, kHalf, 0x0},
    {104898, kHalf, 0x0}, {104896, kHalf, 0x0}, {104916, kWord, 0x0}, {104914, kHalf, 0x0},
    {104912, kHalf, 0x0}, {104910, kHalf, 0x0}, {104908, kHalf, 0x0}, {104928, kWord, 0x0},
    {104926, kHalf, 0x0}, {104924, kHalf, 0x0}, {104922, kHalf, 0x0}, {104920, kHalf, 0x0},
    {104940, kWord, 0x0}, {104938, kHalf, 0x0}, {104936, kHalf, 0x0}, {104934, kHalf, 0x0},
    {104932, kHalf, 0x0}, {104952, kWord, 0x0}, {104950, kHalf, 0x0}, {104948, kHalf, 0x0},
    {104946, kHalf, 0x0}, {104944, kHalf, 0x0}, {104964, kWord, 0x0}, {104962, kHalf, 0x0},
    {104960, kHalf, 0x0}, {104958, kHalf, 0x0}, {104956, kHalf, 0x0}, {104976, kWord, 0x0},
    {104974, kHalf, 0x0}, {104972, kHalf, 0x0}, {104970, kHalf, 0x0}, {104968, kHalf, 0x0},
    {104988, kWord, 0x0}, {104986, kHalf, 0x0}, {104984, kHalf, 0x0}, {104982, kHalf, 0x0},
    {104980, kHalf, 0x0}, {105000, kWord, 0x0}, {104998, kHalf, 0x0}, {104996, kHalf, 0x0},
    {104994, kHalf, 0x0}, {104992, kHalf, 0x0}, {105012, kWord, 0x0}, {105010, kHalf, 0x0},
    {105008, kHalf, 0x0}, {105006, kHalf, 0x0}, {105004, kHalf, 0x0}, {105024, kWord, 0x0},
    {105022, kHalf, 0x0}, {105020, kHalf, 0x0}, {105018, kHalf, 0x0}, {105016, kHalf, 0x0},
    {105036, kWord, 0x0}, {105034, kHalf, 0x0}, {105032, kHalf, 0x0}, {105030, kHalf, 0x0},
    {105028, kHalf, 0x0}, {105048, kWord, 0x0}, {105046, kHalf, 0x0}, {105044, kHalf, 0x0},
    {105042, kHalf, 0x0}, {105040, kHalf, 0x0}, {105060, kWord, 0x0}, {105058, kHalf, 0x0},
    {105056, kHalf, 0x0}, {105054, kHalf, 0x0}, {105052, kHalf, 0x0}, {105072, kWord, 0x0},
    {105070, kHalf, 0x0}, {105068, kHalf, 0x0}, {105066, kHalf, 0x0}, {105064, kHalf, 0x0},
    {105084, kWord, 0x0}, {105082, kHalf, 0x0}, {105080, kHalf, 0x0}, {105078, kHalf, 0x0},
    {105076, kHalf, 0x0}, {105096, kWord, 0x0}, {105094, kHalf, 0x0}, {105092, kHalf, 0x0},
    {105090, kHalf, 0x0}, {105088, kHalf, 0x0}, {105108, kWord, 0x0}, {105106, kHalf, 0x0},
    {105104, kHalf, 0x0}, {105102, kHalf, 0x0}, {105100, kHalf, 0x0}, {105120, kWord, 0x0},
    {105118, kHalf, 0x0}, {105116, kHalf, 0x0}, {105114, kHalf, 0x0}, {105112, kHalf, 0x0},
    {105132, kWord, 0x0}, {105130, kHalf, 0x0}, {105128, kHalf, 0x0}, {105126, kHalf, 0x0},
    {105124, kHalf, 0x0}, {105144, kWord, 0x0}, {105142, kHalf, 0x0}, {105140, kHalf, 0x0},
    {105138, kHalf, 0x0}, {105136, kHalf, 0x0}, {105156, kWord, 0x0}, {105154, kHalf, 0x0},
    {105152, kHalf, 0x0}, {105150, kHalf, 0x0}, {105148, kHalf, 0x0}, {105168, kWord, 0x0},
    {105166, kHalf, 0x0}, {105164, kHalf, 0x0}, {105162, kHalf, 0x0}, {105160, kHalf, 0x0},
    {105180, kWord, 0x0}, {105178, kHalf, 0x0}, {105176, kHalf, 0x0}, {105174, kHalf, 0x0},
    {105172, kHalf, 0x0}, {105192, kWord, 0x0}, {105190, kHalf, 0x0}, {105188, kHalf, 0x0},
    {105186, kHalf, 0x0}, {105184, kHalf, 0x0}, {105204, kWord, 0x0}, {105202, kHalf, 0x0},
    {105200, kHalf, 0x0}, {105198, kHalf, 0x0}, {105196, kHalf, 0x0}, {105216, kWord, 0x0},
    {105214, kHalf, 0x0}, {105212, kHalf, 0x0}, {105210, kHalf, 0x0}, {105208, kHalf, 0x0},
    {105228, kWord, 0x0}, {105226, kHalf, 0x0}, {105224, kHalf, 0x0}, {105222, kHalf, 0x0},
    {105220, kHalf, 0x0}, {105240, kWord, 0x0}, {105238, kHalf, 0x0}, {105236, kHalf, 0x0},
    {105234, kHalf, 0x0}, {105232, kHalf, 0x0}, {105252, kWord, 0x0}, {105250, kHalf, 0x0},
    {105248, kHalf, 0x0}, {105246, kHalf, 0x0}, {105244, kHalf, 0x0}, {105264, kWord, 0x0},
    {105262, kHalf, 0x0}, {105260, kHalf, 0x0}, {105258, kHalf, 0x0}, {105256, kHalf, 0x0},
    {105276, kWord, 0x0}, {105274, kHalf, 0x0}, {105272, kHalf, 0x0}, {105270, kHalf, 0x0},
    {105268, kHalf, 0x0}, {105288, kWord, 0x0}, {105286, kHalf, 0x0}, {105284, kHalf, 0x0},
    {105282, kHalf, 0x0}, {105280, kHalf, 0x0}, {105300, kWord, 0x0}, {105298, kHalf, 0x0},
    {105296, kHalf, 0x0}, {105294, kHalf, 0x0}, {105292, kHalf, 0x0}, {105312, kWord, 0x0},
    {105310, kHalf, 0x0}, {105308, kHalf, 0x0}, {105306, kHalf, 0x0}, {105304, kHalf, 0x0},
    {105324, kWord, 0x0}, {105322, kHalf, 0x0}, {105320, kHalf, 0x0}, {105318, kHalf, 0x0},
    {105316, kHalf, 0x0}, {105336, kWord, 0x0}, {105334, kHalf, 0x0}, {105332, kHalf, 0x0},
    {105330, kHalf, 0x0}, {105328, kHalf, 0x0}, {105348, kWord, 0x0}, {105346, kHalf, 0x0},
    {105344, kHalf, 0x0}, {105342, kHalf, 0x0}, {105340, kHalf, 0x0}, {105360, kWord, 0x0},
    {105358, kHalf, 0x0}, {105356, kHalf, 0x0}, {105354, kHalf, 0x0}, {105352, kHalf, 0x0},
    {105372, kWord, 0x0}, {105370, kHalf, 0x0}, {105368, kHalf, 0x0}, {105366, kHalf, 0x0},
    {105364, kHalf, 0x0}, {105384, kWord, 0x0}, {105382, kHalf, 0x0}, {105380, kHalf, 0x0},
    {105378, kHalf, 0x0}, {105376, kHalf, 0x0}, {105396, kWord, 0x0}, {105394, kHalf, 0x0},
    {105392, kHalf, 0x0}, {105390, kHalf, 0x0}, {105388, kHalf, 0x0}, {105408, kWord, 0x0},
    {105406, kHalf, 0x0}, {105404, kHalf, 0x0}, {105402, kHalf, 0x0}, {105400, kHalf, 0x0},
    {105420, kWord, 0x0}, {105418, kHalf, 0x0}, {105416, kHalf, 0x0}, {105414, kHalf, 0x0},
    {105412, kHalf, 0x0}, {105432, kWord, 0x0}, {105430, kHalf, 0x0}, {105428, kHalf, 0x0},
    {105426, kHalf, 0x0}, {105424, kHalf, 0x0}, {105444, kWord, 0x0}, {105442, kHalf, 0x0},
    {105440, kHalf, 0x0}, {105438, kHalf, 0x0}, {105436, kHalf, 0x0}, {105456, kWord, 0x0},
    {105454, kHalf, 0x0}, {105452, kHalf, 0x0}, {105450, kHalf, 0x0}, {105448, kHalf, 0x0},
    {105468, kWord, 0x0}, {105466, kHalf, 0x0}, {105464, kHalf, 0x0}, {105462, kHalf, 0x0},
    {105460, kHalf, 0x0}, {105480, kWord, 0x0}, {105478, kHalf, 0x0}, {105476, kHalf, 0x0},
    {105474, kHalf, 0x0}, {105472, kHalf, 0x0}, {105492, kWord, 0x0}, {105490, kHalf, 0x0},
    {105488, kHalf, 0x0}, {105486, kHalf, 0x0}, {105484, kHalf, 0x0}, {105504, kWord, 0x0},
    {105502, kHalf, 0x0}, {105500, kHalf, 0x0}, {105498, kHalf, 0x0}, {105496, kHalf, 0x0},
    {105516, kWord, 0x0}, {105514, kHalf, 0x0}, {105512, kHalf, 0x0}, {105510, kHalf, 0x0},
    {105508, kHalf, 0x0}, {105528, kWord, 0x0}, {105526, kHalf, 0x0}, {105524, kHalf, 0x0},
    {105522, kHalf, 0x0}, {105520, kHalf, 0x0}, {105540, kWord, 0x0}, {105538, kHalf, 0x0},
    {105536, kHalf, 0x0}, {105534, kHalf, 0x0}, {105532, kHalf, 0x0}, {105552, kWord, 0x0},
    {105550, kHalf, 0x0}, {105548, kHalf, 0x0}, {105546, kHalf, 0x0}, {105544, kHalf, 0x0},
    {105564, kWord, 0x0}, {105562, kHalf, 0x0}, {105560, kHalf, 0x0}, {105558, kHalf, 0x0},
    {105556, kHalf, 0x0}, {105576, kWord, 0x0}, {105574, kHalf, 0x0}, {105572, kHalf, 0x0},
    {105570, kHalf, 0x0}, {105568, kHalf, 0x0}, {105588, kWord, 0x0}, {105586, kHalf, 0x0},
    {105584, kHalf, 0x0}, {105582, kHalf, 0x0}, {105580, kHalf, 0x0}, {105600, kWord, 0x0},
    {105598, kHalf, 0x0}, {105596, kHalf, 0x0}, {105594, kHalf, 0x0}, {105592, kHalf, 0x0},
    {105612, kWord, 0x0}, {105610, kHalf, 0x0}, {105608, kHalf, 0x0}, {105606, kHalf, 0x0},
    {105604, kHalf, 0x0}, {105624, kWord, 0x0}, {105622, kHalf, 0x0}, {105620, kHalf, 0x0},
    {105618, kHalf, 0x0}, {105616, kHalf, 0x0}, {105636, kWord, 0x0}, {105634, kHalf, 0x0},
    {105632, kHalf, 0x0}, {105630, kHalf, 0x0}, {105628, kHalf, 0x0}, {105648, kWord, 0x0},
    {105646, kHalf, 0x0}, {105644, kHalf, 0x0}, {105642, kHalf, 0x0}, {105640, kHalf, 0x0},
    {105660, kWord, 0x0}, {105658, kHalf, 0x0}, {105656, kHalf, 0x0}, {105654, kHalf, 0x0},
    {105652, kHalf, 0x0}, {105672, kWord, 0x0}, {105670, kHalf, 0x0}, {105668, kHalf, 0x0},
    {105666, kHalf, 0x0}, {105664, kHalf, 0x0}, {105684, kWord, 0x0}, {105682, kHalf, 0x0},
    {105680, kHalf, 0x0}, {105678, kHalf, 0x0}, {105676, kHalf, 0x0}, {105696, kWord, 0x0},
    {105694, kHalf, 0x0}, {105692, kHalf, 0x0}, {105690, kHalf, 0x0}, {105688, kHalf, 0x0},
    {105708, kWord, 0x0}, {105706, kHalf, 0x0}, {105704, kHalf, 0x0}, {105702, kHalf, 0x0},
    {105700, kHalf, 0x0}, {105720, kWord, 0x0}, {105718, kHalf, 0x0}, {105716, kHalf, 0x0},
    {105714, kHalf, 0x0}, {105712, kHalf, 0x0}, {105732, kWord, 0x0}, {105730, kHalf, 0x0},
    {105728, kHalf, 0x0}, {105726, kHalf, 0x0}, {105724, kHalf, 0x0}, {105744, kWord, 0x0},
    {105742, kHalf, 0x0}, {105740, kHalf, 0x0}, {105738, kHalf, 0x0}, {105736, kHalf, 0x0},
    {105756, kWord, 0x0}, {105754, kHalf, 0x0}, {105752, kHalf, 0x0}, {105750, kHalf, 0x0},
    {105748, kHalf, 0x0}, {105768, kWord, 0x0}, {105766, kHalf, 0x0}, {105764, kHalf, 0x0},
    {105762, kHalf, 0x0}, {105760, kHalf, 0x0}, {105780, kWord, 0x0}, {105778, kHalf, 0x0},
    {105776, kHalf, 0x0}, {105774, kHalf, 0x0}, {105772, kHalf, 0x0}, {105792, kWord, 0x0},
    {105790, kHalf, 0x0}, {105788, kHalf, 0x0}, {105786, kHalf, 0x0}, {105784, kHalf, 0x0},
    {105804, kWord, 0x0}, {105802, kHalf, 0x0}, {105800, kHalf, 0x0}, {105798, kHalf, 0x0},
    {105796, kHalf, 0x0}, {105816, kWord, 0x0}, {105814, kHalf, 0x0}, {105812, kHalf, 0x0},
    {105810, kHalf, 0x0}, {105808, kHalf, 0x0}, {105828, kWord, 0x0}, {105826, kHalf, 0x0},
    {105824, kHalf, 0x0}, {105822, kHalf, 0x0}, {105820, kHalf, 0x0}, {105840, kWord, 0x0},
    {105838, kHalf, 0x0}, {105836, kHalf, 0x0}, {105834, kHalf, 0x0}, {105832, kHalf, 0x0},
    {105852, kWord, 0x0}, {105850, kHalf, 0x0}, {105848, kHalf, 0x0}, {105846, kHalf, 0x0},
    {105844, kHalf, 0x0}, {105864, kWord, 0x0}, {105862, kHalf, 0x0}, {105860, kHalf, 0x0},
    {105858, kHalf, 0x0}, {105856, kHalf, 0x0}, {105876, kWord, 0x0}, {105874, kHalf, 0x0},
    {105872, kHalf, 0x0}, {105870, kHalf, 0x0}, {105868, kHalf, 0x0}, {105888, kWord, 0x0},
    {105886, kHalf, 0x0}, {105884, kHalf, 0x0}, {105882, kHalf, 0x0}, {105880, kHalf, 0x0},
    {105900, kWord, 0x0}, {105898, kHalf, 0x0}, {105896, kHalf, 0x0}, {105894, kHalf, 0x0},
    {105892, kHalf, 0x0}, {105912, kWord, 0x0}, {105910, kHalf, 0x0}, {105908, kHalf, 0x0},
    {105906, kHalf, 0x0}, {105904, kHalf, 0x0}, {105924, kWord, 0x0}, {105922, kHalf, 0x0},
    {105920, kHalf, 0x0}, {105918, kHalf, 0x0}, {105916, kHalf, 0x0}, {105936, kWord, 0x0},
    {105934, kHalf, 0x0}, {105932, kHalf, 0x0}, {105930, kHalf, 0x0}, {105928, kHalf, 0x0},
    {105948, kWord, 0x0}, {105946, kHalf, 0x0}, {105944, kHalf, 0x0}, {105942, kHalf, 0x0},
    {105940, kHalf, 0x0}, {105960, kWord, 0x0}, {105958, kHalf, 0x0}, {105956, kHalf, 0x0},
    {105954, kHalf, 0x0}, {105952, kHalf, 0x0}, {105972, kWord, 0x0}, {105970, kHalf, 0x0},
    {105968, kHalf, 0x0}, {105966, kHalf, 0x0}, {105964, kHalf, 0x0}, {105984, kWord, 0x0},
    {105982, kHalf, 0x0}, {105980, kHalf, 0x0}, {105978, kHalf, 0x0}, {105976, kHalf, 0x0},
    {105996, kWord, 0x0}, {105994, kHalf, 0x0}, {105992, kHalf, 0x0}, {105990, kHalf, 0x0},
    {105988, kHalf, 0x0}, {106008, kWord, 0x0}, {106006, kHalf, 0x0}, {106004, kHalf, 0x0},
    {106002, kHalf, 0x0}, {106000, kHalf, 0x0}, {106020, kWord, 0x0}, {106018, kHalf, 0x0},
    {106016, kHalf, 0x0}, {106014, kHalf, 0x0}, {106012, kHalf, 0x0}, {106032, kWord, 0x0},
    {106030, kHalf, 0x0}, {106028, kHalf, 0x0}, {106026, kHalf, 0x0}, {106024, kHalf, 0x0},
    {106044, kWord, 0x0}, {106042, kHalf, 0x0}, {106040, kHalf, 0x0}, {106038, kHalf, 0x0},
    {106036, kHalf, 0x0}, {106056, kWord, 0x0}, {106054, kHalf, 0x0}, {106052, kHalf, 0x0},
    {106050, kHalf, 0x0}, {106048, kHalf, 0x0}, {106068, kWord, 0x0}, {106066, kHalf, 0x0},
    {106064, kHalf, 0x0}, {106062, kHalf, 0x0}, {106060, kHalf, 0x0}, {106080, kWord, 0x0},
    {106078, kHalf, 0x0}, {106076, kHalf, 0x0}, {106074, kHalf, 0x0}, {106072, kHalf, 0x0},
    {106092, kWord, 0x0}, {106090, kHalf, 0x0}, {106088, kHalf, 0x0}, {106086, kHalf, 0x0},
    {106084, kHalf, 0x0}, {106104, kWord, 0x0}, {106102, kHalf, 0x0}, {106100, kHalf, 0x0},
    {106098, kHalf, 0x0}, {106096, kHalf, 0x0}, {106116, kWord, 0x0}, {106114, kHalf, 0x0},
    {106112, kHalf, 0x0}, {106110, kHalf, 0x0}, {106108, kHalf, 0x0}, {106128, kWord, 0x0},
    {106126, kHalf, 0x0}, {106124, kHalf, 0x0}, {106122, kHalf, 0x0}, {106120, kHalf, 0x0},
    {106140, kWord, 0x0}, {106138, kHalf, 0x0}, {106136, kHalf, 0x0}, {106134, kHalf, 0x0},
    {106132, kHalf, 0x0}, {106152, kWord, 0x0}, {106150, kHalf, 0x0}, {106148, kHalf, 0x0},
    {106146, kHalf, 0x0}, {106144, kHalf, 0x0}, {106164, kWord, 0x0}, {106162, kHalf, 0x0},
    {106160, kHalf, 0x0}, {106158, kHalf, 0x0}, {106156, kHalf, 0x0}, {106176, kWord, 0x0},
    {106174, kHalf, 0x0}, {106172, kHalf, 0x0}, {106170, kHalf, 0x0}, {106168, kHalf, 0x0},
    {106188, kWord, 0x0}, {106186, kHalf, 0x0}, {106184, kHalf, 0x0}, {106182, kHalf, 0x0},
    {106180, kHalf, 0x0}, {106200, kWord, 0x0}, {106198, kHalf, 0x0}, {106196, kHalf, 0x0},
    {106194, kHalf, 0x0}, {106192, kHalf, 0x0}, {106212, kWord, 0x0}, {106210, kHalf, 0x0},
    {106208, kHalf, 0x0}, {106206, kHalf, 0x0}, {106204, kHalf, 0x0}, {106224, kWord, 0x0},
    {106222, kHalf, 0x0}, {106220, kHalf, 0x0}, {106218, kHalf, 0x0}, {106216, kHalf, 0x0},
    {106236, kWord, 0x0}, {106234, kHalf, 0x0}, {106232, kHalf, 0x0}, {106230, kHalf, 0x0},
    {106228, kHalf, 0x0}, {106248, kWord, 0x0}, {106246, kHalf, 0x0}, {106244, kHalf, 0x0},
    {106242, kHalf, 0x0}, {106240, kHalf, 0x0}, {106260, kWord, 0x0}, {106258, kHalf, 0x0},
    {106256, kHalf, 0x0}, {106254, kHalf, 0x0}, {106252, kHalf, 0x0}, {106272, kWord, 0x0},
    {106270, kHalf, 0x0}, {106268, kHalf, 0x0}, {106266, kHalf, 0x0}, {106264, kHalf, 0x0},
    {106284, kWord, 0x0}, {106282, kHalf, 0x0}, {106280, kHalf, 0x0}, {106278, kHalf, 0x0},
    {106276, kHalf, 0x0}, {106296, kWord, 0x0}, {106294, kHalf, 0x0}, {106292, kHalf, 0x0},
    {106290, kHalf, 0x0}, {106288, kHalf, 0x0}, {106308, kWord, 0x0}, {106306, kHalf, 0x0},
    {106304, kHalf, 0x0}, {106302, kHalf, 0x0}, {106300, kHalf, 0x0}, {106320, kWord, 0x0},
    {106318, kHalf, 0x0}, {106316, kHalf, 0x0}, {106314, kHalf, 0x0}, {106312, kHalf, 0x0},
    {106332, kWord, 0x0}, {106330, kHalf, 0x0}, {106328, kHalf, 0x0}, {106326, kHalf, 0x0},
    {106324, kHalf, 0x0}, {106344, kWord, 0x0}, {106342, kHalf, 0x0}, {106340, kHalf, 0x0},
    {106338, kHalf, 0x0}, {106336, kHalf, 0x0}, {106356, kWord, 0x0}, {106354, kHalf, 0x0},
    {106352, kHalf, 0x0}, {106350, kHalf, 0x0}, {106348, kHalf, 0x0}, {106368, kWord, 0x0},
    {106366, kHalf, 0x0}, {106364, kHalf, 0x0}, {106362, kHalf, 0x0}, {106360, kHalf, 0x0},
    {106380, kWord, 0x0}, {106378, kHalf, 0x0}, {106376, kHalf, 0x0}, {106374, kHalf, 0x0},
    {106372, kHalf, 0x0}, {106392, kWord, 0x0}, {106390, kHalf, 0x0}, {106388, kHalf, 0x0},
    {106386, kHalf, 0x0}, {106384, kHalf, 0x0}, {106404, kWord, 0x0}, {106402, kHalf, 0x0},
    {106400, kHalf, 0x0}, {106398, kHalf, 0x0}, {106396, kHalf, 0x0}, {106416, kWord, 0x0},
    {106414, kHalf, 0x0}, {106412, kHalf, 0x0}, {106410, kHalf, 0x0}, {106408, kHalf, 0x0},
    {106428, kWord, 0x0}, {106426, kHalf, 0x0}, {106424, kHalf, 0x0}, {106422, kHalf, 0x0},
    {106420, kHalf, 0x0}, {106440, kWord, 0x0}, {106438, kHalf, 0x0}, {106436, kHalf, 0x0},
    {106434, kHalf, 0x0}, {106432, kHalf, 0x0}, {106452, kWord, 0x0}, {106450, kHalf, 0x0},
    {106448, kHalf, 0x0}, {106446, kHalf, 0x0}, {106444, kHalf, 0x0}, {106464, kWord, 0x0},
    {106462, kHalf, 0x0}, {106460, kHalf, 0x0}, {106458, kHalf, 0x0}, {106456, kHalf, 0x0},
    {106476, kWord, 0x0}, {106474, kHalf, 0x0}, {106472, kHalf, 0x0}, {106470, kHalf, 0x0},
    {106468, kHalf, 0x0}, {106488, kWord, 0x0}, {106486, kHalf, 0x0}, {106484, kHalf, 0x0},
    {106482, kHalf, 0x0}, {106480, kHalf, 0x0}, {106500, kWord, 0x0}, {106498, kHalf, 0x0},
    {106496, kHalf, 0x0}, {106494, kHalf, 0x0}, {106492, kHalf, 0x0}, {106512, kWord, 0x0},
    {106510, kHalf, 0x0}, {106508, kHalf, 0x0}, {106506, kHalf, 0x0}, {106504, kHalf, 0x0},
    {106524, kWord, 0x0}, {106522, kHalf, 0x0}, {106520, kHalf, 0x0}, {106518, kHalf, 0x0},
    {106516, kHalf, 0x0}, {106536, kWord, 0x0}, {106534, kHalf, 0x0}, {106532, kHalf, 0x0},
    {106530, kHalf, 0x0}, {106528, kHalf, 0x0}, {106548, kWord, 0x0}, {106546, kHalf, 0x0},
    {106544, kHalf, 0x0}, {106542, kHalf, 0x0}, {106540, kHalf, 0x0}, {106560, kWord, 0x0},
    {106558, kHalf, 0x0}, {106556, kHalf, 0x0}, {106554, kHalf, 0x0}, {106552, kHalf, 0x0},
    {106572, kWord, 0x0}, {106570, kHalf, 0x0}, {106568, kHalf, 0x0}, {106566, kHalf, 0x0},
    {106564, kHalf, 0x0}, {106584, kWord, 0x0}, {106582, kHalf, 0x0}, {106580, kHalf, 0x0},
    {106578, kHalf, 0x0}, {106576, kHalf, 0x0}, {106596, kWord, 0x0}, {106594, kHalf, 0x0},
    {106592, kHalf, 0x0}, {106590, kHalf, 0x0}, {106588, kHalf, 0x0}, {106608, kWord, 0x0},
    {106606, kHalf, 0x0}, {106604, kHalf, 0x0}, {106602, kHalf, 0x0}, {106600, kHalf, 0x0},
    {106620, kWord, 0x0}, {106618, kHalf, 0x0}, {106616, kHalf, 0x0}, {106614, kHalf, 0x0},
    {106612, kHalf, 0x0}, {106632, kWord, 0x0}, {106630, kHalf, 0x0}, {106628, kHalf, 0x0},
    {106626, kHalf, 0x0}, {106624, kHalf, 0x0}, {106644, kWord, 0x0}, {106642, kHalf, 0x0},
    {106640, kHalf, 0x0}, {106638, kHalf, 0x0}, {106636, kHalf, 0x0}, {106656, kWord, 0x0},
    {106654, kHalf, 0x0}, {106652, kHalf, 0x0}, {106650, kHalf, 0x0}, {106648, kHalf, 0x0},
    {106668, kWord, 0x0}, {106666, kHalf, 0x0}, {106664, kHalf, 0x0}, {106662, kHalf, 0x0},
    {106660, kHalf, 0x0}, {106680, kWord, 0x0}, {106678, kHalf, 0x0}, {106676, kHalf, 0x0},
    {106674, kHalf, 0x0}, {106672, kHalf, 0x0}, {106692, kWord, 0x0}, {106690, kHalf, 0x0},
    {106688, kHalf, 0x0}, {106686, kHalf, 0x0}, {106684, kHalf, 0x0}, {106704, kWord, 0x0},
    {106702, kHalf, 0x0}, {106700, kHalf, 0x0}, {106698, kHalf, 0x0}, {106696, kHalf, 0x0},
    {106716, kWord, 0x0}, {106714, kHalf, 0x0}, {106712, kHalf, 0x0}, {106710, kHalf, 0x0},
    {106708, kHalf, 0x0}, {106728, kWord, 0x0}, {106726, kHalf, 0x0}, {106724, kHalf, 0x0},
    {106722, kHalf, 0x0}, {106720, kHalf, 0x0}, {106740, kWord, 0x0}, {106738, kHalf, 0x0},
    {106736, kHalf, 0x0}, {106734, kHalf, 0x0}, {106732, kHalf, 0x0}, {106752, kWord, 0x0},
    {106750, kHalf, 0x0}, {106748, kHalf, 0x0}, {106746, kHalf, 0x0}, {106744, kHalf, 0x0},
    {106764, kWord, 0x0}, {106762, kHalf, 0x0}, {106760, kHalf, 0x0}, {106758, kHalf, 0x0},
    {106756, kHalf, 0x0}, {106776, kWord, 0x0}, {106774, kHalf, 0x0}, {106772, kHalf, 0x0},
    {106770, kHalf, 0x0}, {106768, kHalf, 0x0}, {106788, kWord, 0x0}, {106786, kHalf, 0x0},
    {106784, kHalf, 0x0}, {106782, kHalf, 0x0}, {106780, kHalf, 0x0}, {106800, kWord, 0x0},
    {106798, kHalf, 0x0}, {106796, kHalf, 0x0}, {106794, kHalf, 0x0}, {106792, kHalf, 0x0},
    {106812, kWord, 0x0}, {106810, kHalf, 0x0}, {106808, kHalf, 0x0}, {106806, kHalf, 0x0},
    {106804, kHalf, 0x0}, {106824, kWord, 0x0}, {106822, kHalf, 0x0}, {106820, kHalf, 0x0},
    {106818, kHalf, 0x0}, {106816, kHalf, 0x0}, {106836, kWord, 0x0}, {106834, kHalf, 0x0},
    {106832, kHalf, 0x0}, {106830, kHalf, 0x0}, {106828, kHalf, 0x0}, {106848, kWord, 0x0},
    {106846, kHalf, 0x0}, {106844, kHalf, 0x0}, {106842, kHalf, 0x0}, {106840, kHalf, 0x0},
    {106860, kWord, 0x0}, {106858, kHalf, 0x0}, {106856, kHalf, 0x0}, {106854, kHalf, 0x0},
    {106852, kHalf, 0x0}, {106872, kWord, 0x0}, {106870, kHalf, 0x0}, {106868, kHalf, 0x0},
    {106866, kHalf, 0x0}, {106864, kHalf, 0x0}, {106884, kWord, 0x0}, {106882, kHalf, 0x0},
    {106880, kHalf, 0x0}, {106878, kHalf, 0x0}, {106876, kHalf, 0x0}, {106896, kWord, 0x0},
    {106894, kHalf, 0x0}, {106892, kHalf, 0x0}, {106890, kHalf, 0x0}, {106888, kHalf, 0x0},
    {106908, kWord, 0x0}, {106906, kHalf, 0x0}, {106904, kHalf, 0x0}, {106902, kHalf, 0x0},
    {106900, kHalf, 0x0}, {106920, kWord, 0x0}, {106918, kHalf, 0x0}, {106916, kHalf, 0x0},
    {106914, kHalf, 0x0}, {106912, kHalf, 0x0}, {106932, kWord, 0x0}, {106930, kHalf, 0x0},
    {106928, kHalf, 0x0}, {106926, kHalf, 0x0}, {106924, kHalf, 0x0}, {106944, kWord, 0x0},
    {106942, kHalf, 0x0}, {106940, kHalf, 0x0}, {106938, kHalf, 0x0}, {106936, kHalf, 0x0},
    {106956, kWord, 0x0}, {106954, kHalf, 0x0}, {106952, kHalf, 0x0}, {106950, kHalf, 0x0},
    {106948, kHalf, 0x0}, {106968, kWord, 0x0}, {106966, kHalf, 0x0}, {106964, kHalf, 0x0},
    {106962, kHalf, 0x0}, {106960, kHalf, 0x0}, {106980, kWord, 0x0}, {106978, kHalf, 0x0},
    {106976, kHalf, 0x0}, {106974, kHalf, 0x0}, {106972, kHalf, 0x0}, {106992, kWord, 0x0},
    {106990, kHalf, 0x0}, {106988, kHalf, 0x0}, {106986, kHalf, 0x0}, {106984, kHalf, 0x0},
    {107004, kWord, 0x0}, {107002, kHalf, 0x0}, {107000, kHalf, 0x0}, {106998, kHalf, 0x0},
    {106996, kHalf, 0x0}, {107016, kWord, 0x0}, {107014, kHalf, 0x0}, {107012, kHalf, 0x0},
    {107010, kHalf, 0x0}, {107008, kHalf, 0x0}, {107028, kWord, 0x0}, {107026, kHalf, 0x0},
    {107024, kHalf, 0x0}, {107022, kHalf, 0x0}, {107020, kHalf, 0x0}, {107040, kWord, 0x0},
    {107038, kHalf, 0x0}, {107036, kHalf, 0x0}, {107034, kHalf, 0x0}, {107032, kHalf, 0x0},
    {107052, kWord, 0x0}, {107050, kHalf, 0x0}, {107048, kHalf, 0x0}, {107046, kHalf, 0x0},
    {107044, kHalf, 0x0}, {107064, kWord, 0x0}, {107062, kHalf, 0x0}, {107060, kHalf, 0x0},
    {107058, kHalf, 0x0}, {107056, kHalf, 0x0}, {107076, kWord, 0x0}, {107074, kHalf, 0x0},
    {107072, kHalf, 0x0}, {107070, kHalf, 0x0}, {107068, kHalf, 0x0}, {107088, kWord, 0x0},
    {107086, kHalf, 0x0}, {107084, kHalf, 0x0}, {107082, kHalf, 0x0}, {107080, kHalf, 0x0},
    {107100, kWord, 0x0}, {107098, kHalf, 0x0}, {107096, kHalf, 0x0}, {107094, kHalf, 0x0},
    {107092, kHalf, 0x0}, {107112, kWord, 0x0}, {107110, kHalf, 0x0}, {107108, kHalf, 0x0},
    {107106, kHalf, 0x0}, {107104, kHalf, 0x0}, {107124, kWord, 0x0}, {107122, kHalf, 0x0},
    {107120, kHalf, 0x0}, {107118, kHalf, 0x0}, {107116, kHalf, 0x0}, {107136, kWord, 0x0},
    {107134, kHalf, 0x0}, {107132, kHalf, 0x0}, {107130, kHalf, 0x0}, {107128, kHalf, 0x0},
    {107148, kWord, 0x0}, {107146, kHalf, 0x0}, {107144, kHalf, 0x0}, {107142, kHalf, 0x0},
    {107140, kHalf, 0x0}, {107160, kWord, 0x0}, {107158, kHalf, 0x0}, {107156, kHalf, 0x0},
    {107154, kHalf, 0x0}, {107152, kHalf, 0x0}, {107172, kWord, 0x0}, {107170, kHalf, 0x0},
    {107168, kHalf, 0x0}, {107166, kHalf, 0x0}, {107164, kHalf, 0x0}, {107184, kWord, 0x0},
    {107182, kHalf, 0x0}, {107180, kHalf, 0x0}, {107178, kHalf, 0x0}, {107176, kHalf, 0x0},
    {107196, kWord, 0x0}, {107194, kHalf, 0x0}, {107192, kHalf, 0x0}, {107190, kHalf, 0x0},
    {107188, kHalf, 0x0}, {107208, kWord, 0x0}, {107206, kHalf, 0x0}, {107204, kHalf, 0x0},
    {107202, kHalf, 0x0}, {107200, kHalf, 0x0}, {107220, kWord, 0x0}, {107218, kHalf, 0x0},
    {107216, kHalf, 0x0}, {107214, kHalf, 0x0}, {107212, kHalf, 0x0}, {107232, kWord, 0x0},
    {107230, kHalf, 0x0}, {107228, kHalf, 0x0}, {107226, kHalf, 0x0}, {107224, kHalf, 0x0},
    {107244, kWord, 0x0}, {107242, kHalf, 0x0}, {107240, kHalf, 0x0}, {107238, kHalf, 0x0},
    {107236, kHalf, 0x0}, {107256, kWord, 0x0}, {107254, kHalf, 0x0}, {107252, kHalf, 0x0},
    {107250, kHalf, 0x0}, {107248, kHalf, 0x0}, {107268, kWord, 0x0}, {107266, kHalf, 0x0},
    {107264, kHalf, 0x0}, {107262, kHalf, 0x0}, {107260, kHalf, 0x0}, {107280, kWord, 0x0},
    {107278, kHalf, 0x0}, {107276, kHalf, 0x0}, {107274, kHalf, 0x0}, {107272, kHalf, 0x0},
    {107292, kWord, 0x0}, {107290, kHalf, 0x0}, {107288, kHalf, 0x0}, {107286, kHalf, 0x0},
    {107284, kHalf, 0x0}, {107304, kWord, 0x0}, {107302, kHalf, 0x0}, {107300, kHalf, 0x0},
    {107298, kHalf, 0x0}, {107296, kHalf, 0x0}, {107316, kWord, 0x0}, {107314, kHalf, 0x0},
    {107312, kHalf, 0x0}, {107310, kHalf, 0x0}, {107308, kHalf, 0x0}, {107328, kWord, 0x0},
    {107326, kHalf, 0x0}, {107324, kHalf, 0x0}, {107322, kHalf, 0x0}, {107320, kHalf, 0x0},
    {107340, kWord, 0x0}, {107338, kHalf, 0x0}, {107336, kHalf, 0x0}, {107334, kHalf, 0x0},
    {107332, kHalf, 0x0}, {107352, kWord, 0x0}, {107350, kHalf, 0x0}, {107348, kHalf, 0x0},
    {107346, kHalf, 0x0}, {107344, kHalf, 0x0}, {107364, kWord, 0x0}, {107362, kHalf, 0x0},
    {107360, kHalf, 0x0}, {107358, kHalf, 0x0}, {107356, kHalf, 0x0}, {107376, kWord, 0x0},
    {107374, kHalf, 0x0}, {107372, kHalf, 0x0}, {107370, kHalf, 0x0}, {107368, kHalf, 0x0},
    {107388, kWord, 0x0}, {107386, kHalf, 0x0}, {107384, kHalf, 0x0}, {107382, kHalf, 0x0},
    {107380, kHalf, 0x0}, {107400, kWord, 0x0}, {107398, kHalf, 0x0}, {107396, kHalf, 0x0},
    {107394, kHalf, 0x0}, {107392, kHalf, 0x0}, {107412, kWord, 0x0}, {107410, kHalf, 0x0},
    {107408, kHalf, 0x0}, {107406, kHalf, 0x0}, {107404, kHalf, 0x0}, {107424, kWord, 0x0},
    {107422, kHalf, 0x0}, {107420, kHalf, 0x0}, {107418, kHalf, 0x0}, {107416, kHalf, 0x0},
    {107436, kWord, 0x0}, {107434, kHalf, 0x0}, {107432, kHalf, 0x0}, {107430, kHalf, 0x0},
    {107428, kHalf, 0x0}, {107448, kWord, 0x0}, {107446, kHalf, 0x0}, {107444, kHalf, 0x0},
    {107442, kHalf, 0x0}, {107440, kHalf, 0x0}, {107460, kWord, 0x0}, {107458, kHalf, 0x0},
    {107456, kHalf, 0x0}, {107454, kHalf, 0x0}, {107452, kHalf, 0x0}, {107472, kWord, 0x0},
    {107470, kHalf, 0x0}, {107468, kHalf, 0x0}, {107466, kHalf, 0x0}, {107464, kHalf, 0x0},
    {107484, kWord, 0x0}, {107482, kHalf, 0x0}, {107480, kHalf, 0x0}, {107478, kHalf, 0x0},
    {107476, kHalf, 0x0}, {107496, kWord, 0x0}, {107494, kHalf, 0x0}, {107492, kHalf, 0x0},
    {107490, kHalf, 0x0}, {107488, kHalf, 0x0}, {107508, kWord, 0x0}, {107506, kHalf, 0x0},
    {107504, kHalf, 0x0}, {107502, kHalf, 0x0}, {107500, kHalf, 0x0}, {107520, kWord, 0x0},
    {107518, kHalf, 0x0}, {107516, kHalf, 0x0}, {107514, kHalf, 0x0}, {107512, kHalf, 0x0},
    {107532, kWord, 0x0}, {107530, kHalf, 0x0}, {107528, kHalf, 0x0}, {107526, kHalf, 0x0},
    {107524, kHalf, 0x0}, {107544, kWord, 0x0}, {107542, kHalf, 0x0}, {107540, kHalf, 0x0},
    {107538, kHalf, 0x0}, {107536, kHalf, 0x0}, {107556, kWord, 0x0}, {107554, kHalf, 0x0},
    {107552, kHalf, 0x0}, {107550, kHalf, 0x0}, {107548, kHalf, 0x0}, {107568, kWord, 0x0},
    {107566, kHalf, 0x0}, {107564, kHalf, 0x0}, {107562, kHalf, 0x0}, {107560, kHalf, 0x0},
    {107580, kWord, 0x0}, {107578, kHalf, 0x0}, {107576, kHalf, 0x0}, {107574, kHalf, 0x0},
    {107572, kHalf, 0x0}, {107592, kWord, 0x0}, {107590, kHalf, 0x0}, {107588, kHalf, 0x0},
    {107586, kHalf, 0x0}, {107584, kHalf, 0x0}, {107604, kWord, 0x0}, {107602, kHalf, 0x0},
    {107600, kHalf, 0x0}, {107598, kHalf, 0x0}, {107596, kHalf, 0x0}, {107616, kWord, 0x0},
    {107614, kHalf, 0x0}, {107612, kHalf, 0x0}, {107610, kHalf, 0x0}, {107608, kHalf, 0x0},
    {107628, kWord, 0x0}, {107626, kHalf, 0x0}, {107624, kHalf, 0x0}, {107622, kHalf, 0x0},
    {107620, kHalf, 0x0}, {107640, kWord, 0x0}, {107638, kHalf, 0x0}, {107636, kHalf, 0x0},
    {107634, kHalf, 0x0}, {107632, kHalf, 0x0}, {107652, kWord, 0x0}, {107650, kHalf, 0x0},
    {107648, kHalf, 0x0}, {107646, kHalf, 0x0}, {107644, kHalf, 0x0}, {107664, kWord, 0x0},
    {107662, kHalf, 0x0}, {107660, kHalf, 0x0}, {107658, kHalf, 0x0}, {107656, kHalf, 0x0},
    {107676, kWord, 0x0}, {107674, kHalf, 0x0}, {107672, kHalf, 0x0}, {107670, kHalf, 0x0},
    {107668, kHalf, 0x0}, {107688, kWord, 0x0}, {107686, kHalf, 0x0}, {107684, kHalf, 0x0},
    {107682, kHalf, 0x0}, {107680, kHalf, 0x0}, {107700, kWord, 0x0}, {107698, kHalf, 0x0},
    {107696, kHalf, 0x0}, {107694, kHalf, 0x0}, {107692, kHalf, 0x0}, {107712, kWord, 0x0},
    {107710, kHalf, 0x0}, {107708, kHalf, 0x0}, {107706, kHalf, 0x0}, {107704, kHalf, 0x0},
    {107724, kWord, 0x0}, {107722, kHalf, 0x0}, {107720, kHalf, 0x0}, {107718, kHalf, 0x0},
    {107716, kHalf, 0x0}, {107736, kWord, 0x0}, {107734, kHalf, 0x0}, {107732, kHalf, 0x0},
    {107730, kHalf, 0x0}, {107728, kHalf, 0x0}, {107748, kWord, 0x0}, {107746, kHalf, 0x0},
    {107744, kHalf, 0x0}, {107742, kHalf, 0x0}, {107740, kHalf, 0x0}, {107760, kWord, 0x0},
    {107758, kHalf, 0x0}, {107756, kHalf, 0x0}, {107754, kHalf, 0x0}, {107752, kHalf, 0x0},
    {107772, kWord, 0x0}, {107770, kHalf, 0x0}, {107768, kHalf, 0x0}, {107766, kHalf, 0x0},
    {107764, kHalf, 0x0}, {107784, kWord, 0x0}, {107782, kHalf, 0x0}, {107780, kHalf, 0x0},
    {107778, kHalf, 0x0}, {107776, kHalf, 0x0}, {107796, kWord, 0x0}, {107794, kHalf, 0x0},
    {107792, kHalf, 0x0}, {107790, kHalf, 0x0}, {107788, kHalf, 0x0}, {107808, kWord, 0x0},
    {107806, kHalf, 0x0}, {107804, kHalf, 0x0}, {107802, kHalf, 0x0}, {107800, kHalf, 0x0},
    {107820, kWord, 0x0}, {107818, kHalf, 0x0}, {107816, kHalf, 0x0}, {107814, kHalf, 0x0},
    {107812, kHalf, 0x0}, {107832, kWord, 0x0}, {107830, kHalf, 0x0}, {107828, kHalf, 0x0},
    {107826, kHalf, 0x0}, {107824, kHalf, 0x0}, {107844, kWord, 0x0}, {107842, kHalf, 0x0},
    {107840, kHalf, 0x0}, {107838, kHalf, 0x0}, {107836, kHalf, 0x0}, {107856, kWord, 0x0},
    {107854, kHalf, 0x0}, {107852, kHalf, 0x0}, {107850, kHalf, 0x0}, {107848, kHalf, 0x0},
    {107868, kWord, 0x0}, {107866, kHalf, 0x0}, {107864, kHalf, 0x0}, {107862, kHalf, 0x0},
    {107860, kHalf, 0x0}, {107880, kWord, 0x0}, {107878, kHalf, 0x0}, {107876, kHalf, 0x0},
    {107874, kHalf, 0x0}, {107872, kHalf, 0x0}, {107892, kWord, 0x0}, {107890, kHalf, 0x0},
    {107888, kHalf, 0x0}, {107886, kHalf, 0x0}, {107884, kHalf, 0x0}, {107904, kWord, 0x0},
    {107902, kHalf, 0x0}, {107900, kHalf, 0x0}, {107898, kHalf, 0x0}, {107896, kHalf, 0x0},
    {107916, kWord, 0x0}, {107914, kHalf, 0x0}, {107912, kHalf, 0x0}, {107910, kHalf, 0x0},
    {107908, kHalf, 0x0}, {107928, kWord, 0x0}, {107926, kHalf, 0x0}, {107924, kHalf, 0x0},
    {107922, kHalf, 0x0}, {107920, kHalf, 0x0}, {107940, kWord, 0x0}, {107938, kHalf, 0x0},
    {107936, kHalf, 0x0}, {107934, kHalf, 0x0}, {107932, kHalf, 0x0}, {107952, kWord, 0x0},
    {107950, kHalf, 0x0}, {107948, kHalf, 0x0}, {107946, kHalf, 0x0}, {107944, kHalf, 0x0},
    {107964, kWord, 0x0}, {107962, kHalf, 0x0}, {107960, kHalf, 0x0}, {107958, kHalf, 0x0},
    {107956, kHalf, 0x0}, {107976, kWord, 0x0}, {107974, kHalf, 0x0}, {107972, kHalf, 0x0},
    {107970, kHalf, 0x0}, {107968, kHalf, 0x0}, {107988, kWord, 0x0}, {107986, kHalf, 0x0},
    {107984, kHalf, 0x0}, {107982, kHalf, 0x0}, {107980, kHalf, 0x0}, {108000, kWord, 0x0},
    {107998, kHalf, 0x0}, {107996, kHalf, 0x0}, {107994, kHalf, 0x0}, {107992, kHalf, 0x0},
    {108012, kWord, 0x0}, {108010, kHalf, 0x0}, {108008, kHalf, 0x0}, {108006, kHalf, 0x0},
    {108004, kHalf, 0x0}, {108024, kWord, 0x0}, {108022, kHalf, 0x0}, {108020, kHalf, 0x0},
    {108018, kHalf, 0x0}, {108016, kHalf, 0x0}, {108036, kWord, 0x0}, {108034, kHalf, 0x0},
    {108032, kHalf, 0x0}, {108030, kHalf, 0x0}, {108028, kHalf, 0x0}, {108048, kWord, 0x0},
    {108046, kHalf, 0x0}, {108044, kHalf, 0x0}, {108042, kHalf, 0x0}, {108040, kHalf, 0x0},
    {108060, kWord, 0x0}, {108058, kHalf, 0x0}, {108056, kHalf, 0x0}, {108054, kHalf, 0x0},
    {108052, kHalf, 0x0}, {108072, kWord, 0x0}, {108070, kHalf, 0x0}, {108068, kHalf, 0x0},
    {108066, kHalf, 0x0}, {108064, kHalf, 0x0}, {108084, kWord, 0x0}, {108082, kHalf, 0x0},
    {108080, kHalf, 0x0}, {108078, kHalf, 0x0}, {108076, kHalf, 0x0}, {108096, kWord, 0x0},
    {108094, kHalf, 0x0}, {108092, kHalf, 0x0}, {108090, kHalf, 0x0}, {108088, kHalf, 0x0},
    {108108, kWord, 0x0}, {108106, kHalf, 0x0}, {108104, kHalf, 0x0}, {108102, kHalf, 0x0},
    {108100, kHalf, 0x0}, {108120, kWord, 0x0}, {108118, kHalf, 0x0}, {108116, kHalf, 0x0},
    {108114, kHalf, 0x0}, {108112, kHalf, 0x0}, {108132, kWord, 0x0}, {108130, kHalf, 0x0},
    {108128, kHalf, 0x0}, {108126, kHalf, 0x0}, {108124, kHalf, 0x0}, {108144, kWord, 0x0},
    {108142, kHalf, 0x0}, {108140, kHalf, 0x0}, {108138, kHalf, 0x0}, {108136, kHalf, 0x0},
    {108156, kWord, 0x0}, {108154, kHalf, 0x0}, {108152, kHalf, 0x0}, {108150, kHalf, 0x0},
    {108148, kHalf, 0x0}, {108168, kWord, 0x0}, {108166, kHalf, 0x0}, {108164, kHalf, 0x0},
    {108162, kHalf, 0x0}, {108160, kHalf, 0x0}, {108180, kWord, 0x0}, {108178, kHalf, 0x0},
    {108176, kHalf, 0x0}, {108174, kHalf, 0x0}, {108172, kHalf, 0x0}, {108192, kWord, 0x0},
    {108190, kHalf, 0x0}, {108188, kHalf, 0x0}, {108186, kHalf, 0x0}, {108184, kHalf, 0x0},
    {108204, kWord, 0x0}, {108202, kHalf, 0x0}, {108200, kHalf, 0x0}, {108198, kHalf, 0x0},
    {108196, kHalf, 0x0}, {108216, kWord, 0x0}, {108214, kHalf, 0x0}, {108212, kHalf, 0x0},
    {108210, kHalf, 0x0}, {108208, kHalf, 0x0}, {108228, kWord, 0x0}, {108226, kHalf, 0x0},
    {108224, kHalf, 0x0}, {108222, kHalf, 0x0}, {108220, kHalf, 0x0}, {108240, kWord, 0x0},
    {108238, kHalf, 0x0}, {108236, kHalf, 0x0}, {108234, kHalf, 0x0}, {108232, kHalf, 0x0},
    {108252, kWord, 0x0}, {108250, kHalf, 0x0}, {108248, kHalf, 0x0}, {108246, kHalf, 0x0},
    {108244, kHalf, 0x0}, {108264, kWord, 0x0}, {108262, kHalf, 0x0}, {108260, kHalf, 0x0},
    {108258, kHalf, 0x0}, {108256, kHalf, 0x0}, {108276, kWord, 0x0}, {108274, kHalf, 0x0},
    {108272, kHalf, 0x0}, {108270, kHalf, 0x0}, {108268, kHalf, 0x0}, {108288, kWord, 0x0},
    {108286, kHalf, 0x0}, {108284, kHalf, 0x0}, {108282, kHalf, 0x0}, {108280, kHalf, 0x0},
    {108300, kWord, 0x0}, {108298, kHalf, 0x0}, {108296, kHalf, 0x0}, {108294, kHalf, 0x0},
    {108292, kHalf, 0x0}, {108312, kWord, 0x0}, {108310, kHalf, 0x0}, {108308, kHalf, 0x0},
    {108306, kHalf, 0x0}, {108304, kHalf, 0x0}, {108324, kWord, 0x0}, {108322, kHalf, 0x0},
    {108320, kHalf, 0x0}, {108318, kHalf, 0x0}, {108316, kHalf, 0x0}, {108336, kWord, 0x0},
    {108334, kHalf, 0x0}, {108332, kHalf, 0x0}, {108330, kHalf, 0x0}, {108328, kHalf, 0x0},
    {108348, kWord, 0x0}, {108346, kHalf, 0x0}, {108344, kHalf, 0x0}, {108342, kHalf, 0x0},
    {108340, kHalf, 0x0}, {108360, kWord, 0x0}, {108358, kHalf, 0x0}, {108356, kHalf, 0x0},
    {108354, kHalf, 0x0}, {108352, kHalf, 0x0}, {108372, kWord, 0x0}, {108370, kHalf, 0x0},
    {108368, kHalf, 0x0}, {108366, kHalf, 0x0}, {108364, kHalf, 0x0}, {108384, kWord, 0x0},
    {108382, kHalf, 0x0}, {108380, kHalf, 0x0}, {108378, kHalf, 0x0}, {108376, kHalf, 0x0},
    {108396, kWord, 0x0}, {108394, kHalf, 0x0}, {108392, kHalf, 0x0}, {108390, kHalf, 0x0},
    {108388, kHalf, 0x0}, {108408, kWord, 0x0}, {108406, kHalf, 0x0}, {108404, kHalf, 0x0},
    {108402, kHalf, 0x0}, {108400, kHalf, 0x0}, {108420, kWord, 0x0}, {108418, kHalf, 0x0},
    {108416, kHalf, 0x0}, {108414, kHalf, 0x0}, {108412, kHalf, 0x0}, {108432, kWord, 0x0},
    {108430, kHalf, 0x0}, {108428, kHalf, 0x0}, {108426, kHalf, 0x0}, {108424, kHalf, 0x0},
    {108444, kWord, 0x0}, {108442, kHalf, 0x0}, {108440, kHalf, 0x0}, {108438, kHalf, 0x0},
    {108436, kHalf, 0x0}, {108456, kWord, 0x0}, {108454, kHalf, 0x0}, {108452, kHalf, 0x0},
    {108450, kHalf, 0x0}, {108448, kHalf, 0x0}, {108468, kWord, 0x0}, {108466, kHalf, 0x0},
    {108464, kHalf, 0x0}, {108462, kHalf, 0x0}, {108460, kHalf, 0x0}, {108480, kWord, 0x0},
    {108478, kHalf, 0x0}, {108476, kHalf, 0x0}, {108474, kHalf, 0x0}, {108472, kHalf, 0x0},
    {108492, kWord, 0x0}, {108490, kHalf, 0x0}, {108488, kHalf, 0x0}, {108486, kHalf, 0x0},
    {108484, kHalf, 0x0}, {108504, kWord, 0x0}, {108502, kHalf, 0x0}, {108500, kHalf, 0x0},
    {108498, kHalf, 0x0}, {108496, kHalf, 0x0}, {108516, kWord, 0x0}, {108514, kHalf, 0x0},
    {108512, kHalf, 0x0}, {108510, kHalf, 0x0}, {108508, kHalf, 0x0}, {108528, kWord, 0x0},
    {108526, kHalf, 0x0}, {108524, kHalf, 0x0}, {108522, kHalf, 0x0}, {108520, kHalf, 0x0},
    {108540, kWord, 0x0}, {108538, kHalf, 0x0}, {108536, kHalf, 0x0}, {108534, kHalf, 0x0},
    {108532, kHalf, 0x0}, {108552, kWord, 0x0}, {108550, kHalf, 0x0}, {108548, kHalf, 0x0},
    {108546, kHalf, 0x0}, {108544, kHalf, 0x0}, {108564, kWord, 0x0}, {108562, kHalf, 0x0},
    {108560, kHalf, 0x0}, {108558, kHalf, 0x0}, {108556, kHalf, 0x0}, {108576, kWord, 0x0},
    {108574, kHalf, 0x0}, {108572, kHalf, 0x0}, {108570, kHalf, 0x0}, {108568, kHalf, 0x0},
    {108588, kWord, 0x0}, {108586, kHalf, 0x0}, {108584, kHalf, 0x0}, {108582, kHalf, 0x0},
    {108580, kHalf, 0x0}, {108600, kWord, 0x0}, {108598, kHalf, 0x0}, {108596, kHalf, 0x0},
    {108594, kHalf, 0x0}, {108592, kHalf, 0x0}, {108612, kWord, 0x0}, {108610, kHalf, 0x0},
    {108608, kHalf, 0x0}, {108606, kHalf, 0x0}, {108604, kHalf, 0x0}, {108624, kWord, 0x0},
    {108622, kHalf, 0x0}, {108620, kHalf, 0x0}, {108618, kHalf, 0x0}, {108616, kHalf, 0x0},
    {108636, kWord, 0x0}, {108634, kHalf, 0x0}, {108632, kHalf, 0x0}, {108630, kHalf, 0x0},
    {108628, kHalf, 0x0}, {108648, kWord, 0x0}, {108646, kHalf, 0x0}, {108644, kHalf, 0x0},
    {108642, kHalf, 0x0}, {108640, kHalf, 0x0}, {108660, kWord, 0x0}, {108658, kHalf, 0x0},
    {108656, kHalf, 0x0}, {108654, kHalf, 0x0}, {108652, kHalf, 0x0}, {108672, kWord, 0x0},
    {108670, kHalf, 0x0}, {108668, kHalf, 0x0}, {108666, kHalf, 0x0}, {108664, kHalf, 0x0},
    {108684, kWord, 0x0}, {108682, kHalf, 0x0}, {108680, kHalf, 0x0}, {108678, kHalf, 0x0},
    {108676, kHalf, 0x0}, {108696, kWord, 0x0}, {108694, kHalf, 0x0}, {108692, kHalf, 0x0},
    {108690, kHalf, 0x0}, {108688, kHalf, 0x0}, {108708, kWord, 0x0}, {108706, kHalf, 0x0},
    {108704, kHalf, 0x0}, {108702, kHalf, 0x0}, {108700, kHalf, 0x0}, {108720, kWord, 0x0},
    {108718, kHalf, 0x0}, {108716, kHalf, 0x0}, {108714, kHalf, 0x0}, {108712, kHalf, 0x0},
    {108732, kWord, 0x0}, {108730, kHalf, 0x0}, {108728, kHalf, 0x0}, {108726, kHalf, 0x0},
    {108724, kHalf, 0x0}, {108744, kWord, 0x0}, {108742, kHalf, 0x0}, {108740, kHalf, 0x0},
    {108738, kHalf, 0x0}, {108736, kHalf, 0x0}, {108756, kWord, 0x0}, {108754, kHalf, 0x0},
    {108752, kHalf, 0x0}, {108750, kHalf, 0x0}, {108748, kHalf, 0x0}, {108768, kWord, 0x0},
    {108766, kHalf, 0x0}, {108764, kHalf, 0x0}, {108762, kHalf, 0x0}, {108760, kHalf, 0x0},
    {108780, kWord, 0x0}, {108778, kHalf, 0x0}, {108776, kHalf, 0x0}, {108774, kHalf, 0x0},
    {108772, kHalf, 0x0}, {108792, kWord, 0x0}, {108790, kHalf, 0x0}, {108788, kHalf, 0x0},
    {108786, kHalf, 0x0}, {108784, kHalf, 0x0}, {108804, kWord, 0x0}, {108802, kHalf, 0x0},
    {108800, kHalf, 0x0}, {108798, kHalf, 0x0}, {108796, kHalf, 0x0}, {108816, kWord, 0x0},
    {108814, kHalf, 0x0}, {108812, kHalf, 0x0}, {108810, kHalf, 0x0}, {108808, kHalf, 0x0},
    {108828, kWord, 0x0}, {108826, kHalf, 0x0}, {108824, kHalf, 0x0}, {108822, kHalf, 0x0},
    {108820, kHalf, 0x0}, {108840, kWord, 0x0}, {108838, kHalf, 0x0}, {108836, kHalf, 0x0},
    {108834, kHalf, 0x0}, {108832, kHalf, 0x0}, {108852, kWord, 0x0}, {108850, kHalf, 0x0},
    {108848, kHalf, 0x0}, {108846, kHalf, 0x0}, {108844, kHalf, 0x0}, {108864, kWord, 0x0},
    {108862, kHalf, 0x0}, {108860, kHalf, 0x0}, {108858, kHalf, 0x0}, {108856, kHalf, 0x0},
    {108876, kWord, 0x0}, {108874, kHalf, 0x0}, {108872, kHalf, 0x0}, {108870, kHalf, 0x0},
    {108868, kHalf, 0x0}, {108888, kWord, 0x0}, {108886, kHalf, 0x0}, {108884, kHalf, 0x0},
    {108882, kHalf, 0x0}, {108880, kHalf, 0x0}, {108900, kWord, 0x0}, {108898, kHalf, 0x0},
    {108896, kHalf, 0x0}, {108894, kHalf, 0x0}, {108892, kHalf, 0x0}, {108912, kWord, 0x0},
    {108910, kHalf, 0x0}, {108908, kHalf, 0x0}, {108906, kHalf, 0x0}, {108904, kHalf, 0x0},
    {108924, kWord, 0x0}, {108922, kHalf, 0x0}, {108920, kHalf, 0x0}, {108918, kHalf, 0x0},
    {108916, kHalf, 0x0}, {108936, kWord, 0x0}, {108934, kHalf, 0x0}, {108932, kHalf, 0x0},
    {108930, kHalf, 0x0}, {108928, kHalf, 0x0}, {108948, kWord, 0x0}, {108946, kHalf, 0x0},
    {108944, kHalf, 0x0}, {108942, kHalf, 0x0}, {108940, kHalf, 0x0}, {108960, kWord, 0x0},
    {108958, kHalf, 0x0}, {108956, kHalf, 0x0}, {108954, kHalf, 0x0}, {108952, kHalf, 0x0},
    {108972, kWord, 0x0}, {108970, kHalf, 0x0}, {108968, kHalf, 0x0}, {108966, kHalf, 0x0},
    {108964, kHalf, 0x0}, {108984, kWord, 0x0}, {108982, kHalf, 0x0}, {108980, kHalf, 0x0},
    {108978, kHalf, 0x0}, {108976, kHalf, 0x0}, {108996, kWord, 0x0}, {108994, kHalf, 0x0},
    {108992, kHalf, 0x0}, {108990, kHalf, 0x0}, {108988, kHalf, 0x0}, {109008, kWord, 0x0},
    {109006, kHalf, 0x0}, {109004, kHalf, 0x0}, {109002, kHalf, 0x0}, {109000, kHalf, 0x0},
    {109020, kWord, 0x0}, {109018, kHalf, 0x0}, {109016, kHalf, 0x0}, {109014, kHalf, 0x0},
    {109012, kHalf, 0x0}, {109032, kWord, 0x0}, {109030, kHalf, 0x0}, {109028, kHalf, 0x0},
    {109026, kHalf, 0x0}, {109024, kHalf, 0x0}, {109044, kWord, 0x0}, {109042, kHalf, 0x0},
    {109040, kHalf, 0x0}, {109038, kHalf, 0x0}, {109036, kHalf, 0x0}, {109056, kWord, 0x0},
    {109054, kHalf, 0x0}, {109052, kHalf, 0x0}, {109050, kHalf, 0x0}, {109048, kHalf, 0x0},
    {109068, kWord, 0x0}, {109066, kHalf, 0x0}, {109064, kHalf, 0x0}, {109062, kHalf, 0x0},
    {109060, kHalf, 0x0}, {109080, kWord, 0x0}, {109078, kHalf, 0x0}, {109076, kHalf, 0x0},
    {109074, kHalf, 0x0}, {109072, kHalf, 0x0}, {109092, kWord, 0x0}, {109090, kHalf, 0x0},
    {109088, kHalf, 0x0}, {109086, kHalf, 0x0}, {109084, kHalf, 0x0}, {109104, kWord, 0x0},
    {109102, kHalf, 0x0}, {109100, kHalf, 0x0}, {109098, kHalf, 0x0}, {109096, kHalf, 0x0},
    {109116, kWord, 0x0}, {109114, kHalf, 0x0}, {109112, kHalf, 0x0}, {109110, kHalf, 0x0},
    {109108, kHalf, 0x0}, {109128, kWord, 0x0}, {109126, kHalf, 0x0}, {109124, kHalf, 0x0},
    {109122, kHalf, 0x0}, {109120, kHalf, 0x0}, {109140, kWord, 0x0}, {109138, kHalf, 0x0},
    {109136, kHalf, 0x0}, {109134, kHalf, 0x0}, {109132, kHalf, 0x0}, {109152, kWord, 0x0},
    {109150, kHalf, 0x0}, {109148, kHalf, 0x0}, {109146, kHalf, 0x0}, {109144, kHalf, 0x0},
    {109164, kWord, 0x0}, {109162, kHalf, 0x0}, {109160, kHalf, 0x0}, {109158, kHalf, 0x0},
    {109156, kHalf, 0x0}, {109176, kWord, 0x0}, {109174, kHalf, 0x0}, {109172, kHalf, 0x0},
    {109170, kHalf, 0x0}, {109168, kHalf, 0x0}, {109188, kWord, 0x0}, {109186, kHalf, 0x0},
    {109184, kHalf, 0x0}, {109182, kHalf, 0x0}, {109180, kHalf, 0x0}, {109200, kWord, 0x0},
    {109198, kHalf, 0x0}, {109196, kHalf, 0x0}, {109194, kHalf, 0x0}, {109192, kHalf, 0x0},
    {109212, kWord, 0x0}, {109210, kHalf, 0x0}, {109208, kHalf, 0x0}, {109206, kHalf, 0x0},
    {109204, kHalf, 0x0}, {109224, kWord, 0x0}, {109222, kHalf, 0x0}, {109220, kHalf, 0x0},
    {109218, kHalf, 0x0}, {109216, kHalf, 0x0}, {109236, kWord, 0x0}, {109234, kHalf, 0x0},
    {109232, kHalf, 0x0}, {109230, kHalf, 0x0}, {109228, kHalf, 0x0}, {109248, kWord, 0x0},
    {109246, kHalf, 0x0}, {109244, kHalf, 0x0}, {109242, kHalf, 0x0}, {109240, kHalf, 0x0},
    {109260, kWord, 0x0}, {109258, kHalf, 0x0}, {109256, kHalf, 0x0}, {109254, kHalf, 0x0},
    {109252, kHalf, 0x0}, {109272, kWord, 0x0}, {109270, kHalf, 0x0}, {109268, kHalf, 0x0},
    {109266, kHalf, 0x0}, {109264, kHalf, 0x0}, {109284, kWord, 0x0}, {109282, kHalf, 0x0},
    {109280, kHalf, 0x0}, {109278, kHalf, 0x0}, {109276, kHalf, 0x0}, {109296, kWord, 0x0},
    {109294, kHalf, 0x0}, {109292, kHalf, 0x0}, {109290, kHalf, 0x0}, {109288, kHalf, 0x0},
    {109308, kWord, 0x0}, {109306, kHalf, 0x0}, {109304, kHalf, 0x0}, {109302, kHalf, 0x0},
    {109300, kHalf, 0x0}, {109320, kWord, 0x0}, {109318, kHalf, 0x0}, {109316, kHalf, 0x0},
    {109314, kHalf, 0x0}, {109312, kHalf, 0x0}, {109332, kWord, 0x0}, {109330, kHalf, 0x0},
    {109328, kHalf, 0x0}, {109326, kHalf, 0x0}, {109324, kHalf, 0x0}, {109344, kWord, 0x0},
    {109342, kHalf, 0x0}, {109340, kHalf, 0x0}, {109338, kHalf, 0x0}, {109336, kHalf, 0x0},
    {109356, kWord, 0x0}, {109354, kHalf, 0x0}, {109352, kHalf, 0x0}, {109350, kHalf, 0x0},
    {109348, kHalf, 0x0}, {109368, kWord, 0x0}, {109366, kHalf, 0x0}, {109364, kHalf, 0x0},
    {109362, kHalf, 0x0}, {109360, kHalf, 0x0}, {109380, kWord, 0x0}, {109378, kHalf, 0x0},
    {109376, kHalf, 0x0}, {109374, kHalf, 0x0}, {109372, kHalf, 0x0}, {109392, kWord, 0x0},
    {109390, kHalf, 0x0}, {109388, kHalf, 0x0}, {109386, kHalf, 0x0}, {109384, kHalf, 0x0},
    {109404, kWord, 0x0}, {109402, kHalf, 0x0}, {109400, kHalf, 0x0}, {109398, kHalf, 0x0},
    {109396, kHalf, 0x0}, {109416, kWord, 0x0}, {109414, kHalf, 0x0}, {109412, kHalf, 0x0},
    {109410, kHalf, 0x0}, {109408, kHalf, 0x0}, {109428, kWord, 0x0}, {109426, kHalf, 0x0},
    {109424, kHalf, 0x0}, {109422, kHalf, 0x0}, {109420, kHalf, 0x0}, {109440, kWord, 0x0},
    {109438, kHalf, 0x0}, {109436, kHalf, 0x0}, {109434, kHalf, 0x0}, {109432, kHalf, 0x0},
    {109452, kWord, 0x0}, {109450, kHalf, 0x0}, {109448, kHalf, 0x0}, {109446, kHalf, 0x0},
    {109444, kHalf, 0x0}, {109464, kWord, 0x0}, {109462, kHalf, 0x0}, {109460, kHalf, 0x0},
    {109458, kHalf, 0x0}, {109456, kHalf, 0x0}, {109476, kWord, 0x0}, {109474, kHalf, 0x0},
    {109472, kHalf, 0x0}, {109470, kHalf, 0x0}, {109468, kHalf, 0x0}, {109488, kWord, 0x0},
    {109486, kHalf, 0x0}, {109484, kHalf, 0x0}, {109482, kHalf, 0x0}, {109480, kHalf, 0x0},
    {109500, kWord, 0x0}, {109498, kHalf, 0x0}, {109496, kHalf, 0x0}, {109494, kHalf, 0x0},
    {109492, kHalf, 0x0}, {109512, kWord, 0x0}, {109510, kHalf, 0x0}, {109508, kHalf, 0x0},
    {109506, kHalf, 0x0}, {109504, kHalf, 0x0}, {109524, kWord, 0x0}, {109522, kHalf, 0x0},
    {109520, kHalf, 0x0}, {109518, kHalf, 0x0}, {109516, kHalf, 0x0}, {109536, kWord, 0x0},
    {109534, kHalf, 0x0}, {109532, kHalf, 0x0}, {109530, kHalf, 0x0}, {109528, kHalf, 0x0},
    {109548, kWord, 0x0}, {109546, kHalf, 0x0}, {109544, kHalf, 0x0}, {109542, kHalf, 0x0},
    {109540, kHalf, 0x0}, {109592, kWord, 0x0, 2, 160}, {109852, kWord, 0x0, 5, 100}, {110256, kWord, 0x0, 1536, 4},
    {119900, kWord, 0xFFFFFFFF}, {117168, kWord, 0x0, 3, 4}, {119904, kWord, 0x0}, {119868, kWord, 0x0, 8, 4},
};
constexpr Fill kOptionsDefaults0[] = {
    {16, kWord, 0x1}, {0, kWord, 0x1, 3, 4}, {20, kWord, 0x1}, {12, kWord, 0x1},
    {24, kWord, 0x2}, {36, kByte, 0x0, 6, 1}, {28, kByte, 0xF, 2, 1}, {32, kWord, 0x0},
    {44, kWord, 0x12}, {48, kWord, 0xE}, {52, kWord, 0xD}, {56, kWord, 0x9},
    {60, kWord, 0x8}, {64, kWord, 0xA}, {68, kWord, 0x10}, {72, kWord, 0xF},
    {76, kWord, 0x18}, {80, kWord, 0x7}, {84, kWord, 0x11}, {88, kWord, 0x0},
    {92, kWord, 0x19}, {96, kWord, 0xC}, {100, kWord, 0xB}, {104, kWord, 0x12},
    {108, kWord, 0xE}, {112, kWord, 0xD}, {116, kWord, 0x9}, {120, kWord, 0x8},
    {124, kWord, 0xA}, {128, kWord, 0x10}, {132, kWord, 0xF}, {136, kWord, 0x18},
    {140, kWord, 0x7}, {144, kWord, 0x11}, {148, kWord, 0x0}, {152, kWord, 0x19},
    {156, kWord, 0xC}, {160, kWord, 0xB}, {164, kWord, 0x12}, {168, kWord, 0xE},
    {172, kWord, 0xD}, {176, kWord, 0x9}, {180, kWord, 0x8}, {184, kWord, 0xA},
    {188, kWord, 0x10}, {192, kWord, 0xF}, {196, kWord, 0x18}, {200, kWord, 0x7},
    {204, kWord, 0x11}, {208, kWord, 0x0}, {212, kWord, 0x19}, {216, kWord, 0xC},
    {220, kWord, 0xB}, {224, kWord, 0x12}, {228, kWord, 0xE}, {232, kWord, 0xD},
    {236, kWord, 0x9}, {240, kWord, 0x8}, {244, kWord, 0xA}, {248, kWord, 0x10},
    {252, kWord, 0xF}, {256, kWord, 0x18}, {260, kWord, 0x7}, {264, kWord, 0x11},
    {268, kWord, 0x0}, {272, kWord, 0x19}, {276, kWord, 0xC}, {280, kWord, 0xB},
};

// Stats::Reset (sub_82113880); returns the original's r3.
uint32_t StatsReset(uint8_t* base, uint32_t object) {
  guest_memory::Clear(base, object + 0, 6044);
  guest_memory::Clear(base, object + 0, 24);
  guest_memory::Clear(base, object + 24, 4828);
  guest_memory::Apply(base, object, kStatsReset0);
  guest_memory::Clear(base, object + 4864, 24);
  guest_memory::Apply(base, object, kStatsReset1);
  guest_memory::Clear(base, object + 4924, 24);
  guest_memory::Apply(base, object, kStatsReset2);
  return object + 4948;
}

// MenuList::Init (sub_82209868): r3 the list, r4 its count, r5 the most
// shown, r6 to r10 and the two words at the caller's r1 + 84 and + 92
// (+64, +36 to +56); the cursor's words cleared, its flags (one word per
// entry, 1 each) VirtualAlloc'd anew (the old ones VirtualFree'd).
void MenuListInit(PPCContext& ctx, uint8_t* base, uint32_t list, uint32_t count,
                  uint32_t shown, uint32_t r6, uint32_t r7, uint32_t r8, uint32_t r9,
                  uint32_t r10, uint32_t a9, uint32_t a10) {
  StoreU32(base, list + 0, count);
  StoreU32(base, list + 4, 0);
  StoreU32(base, list + 8, 0);
  StoreU32(base, list + 16, 0);
  StoreU32(base, list + 20, 0);
  StoreU32(base, list + 12, int32_t(count) < int32_t(shown) ? count : shown);
  const uint32_t old = LoadU32(base, list + 68);
  StoreU32(base, list + 36, r7);
  StoreU32(base, list + 40, r8);
  StoreU32(base, list + 44, r9);
  StoreU32(base, list + 48, r10);
  StoreU32(base, list + 52, a9);
  StoreU32(base, list + 56, a10);
  StoreU32(base, list + 64, r6);
  StoreU32(base, list + 24, 0);
  StoreU32(base, list + 28, 0);
  if (old != 0) {
    ctx.r3.u64 = old;
    ctx.r4.u64 = 0;
    ctx.r5.u64 = 0x8000;  // MEM_RELEASE
    sub_82292D70(ctx, base);
  }
  ctx.r3.u64 = 0;
  ctx.r4.u64 = LoadU32(base, list + 0) << 2;
  ctx.r5.u64 = 4096;  // MEM_COMMIT
  ctx.r6.u64 = 4;     // PAGE_READWRITE
  sub_82292DD8(ctx, base);
  StoreU32(base, list + 68, ctx.r3.u32);
  for (int32_t i = 0; i < int32_t(LoadU32(base, list + 0)); i++) {
    StoreU32(base, LoadU32(base, list + 68) + 4 * uint32_t(i), 1);
  }
}

// SaveData::Delete (sub_82105748): the slot's save freed (operator delete,
// sub_8258C868) and its word cleared.
void SaveDelete(PPCContext& ctx, uint8_t* base, uint32_t slot) {
  ctx.r3.u64 = LoadU32(base, kSaves + 4 * slot);
  sub_8258C868(ctx, base);
  StoreU32(base, kSaves + 4 * slot, 0);
}

// SaveData::Create (sub_82105938): a save from operator new (sub_8258C000),
// its stats made (Stats::Stats on +48), stored to the slot (0 when the
// allocation failed, as the original goes on with it), then the course
// records reset, Progress::SetDefaults, the options' defaults at +128916
// and sub_82105B48 on +128904.
void SaveCreate(PPCContext& ctx, uint8_t* base, uint32_t slot) {
  ctx.r3.u64 = kSaveBytes;
  sub_8258C000(ctx, base);
  const uint32_t save = ctx.r3.u32;
  if (save != 0) {
    guest_memory::Apply(base, save + 48, kStats0);
  }
  StoreU32(base, kSaves + 4 * slot, save);
  ctx.r3.u64 = save + 119908;
  sub_82105E98(ctx, base);
  ctx.r3.u64 = LoadU32(base, kSaves + 4 * slot);
  sub_82113FB0(ctx, base);
  ctx.r3.u64 = LoadU32(base, kSaves + 4 * slot) + 128916;
  sub_82105AB8(ctx, base);
  ctx.r3.u64 = LoadU32(base, kSaves + 4 * slot) + 128904;
  sub_82105B48(ctx, base);
}

// Players::ResetControllers (sub_8209F718): controller i = i, then the
// flag set.
void ResetControllers(uint8_t* base) {
  for (uint32_t i = 0; i < 4; i++) {
    StoreU32(base, kControllers + 4 * i, i);
  }
  StoreU32(base, kControllersSet, 1);
}

bool On() { return REXCVAR_GET(native_save_data); }

}  // namespace

// Stats::Stats: r3 left.
extern "C" REX_FUNC(sub_82105830) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105830(ctx, base);
    return;
  }
  guest_memory::Apply(base, ctx.r3.u32, kStats0);
}

// Stats::ResetRecords: r3 left.
extern "C" REX_FUNC(sub_82113828) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82113828(ctx, base);
    return;
  }
  guest_memory::Apply(base, ctx.r3.u32, kStatsResetRecords0);
}

extern "C" REX_FUNC(sub_82113880) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82113880(ctx, base);
    return;
  }
  ctx.r3.u64 = StatsReset(base, ctx.r3.u32);
}

// Progress::Clear: the 119,908 bytes cleared, then its fields, the stats
// (+48) reset, and the rest; r3 as Stats::Reset leaves it.
extern "C" REX_FUNC(sub_82113D28) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82113D28(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  guest_memory::Clear(base, object + 0, 119908);
  guest_memory::Apply(base, object, kProgressClear0);
  const uint32_t r3 = StatsReset(base, object + 48);
  guest_memory::Apply(base, object, kProgressClear1);
  ctx.r3.u64 = r3;
}

// BitSet400::SetAll: the 13 words cleared, then bits 0 to 399 set (bit i
// in word i / 32 at 1 << (i % 32)): 12 full words and 0xFFFF.
extern "C" REX_FUNC(sub_821134F8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821134F8(ctx, base);
    return;
  }
  const uint32_t set = ctx.r3.u32;
  for (uint32_t i = 0; i < 12; i++) {
    StoreU32(base, set + 4 * i, 0xFFFFFFFFu);
  }
  StoreU32(base, set + 48, 0x0000FFFFu);
}

// ControlMap::SetDefault: the 15 actions' buttons.
extern "C" REX_FUNC(sub_82105A18) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105A18(ctx, base);
    return;
  }
  static constexpr uint32_t kButtons[15] = {18, 14, 13, 9, 8, 10, 16, 15, 24, 7, 17, 0, 25, 12, 11};
  for (uint32_t i = 0; i < 15; i++) {
    StoreU32(base, ctx.r3.u32 + 4 * i, kButtons[i]);
  }
}

// Options::SetDefaults: the options and their four control maps (inline);
// r3 as the last map leaves it.
extern "C" REX_FUNC(sub_82105AB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105AB8(ctx, base);
    return;
  }
  const uint32_t options = ctx.r3.u32;
  guest_memory::Apply(base, options, kOptionsDefaults0);
  ctx.r3.u64 = options + 224;
}

// CourseRecords::Reset: the header (+0 to +19) cleared, then the 22
// records' own reset (sub_82105E20).
extern "C" REX_FUNC(sub_82105E98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105E98(ctx, base);
    return;
  }
  const uint32_t records = ctx.r3.u32;
  for (uint32_t i = 0; i < 16; i += 4) {
    StoreU32(base, records + i, 0);
  }
  guest::StoreU16(base, records + 18, 0);
  guest::StoreU16(base, records + 16, 0);
  for (uint32_t i = 0; i < 22; i++) {
    ctx.r3.u64 = records + 20 + 408 * i;
    sub_82105E20(ctx, base);
  }
}

extern "C" REX_FUNC(sub_82105748) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105748(ctx, base);
    return;
  }
  SaveDelete(ctx, base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_82105938) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105938(ctx, base);
    return;
  }
  SaveCreate(ctx, base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_8209F718) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8209F718(ctx, base);
    return;
  }
  ResetControllers(base);
}

extern "C" REX_FUNC(sub_82209868) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82209868(ctx, base);
    return;
  }
  const uint32_t sp = ctx.r1.u32;
  MenuListInit(ctx, base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32,
               ctx.r8.u32, ctx.r9.u32, ctx.r10.u32, LoadU32(base, sp + 84),
               LoadU32(base, sp + 92));
}

// TitleScreen::ResetPlayer (sub_8221F7A8): the title's menu list set up
// (1 entry: 154, 153, 155, 156, 159, 158), the title state's +36 cleared,
// the four player flags set, save slot 0 deleted and made again, the
// controllers reset.
extern "C" REX_FUNC(sub_8221F7A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8221F7A8(ctx, base);
    return;
  }
  MenuListInit(ctx, base, LoadU32(base, kMenuList), 1, 1, 0, 154, 153, 155, 156, 159, 158);
  StoreU32(base, LoadU32(base, kTitleState) + 36, 0);
  for (uint32_t i = 0; i < 4; i++) {
    StoreU32(base, kPlayerFlags + 4 * i, 1);
  }
  SaveDelete(ctx, base, 0);
  SaveCreate(ctx, base, 0);
  ResetControllers(base);
}

// The defaults' tables' getters.
#define SBX_TABLE_GETTER(address, table)    \
  extern "C" REX_FUNC(sub_##address) {      \
    REX_FUNC_PROLOGUE();                    \
    if (!On()) {                            \
      __imp__sub_##address(ctx, base);      \
      return;                               \
    }                                       \
    ctx.r3.u64 = table;                     \
  }

SBX_TABLE_GETTER(820CCA60, 0x826B4D88)
SBX_TABLE_GETTER(820CCA70, 0x826B2D24)
SBX_TABLE_GETTER(820CCA80, 0x826B3B14)
SBX_TABLE_GETTER(820CCA90, 0x826B3904)
SBX_TABLE_GETTER(820CCAA0, 0x826B4DC4)

#undef SBX_TABLE_GETTER
