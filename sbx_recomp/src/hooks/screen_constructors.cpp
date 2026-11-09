// The front end's screen constructors as our own C++
// (--native_screen_constructors, on by default): the panels, panel groups
// and sets, node tables, list screens and node screens the type 6 and 7
// menus are made of, and the two screens' sets that make them.
//
// Written from SBX's generated code. Each constructor is a run of stores
// (vtables, defaults, links to its own members) and calls to its members'
// constructors; the stores are written as tables here, read from SBX's
// code in its order, and the calls go through the members' functions
// (ours where this file has them). Six stay recompiled: the two that
// allocate a member and test it (Type6PanelScreen sub_8222E480,
// Type7PanelScreen sub_8223DF38), the two that store through a member they
// load (Type6ListScreen sub_82230D80, Type7ListScreen sub_82240758),
// PanelSet::PanelSet (sub_822490A8, stores through its argument) and
// ColorListScreen::ColorListScreen (sub_82252A70, copies a word it loads).

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_screen_constructors, true, "SBX",
                    "Our own C++ for the front end's screen constructors (panels, groups, "
                    "sets, node tables, list and node screens) instead of the recompiled "
                    "functions");

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

// NodeBase::NodeBase (sub_82256928).
// 2 stores
constexpr Fill kNodeBase_NodeBase0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40},
};

// PanelGroupBase::PanelGroupBase (sub_8224C888).
// 10 stores
constexpr Fill kPanelGroupBase_PanelGroupBase0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40}, {0, kWord, 0x82010644}, {8, kWord, 0xFFFFFFFF, 7, 4},
};

// PanelCBase::PanelCBase (sub_8224B148).
// 75 stores
constexpr Fill kPanelCBase_PanelCBase0[] = {
    {4, kWord, 0x1}, {0, kWord, 0x82010E70}, {16, kWord, 0xFFFFFFFF}, {12, kWord, 0x0},
    {8, kWord, 0xFFFFFFFF, 2, 16}, {28, kWord, 0xFFFFFFFF, 11, 4}, {276, kWord, 0x0}, {72, kWord, 0xFFFFFFFF},
    {280, kWord, 0x0}, {76, kWord, 0xFFFFFFFF}, {284, kWord, 0x0}, {80, kWord, 0xFFFFFFFF},
    {288, kWord, 0x0}, {84, kWord, 0xFFFFFFFF}, {292, kWord, 0x0}, {88, kWord, 0xFFFFFFFF},
    {296, kWord, 0x0}, {92, kWord, 0xFFFFFFFF}, {300, kWord, 0x0}, {96, kWord, 0xFFFFFFFF},
    {304, kWord, 0x0}, {100, kWord, 0xFFFFFFFF}, {308, kWord, 0x0}, {104, kWord, 0xFFFFFFFF},
    {312, kWord, 0x0}, {108, kWord, 0xFFFFFFFF, 9, 4}, {148, kWord, 0xFFFFFFFF, 3, 4}, {260, kWord, 0xFFFFFFFF, 3, 4},
    {160, kWord, 0xFFFFFFFF, 22, 4}, {272, kWord, 0xFFFFFFFF}, {316, kWord, 0x0},
};

// PanelDBase::PanelDBase (sub_8224A9F0).
// 22 stores
constexpr Fill kPanelDBase_PanelDBase0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40}, {8, kWord, 0xFFFFFFFF, 5, 4}, {44, kWord, 0xFFFFFFFF, 2, 4},
    {28, kWord, 0xFFFFFFFF, 4, 4}, {52, kWord, 0xFFFFFFFF, 2, 4}, {68, kWord, 0xFFFFFFFF}, {64, kWord, 0xFFFFFFFF, 2, 16},
    {60, kWord, 0xFFFFFFFF, 2, 12}, {76, kWord, 0xFFFFFFFF}, {88, kWord, 0x0},
};

// PanelEBase::PanelEBase (sub_8224C958).
// 40 stores
constexpr Fill kPanelEBase_PanelEBase0[] = {
    {12, kWord, 0xFFFFFFFF}, {0, kWord, 0x0}, {60, kWord, 0xFFFFFFFF, 23, 4}, {8, kWord, 0xFFFFFFFF},
    {4, kWord, 0x0}, {16, kWord, 0xFFFFFFFF, 11, 4}, {152, kWord, 0x0, 2, 4},
};

// PanelE::PanelE (sub_8222E2A0).
// 68 stores
constexpr Fill kPanelE_PanelE0[] = {
    {12, kWord, 0xFFFFFFFF}, {0, kWord, 0x0}, {60, kWord, 0xFFFFFFFF, 23, 4}, {8, kWord, 0xFFFFFFFF},
    {4, kWord, 0x0}, {16, kWord, 0xFFFFFFFF, 11, 4}, {152, kWord, 0x0, 2, 4}, {0, kArg, 4},
    {60, kWord, 0x9}, {4, kWord, 0x4, 2, 12}, {72, kWord, 0x10}, {80, kWord, 0x51},
    {88, kWord, 0x52}, {76, kWord, 0x50}, {68, kWord, 0xF}, {100, kWord, 0x44},
    {112, kWord, 0x2}, {116, kWord, 0x53}, {20, kWord, 0x5}, {24, kWord, 0x6},
    {28, kWord, 0x7}, {32, kWord, 0x8}, {64, kWord, 0x5}, {92, kWord, 0x3E},
    {96, kWord, 0x43}, {104, kWord, 0x48}, {120, kWord, 0x3}, {124, kWord, 0x5},
    {128, kWord, 0x6}, {132, kWord, 0x7}, {136, kWord, 0x8}, {140, kWord, 0xFFFFFFFF},
    {144, kWord, 0x1}, {148, kWord, 0x3},
};

// PanelFBase::PanelFBase (sub_8224D468).
// 32 stores
constexpr Fill kPanelFBase_PanelFBase0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40}, {24, kWord, 0xFFFFFFFF, 18, 4}, {116, kWord, 0xFFFFFFFF, 4, 4},
    {96, kWord, 0xFFFFFFFF, 5, 4}, {132, kWord, 0xFFFFFFFF, 2, 4}, {140, kWord, 0x0},
};

// Panel6C::Panel6C (sub_8222F730).
// 111 stores
constexpr Fill kPanel6C_Panel6C0[] = {
    {4, kWord, 0x1}, {0, kWord, 0x82010E70}, {16, kWord, 0xFFFFFFFF}, {12, kWord, 0x0},
    {8, kWord, 0xFFFFFFFF, 2, 16}, {28, kWord, 0xFFFFFFFF, 11, 4}, {276, kWord, 0x0}, {72, kWord, 0xFFFFFFFF},
    {280, kWord, 0x0}, {76, kWord, 0xFFFFFFFF}, {284, kWord, 0x0}, {80, kWord, 0xFFFFFFFF},
    {288, kWord, 0x0}, {84, kWord, 0xFFFFFFFF}, {292, kWord, 0x0}, {88, kWord, 0xFFFFFFFF},
    {296, kWord, 0x0}, {92, kWord, 0xFFFFFFFF}, {300, kWord, 0x0}, {96, kWord, 0xFFFFFFFF},
    {304, kWord, 0x0}, {100, kWord, 0xFFFFFFFF}, {308, kWord, 0x0}, {104, kWord, 0xFFFFFFFF},
    {312, kWord, 0x0}, {108, kWord, 0xFFFFFFFF, 9, 4}, {148, kWord, 0xFFFFFFFF, 3, 4}, {260, kWord, 0xFFFFFFFF, 3, 4},
    {160, kWord, 0xFFFFFFFF, 22, 4}, {272, kWord, 0xFFFFFFFF}, {316, kWord, 0x0}, {12, kArg, 4},
    {20, kWord, 0x7}, {0, kWord, 0x82010634}, {168, kWord, 0x7}, {216, kWord, 0x9},
    {32, kWord, 0x5, 2, 112}, {172, kWord, 0x8}, {160, kWord, 0x5}, {176, kWord, 0xB},
    {180, kWord, 0xC}, {224, kWord, 0xD}, {228, kWord, 0xE}, {132, kWord, 0xD},
    {136, kWord, 0xE}, {44, kWord, 0xFFFFFFFF, 4, 4}, {68, kWord, 0x5}, {40, kWord, 0x6D},
    {4, kWord, 0x0, 2, 20}, {164, kWord, 0x6}, {220, kWord, 0xA}, {152, kWord, 0x0},
    {140, kWord, 0xF}, {272, kWord, 0x4}, {60, kWord, 0x70}, {72, kWord, 0x6},
    {108, kWord, 0xF}, {112, kWord, 0x10}, {124, kWord, 0x0}, {128, kWord, 0x2B},
    {276, kWord, 0x7B}, {280, kWord, 0x70},
};

// Panel7C::Panel7C (sub_8223F270).
// 111 stores
constexpr Fill kPanel7C_Panel7C0[] = {
    {4, kWord, 0x1}, {0, kWord, 0x82010E70}, {16, kWord, 0xFFFFFFFF}, {12, kWord, 0x0},
    {8, kWord, 0xFFFFFFFF, 2, 16}, {28, kWord, 0xFFFFFFFF, 11, 4}, {276, kWord, 0x0}, {72, kWord, 0xFFFFFFFF},
    {280, kWord, 0x0}, {76, kWord, 0xFFFFFFFF}, {284, kWord, 0x0}, {80, kWord, 0xFFFFFFFF},
    {288, kWord, 0x0}, {84, kWord, 0xFFFFFFFF}, {292, kWord, 0x0}, {88, kWord, 0xFFFFFFFF},
    {296, kWord, 0x0}, {92, kWord, 0xFFFFFFFF}, {300, kWord, 0x0}, {96, kWord, 0xFFFFFFFF},
    {304, kWord, 0x0}, {100, kWord, 0xFFFFFFFF}, {308, kWord, 0x0}, {104, kWord, 0xFFFFFFFF},
    {312, kWord, 0x0}, {108, kWord, 0xFFFFFFFF, 9, 4}, {148, kWord, 0xFFFFFFFF, 3, 4}, {260, kWord, 0xFFFFFFFF, 3, 4},
    {160, kWord, 0xFFFFFFFF, 22, 4}, {272, kWord, 0xFFFFFFFF}, {316, kWord, 0x0}, {12, kArg, 4},
    {32, kWord, 0x3}, {0, kWord, 0x82010B9C}, {176, kWord, 0xB}, {144, kWord, 0x5, 2, 16},
    {224, kWord, 0xD}, {168, kWord, 0x7}, {172, kWord, 0x8}, {228, kWord, 0xE},
    {132, kWord, 0xD}, {136, kWord, 0xE}, {44, kWord, 0xFFFFFFFF, 4, 4}, {68, kWord, 0x5},
    {40, kWord, 0x6D}, {4, kWord, 0x0}, {20, kWord, 0x6}, {24, kWord, 0x4},
    {164, kWord, 0x6}, {180, kWord, 0xC}, {216, kWord, 0x9}, {220, kWord, 0xA},
    {152, kWord, 0x0}, {140, kWord, 0xF}, {272, kWord, 0x4}, {60, kWord, 0x70},
    {72, kWord, 0x6}, {108, kWord, 0xF}, {112, kWord, 0x10}, {124, kWord, 0x0},
    {128, kWord, 0x2B}, {276, kWord, 0x7B}, {280, kWord, 0x70},
};

// Panel6D::Panel6D (sub_8222F8A0).
// 36 stores
constexpr Fill kPanel6D_Panel6D0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40}, {8, kWord, 0xFFFFFFFF, 5, 4}, {44, kWord, 0xFFFFFFFF, 2, 4},
    {28, kWord, 0xFFFFFFFF, 4, 4}, {52, kWord, 0xFFFFFFFF, 2, 4}, {68, kWord, 0xFFFFFFFF}, {64, kWord, 0xFFFFFFFF, 2, 16},
    {60, kWord, 0xFFFFFFFF, 2, 12}, {76, kWord, 0xFFFFFFFF}, {88, kWord, 0x0}, {4, kArg, 4},
    {12, kWord, 0x16}, {0, kWord, 0x82010644}, {60, kWord, 0x15}, {68, kWord, 0x1},
    {64, kWord, 0x0}, {80, kWord, 0x2}, {16, kWord, 0x6E}, {20, kWord, 0x6F},
    {24, kWord, 0x60}, {28, kWord, 0x69}, {32, kWord, 0x6A}, {36, kWord, 0x6B},
    {40, kWord, 0x6C},
};

// Panel7D::Panel7D (sub_8223F450).
// 36 stores
constexpr Fill kPanel7D_Panel7D0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40}, {8, kWord, 0xFFFFFFFF, 5, 4}, {44, kWord, 0xFFFFFFFF, 2, 4},
    {28, kWord, 0xFFFFFFFF, 4, 4}, {52, kWord, 0xFFFFFFFF, 2, 4}, {68, kWord, 0xFFFFFFFF}, {64, kWord, 0xFFFFFFFF, 2, 16},
    {60, kWord, 0xFFFFFFFF, 2, 12}, {76, kWord, 0xFFFFFFFF}, {88, kWord, 0x0}, {4, kArg, 4},
    {12, kWord, 0x14}, {0, kWord, 0x82010644}, {60, kWord, 0x13}, {68, kWord, 0x1},
    {64, kWord, 0x0}, {80, kWord, 0x2}, {16, kWord, 0x6E}, {20, kWord, 0x6F},
    {24, kWord, 0x5E}, {28, kWord, 0x67}, {32, kWord, 0x68}, {36, kWord, 0x69},
    {40, kWord, 0x6A},
};

// Panel6F::Panel6F (sub_8222E388).
// 54 stores
constexpr Fill kPanel6F_Panel6F0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40}, {24, kWord, 0xFFFFFFFF, 18, 4}, {116, kWord, 0xFFFFFFFF, 4, 4},
    {96, kWord, 0xFFFFFFFF, 5, 4}, {132, kWord, 0xFFFFFFFF, 2, 4}, {140, kWord, 0x0}, {4, kArg, 4},
    {28, kWord, 0x3}, {32, kWord, 0x6}, {0, kWord, 0x82010644}, {88, kWord, 0x1},
    {116, kWord, 0x4F}, {84, kWord, 0x4}, {120, kWord, 0x49}, {44, kWord, 0x5},
    {48, kWord, 0x6}, {96, kWord, 0x5}, {100, kWord, 0xF}, {36, kWord, 0x0, 2, 56},
    {132, kWord, 0x4B}, {52, kWord, 0x7}, {104, kWord, 0x10}, {108, kWord, 0x1D},
    {112, kWord, 0x1E}, {40, kWord, 0x0}, {124, kWord, 0x2}, {136, kWord, 0x53},
};

// Panel7F::Panel7F (sub_8223DE58).
// 54 stores
constexpr Fill kPanel7F_Panel7F0[] = {
    {4, kWord, 0x0}, {0, kWord, 0x82000A40}, {24, kWord, 0xFFFFFFFF, 18, 4}, {116, kWord, 0xFFFFFFFF, 4, 4},
    {96, kWord, 0xFFFFFFFF, 5, 4}, {132, kWord, 0xFFFFFFFF, 2, 4}, {140, kWord, 0x0}, {4, kArg, 4},
    {28, kWord, 0x3}, {0, kWord, 0x82010644}, {88, kWord, 0x1}, {32, kWord, 0x6},
    {84, kWord, 0x4}, {120, kWord, 0x49}, {44, kWord, 0x5}, {116, kWord, 0x4F},
    {136, kWord, 0x53}, {48, kWord, 0x6}, {96, kWord, 0x5}, {36, kWord, 0x0, 2, 56},
    {132, kWord, 0x4B}, {52, kWord, 0x7}, {100, kWord, 0xF}, {104, kWord, 0x10},
    {108, kWord, 0x1D}, {112, kWord, 0x1E}, {40, kWord, 0x0}, {124, kWord, 0x2},
};

// PanelGroup::PanelGroup (sub_8222E128).
// 4 stores
constexpr Fill kPanelGroup_PanelGroup0[] = {
    {4, kWord, 0x4}, {0, kWord, 0x82010630}, {12, kWord, 0x1}, {32, kWord, 0x0},
};

// PanelGroup6B::PanelGroup6B (sub_8222F6C8).
// 4 stores
constexpr Fill kPanelGroup6B_PanelGroup6B0[] = {
    {4, kWord, 0x1C}, {0, kWord, 0x82010630}, {12, kWord, 0x4}, {32, kWord, 0x0},
};

// PanelGroup7B::PanelGroup7B (sub_8223F218).
// 4 stores
constexpr Fill kPanelGroup7B_PanelGroup7B0[] = {
    {4, kWord, 0x1C}, {0, kWord, 0x82010630}, {12, kWord, 0x2}, {32, kWord, 0x0},
};

// PanelSetBase::PanelSetBase (sub_82248770).
// 47 stores
constexpr Fill kPanelSetBase_PanelSetBase0[] = {
    {0, kWord, 0x82010D74}, {128, kWord, 0x0}, {120, kWord, 0x0, 2, 80}, {192, kWord, 0x0, 2, 592},
    {1300, kWord, 0x0, 2, 516}, {1876, kWord, 0x0}, {224, kWord, 0xFFFFFFFF}, {204, kWord, 0x0, 5, 4},
    {228, kWord, 0x0, 6, 4}, {1916, kWord, 0x0}, {1920, kWord, 0x1}, {4, kWord, 0x0, 2, 8},
    {1928, kWord, 0x1}, {16, kWord, 0x0, 4, 4}, {0x8280EEB4, kAbsWord, 0x0}, {1932, kWord, 0xFFFFFFFF},
    {1936, kWord, 0x1, 2, 4}, {1944, kWord, 0x0}, {1948, kWord, 0x1}, {1952, kWord, 0xFFFFFFFF},
    {1892, kWord, 0x0, 6, 4}, {1956, kWord, 0x0}, {1960, kWord, 0xFFFFFFFF, 3, 4},
};

// GroupPair::GroupPair (sub_822477C0).
// 3 stores
constexpr Fill kGroupPair_GroupPair0[] = {
    {8, kArg, 4}, {12, kArg, 5}, {0, kWord, 0x82010D34},
};

// MachineScreen::MachineScreen (sub_8224A9A8).
// 2 stores
constexpr Fill kMachineScreen_MachineScreen0[] = {
    {9652, kWord, 0x0}, {0, kWord, 0x82010E18},
};

// Type7Screen0::Type7Screen0 (sub_82243B48).
// 1 stores
constexpr Fill kType7Screen0_Type7Screen00[] = {
    {0, kWord, 0x82000A40},
};

// NodeTable::NodeTable (sub_822457A0).
// 106 stores
constexpr Fill kNodeTable_NodeTable0[] = {
    {0, kWord, 0x82010CA4}, {8, kWord, 0xFFFFFFFF, 2, 4}, {296, kWord, 0xFFFFFFFF, 4, 4}, {16, kWord, 0xFFFFFFFF, 2, 140},
    {312, kWord, 0x0}, {20, kWord, 0xFFFFFFFF, 2, 140}, {316, kWord, 0x0}, {24, kWord, 0xFFFFFFFF, 2, 140},
    {320, kWord, 0x0}, {28, kWord, 0xFFFFFFFF, 2, 140}, {324, kWord, 0x0}, {32, kWord, 0xFFFFFFFF, 2, 140},
    {328, kWord, 0x0}, {36, kWord, 0xFFFFFFFF, 2, 140}, {332, kWord, 0x0}, {40, kWord, 0xFFFFFFFF, 2, 140},
    {336, kWord, 0x0}, {44, kWord, 0xFFFFFFFF, 2, 140}, {340, kWord, 0x0}, {48, kWord, 0xFFFFFFFF, 2, 140},
    {344, kWord, 0x0}, {52, kWord, 0xFFFFFFFF, 2, 140}, {348, kWord, 0x0}, {56, kWord, 0xFFFFFFFF, 2, 140},
    {352, kWord, 0x0}, {60, kWord, 0xFFFFFFFF, 2, 140}, {356, kWord, 0x0}, {64, kWord, 0xFFFFFFFF, 2, 140},
    {360, kWord, 0x0}, {68, kWord, 0xFFFFFFFF, 2, 140}, {364, kWord, 0x0}, {72, kWord, 0xFFFFFFFF, 2, 140},
    {368, kWord, 0x0}, {76, kWord, 0xFFFFFFFF, 2, 140}, {372, kWord, 0x0}, {80, kWord, 0xFFFFFFFF, 2, 140},
    {376, kWord, 0x0}, {84, kWord, 0xFFFFFFFF, 2, 140}, {380, kWord, 0x0}, {88, kWord, 0xFFFFFFFF, 2, 140},
    {384, kWord, 0x0}, {92, kWord, 0xFFFFFFFF, 2, 140}, {388, kWord, 0x0}, {96, kWord, 0xFFFFFFFF, 2, 140},
    {392, kWord, 0x0}, {100, kWord, 0xFFFFFFFF, 2, 140}, {396, kWord, 0x0}, {104, kWord, 0xFFFFFFFF, 2, 140},
    {400, kWord, 0x0}, {108, kWord, 0xFFFFFFFF, 2, 140}, {404, kWord, 0x0}, {112, kWord, 0xFFFFFFFF, 2, 140},
    {408, kWord, 0x0}, {116, kWord, 0xFFFFFFFF, 2, 140}, {412, kWord, 0x0}, {120, kWord, 0xFFFFFFFF, 2, 140},
    {416, kWord, 0x0}, {124, kWord, 0xFFFFFFFF, 2, 140}, {128, kWord, 0xFFFFFFFF, 2, 140}, {132, kWord, 0xFFFFFFFF, 2, 140},
    {136, kWord, 0xFFFFFFFF, 2, 140}, {140, kWord, 0xFFFFFFFF, 2, 140}, {144, kWord, 0xFFFFFFFF, 2, 140}, {148, kWord, 0xFFFFFFFF, 2, 140},
    {152, kWord, 0xFFFFFFFF, 2, 140}, {420, kWord, 0xFFFFFFFF, 2, 4},
};

// PairedNodeTable::PairedNodeTable (sub_82245818).
// 115 stores
constexpr Fill kPairedNodeTable_PairedNodeTable0[] = {
    {0, kWord, 0x82010CA4}, {8, kWord, 0xFFFFFFFF, 2, 4}, {296, kWord, 0xFFFFFFFF, 4, 4}, {16, kWord, 0xFFFFFFFF, 2, 140},
    {312, kWord, 0x0}, {20, kWord, 0xFFFFFFFF, 2, 140}, {316, kWord, 0x0}, {24, kWord, 0xFFFFFFFF, 2, 140},
    {320, kWord, 0x0}, {28, kWord, 0xFFFFFFFF, 2, 140}, {324, kWord, 0x0}, {32, kWord, 0xFFFFFFFF, 2, 140},
    {328, kWord, 0x0}, {36, kWord, 0xFFFFFFFF, 2, 140}, {332, kWord, 0x0}, {40, kWord, 0xFFFFFFFF, 2, 140},
    {336, kWord, 0x0}, {44, kWord, 0xFFFFFFFF, 2, 140}, {340, kWord, 0x0}, {48, kWord, 0xFFFFFFFF, 2, 140},
    {344, kWord, 0x0}, {52, kWord, 0xFFFFFFFF, 2, 140}, {348, kWord, 0x0}, {56, kWord, 0xFFFFFFFF, 2, 140},
    {352, kWord, 0x0}, {60, kWord, 0xFFFFFFFF, 2, 140}, {356, kWord, 0x0}, {64, kWord, 0xFFFFFFFF, 2, 140},
    {360, kWord, 0x0}, {68, kWord, 0xFFFFFFFF, 2, 140}, {364, kWord, 0x0}, {72, kWord, 0xFFFFFFFF, 2, 140},
    {368, kWord, 0x0}, {76, kWord, 0xFFFFFFFF, 2, 140}, {372, kWord, 0x0}, {80, kWord, 0xFFFFFFFF, 2, 140},
    {376, kWord, 0x0}, {84, kWord, 0xFFFFFFFF, 2, 140}, {380, kWord, 0x0}, {88, kWord, 0xFFFFFFFF, 2, 140},
    {384, kWord, 0x0}, {92, kWord, 0xFFFFFFFF, 2, 140}, {388, kWord, 0x0}, {96, kWord, 0xFFFFFFFF, 2, 140},
    {392, kWord, 0x0}, {100, kWord, 0xFFFFFFFF, 2, 140}, {396, kWord, 0x0}, {104, kWord, 0xFFFFFFFF, 2, 140},
    {400, kWord, 0x0}, {108, kWord, 0xFFFFFFFF, 2, 140}, {404, kWord, 0x0}, {112, kWord, 0xFFFFFFFF, 2, 140},
    {408, kWord, 0x0}, {116, kWord, 0xFFFFFFFF, 2, 140}, {412, kWord, 0x0}, {120, kWord, 0xFFFFFFFF, 2, 140},
    {416, kWord, 0x0}, {124, kWord, 0xFFFFFFFF, 2, 140}, {128, kWord, 0xFFFFFFFF, 2, 140}, {132, kWord, 0xFFFFFFFF, 2, 140},
    {136, kWord, 0xFFFFFFFF, 2, 140}, {140, kWord, 0xFFFFFFFF, 2, 140}, {144, kWord, 0xFFFFFFFF, 2, 140}, {148, kWord, 0xFFFFFFFF, 2, 140},
    {152, kWord, 0xFFFFFFFF, 2, 140}, {420, kWord, 0xFFFFFFFF, 2, 4}, {428, kWord, 0x0}, {0, kWord, 0x82010CAC},
    {432, kWord, 0xFFFFFFFF}, {436, kWord, 0x0}, {440, kWord, 0xFFFFFFFF}, {444, kWord, 0x0},
    {448, kWord, 0xFFFFFFFF}, {452, kWord, 0x0}, {456, kWord, 0xFFFFFFFF},
};

// ColorNodeTable::ColorNodeTable (sub_822458D0).
// 142 stores
constexpr Fill kColorNodeTable_ColorNodeTable0[] = {
    {0, kWord, 0x82010CA4}, {8, kWord, 0xFFFFFFFF, 2, 4}, {296, kWord, 0xFFFFFFFF, 4, 4}, {16, kWord, 0xFFFFFFFF, 2, 140},
    {312, kWord, 0x0}, {20, kWord, 0xFFFFFFFF, 2, 140}, {316, kWord, 0x0}, {24, kWord, 0xFFFFFFFF, 2, 140},
    {320, kWord, 0x0}, {28, kWord, 0xFFFFFFFF, 2, 140}, {324, kWord, 0x0}, {32, kWord, 0xFFFFFFFF, 2, 140},
    {328, kWord, 0x0}, {36, kWord, 0xFFFFFFFF, 2, 140}, {332, kWord, 0x0}, {40, kWord, 0xFFFFFFFF, 2, 140},
    {336, kWord, 0x0}, {44, kWord, 0xFFFFFFFF, 2, 140}, {340, kWord, 0x0}, {48, kWord, 0xFFFFFFFF, 2, 140},
    {344, kWord, 0x0}, {52, kWord, 0xFFFFFFFF, 2, 140}, {348, kWord, 0x0}, {56, kWord, 0xFFFFFFFF, 2, 140},
    {352, kWord, 0x0}, {60, kWord, 0xFFFFFFFF, 2, 140}, {356, kWord, 0x0}, {64, kWord, 0xFFFFFFFF, 2, 140},
    {360, kWord, 0x0}, {68, kWord, 0xFFFFFFFF, 2, 140}, {364, kWord, 0x0}, {72, kWord, 0xFFFFFFFF, 2, 140},
    {368, kWord, 0x0}, {76, kWord, 0xFFFFFFFF, 2, 140}, {372, kWord, 0x0}, {80, kWord, 0xFFFFFFFF, 2, 140},
    {376, kWord, 0x0}, {84, kWord, 0xFFFFFFFF, 2, 140}, {380, kWord, 0x0}, {88, kWord, 0xFFFFFFFF, 2, 140},
    {384, kWord, 0x0}, {92, kWord, 0xFFFFFFFF, 2, 140}, {388, kWord, 0x0}, {96, kWord, 0xFFFFFFFF, 2, 140},
    {392, kWord, 0x0}, {100, kWord, 0xFFFFFFFF, 2, 140}, {396, kWord, 0x0}, {104, kWord, 0xFFFFFFFF, 2, 140},
    {400, kWord, 0x0}, {108, kWord, 0xFFFFFFFF, 2, 140}, {404, kWord, 0x0}, {112, kWord, 0xFFFFFFFF, 2, 140},
    {408, kWord, 0x0}, {116, kWord, 0xFFFFFFFF, 2, 140}, {412, kWord, 0x0}, {120, kWord, 0xFFFFFFFF, 2, 140},
    {416, kWord, 0x0}, {124, kWord, 0xFFFFFFFF, 2, 140}, {128, kWord, 0xFFFFFFFF, 2, 140}, {132, kWord, 0xFFFFFFFF, 2, 140},
    {136, kWord, 0xFFFFFFFF, 2, 140}, {140, kWord, 0xFFFFFFFF, 2, 140}, {144, kWord, 0xFFFFFFFF, 2, 140}, {148, kWord, 0xFFFFFFFF, 2, 140},
    {152, kWord, 0xFFFFFFFF, 2, 140}, {420, kWord, 0xFFFFFFFF, 2, 4}, {428, kWord, 0x0}, {0, kWord, 0x82010CB4},
    {432, kWord, 0xFFFFFFFF}, {436, kWord, 0x0}, {440, kWord, 0xFFFFFFFF}, {444, kWord, 0x0},
    {448, kWord, 0xFFFFFFFF}, {452, kWord, 0x0}, {456, kWord, 0xFFFFFFFF, 2, 20}, {480, kWord, 0xFFFFFFFF, 2, 36},
    {484, kWord, 0xFFFFFFFF, 2, 36}, {488, kWord, 0xFFFFFFFF, 2, 36}, {492, kWord, 0xFFFFFFFF, 2, 36}, {496, kWord, 0xFFFFFFFF, 2, 36},
    {500, kWord, 0xFFFFFFFF, 2, 36}, {504, kWord, 0xFFFFFFFF, 2, 36}, {508, kWord, 0xFFFFFFFF, 2, 36}, {512, kWord, 0xFFFFFFFF, 2, 36},
    {552, kWord, 0xFF, 4, 4}, {568, kWord, 0xCD}, {572, kWord, 0xEB}, {576, kWord, 0x43},
    {580, kWord, 0xFF},
};

// ListScreen::ListScreen (sub_8224FC08).
// 112 stores
constexpr Fill kListScreen_ListScreen0[] = {
    {9676, kWord, 0x0}, {0, kWord, 0x82010EC8}, {9832, kWord, 0xFFFFFFFF}, {9680, kWord, 0x1},
    {9836, kWord, 0xFFFFFFFF, 7, 4}, {9872, kWord, 0xFFFFFFFF, 2, 4}, {9888, kWord, 0xFFFFFFFF}, {9880, kWord, 0xFFFFFFFF},
    {9864, kWord, 0xFFFFFFFF, 2, 4}, {9884, kWord, 0xFFFFFFFF, 2, 28}, {9916, kWord, 0xFFFFFFFF, 3, 4}, {9904, kWord, 0xFFFFFFFF, 2, 4},
    {9892, kWord, 0xFFFFFFFF, 3, 4}, {9800, kWord, 0xFFFFFFFF, 2, 4}, {9760, kWord, 0xFFFFFFFF}, {9788, kWord, 0x1},
    {9784, kWord, 0x0}, {9728, kWord, 0xFFFFFFFF}, {9724, kWord, 0xFFFFFFFF, 2, 28}, {9756, kWord, 0xFFFFFFFF, 2, 8},
    {9768, kWord, 0xFFFFFFFF}, {9772, kWord, 0x0, 2, 4}, {9792, kWord, 0xFFFFFFFF, 2, 4}, {9712, kWord, 0xFFFFFFFF, 3, 4},
    {9748, kWord, 0xFFFFFFFF, 2, 188}, {9976, kWord, 0xFFFFFFFF, 4, 40}, {9940, kWord, 0xFFFFFFFF, 5, 40}, {9944, kWord, 0xFFFFFFFF, 5, 40},
    {9948, kWord, 0xFFFFFFFF, 5, 40}, {9952, kWord, 0xFFFFFFFF, 5, 40}, {9956, kWord, 0xFFFFFFFF, 5, 40}, {9960, kWord, 0xFFFFFFFF, 5, 40},
    {9964, kWord, 0xFFFFFFFF, 5, 40}, {9968, kWord, 0xFFFFFFFF, 5, 40}, {9972, kWord, 0xFFFFFFFF, 5, 40}, {9736, kWord, 0xFFFFFFFF, 2, 72},
    {9732, kWord, 0xFFFFFFFF, 2, 80}, {9740, kWord, 0xFFFFFFFF, 2, 76}, {9684, kWord, 0x1}, {10136, kWord, 0x0},
    {10140, kWord, 0xFFFFFFFF}, {9820, kWord, 0xFFFFFFFF, 2, 4}, {9708, kWord, 0x0}, {9744, kWord, 0xFFFFFFFF, 2, 84},
    {9928, kWord, 0xFFFFFFFF, 2, 4},
};

// ListPartBase::ListPartBase (sub_8224FD70).
// 5 stores
constexpr Fill kListPartBase_ListPartBase0[] = {
    {0, kWord, 0x82010F50}, {12, kWord, 0x0}, {4, kWord, 0x0, 2, 4}, {16, kWord, 0x0},
};

// ListPartInnerBase::ListPartInnerBase (sub_8224FF58).
// 3 stores
constexpr Fill kListPartInnerBase_ListPartInnerBase0[] = {
    {32, kWord, 0x0}, {0, kWord, 0x82010F7C}, {36, kWord, 0x0},
};

// ListPartInner::ListPartInner (sub_82250018).
// 8 stores
constexpr Fill kListPartInner_ListPartInner0[] = {
    {32, kWord, 0x0}, {0, kWord, 0x82010F7C}, {36, kWord, 0x0}, {0, kWord, 0x82010FA8},
    {108, kWord, 0x0}, {100, kWord, 0x0}, {112, kWord, 0x3}, {116, kWord, 0x0},
};

// ListPart::ListPart (sub_82250280).
// 6 stores
constexpr Fill kListPart_ListPart0[] = {
    {0, kWord, 0x82010F50}, {12, kWord, 0x0}, {4, kWord, 0x0, 2, 4}, {16, kWord, 0x0},
    {0, kWord, 0x82010FC4},
};
// 2 stores
constexpr Fill kListPart_ListPart1[] = {
    {16, kSelf, 20}, {12, kWord, 0x10},
};

// NodeListScreen::NodeListScreen (sub_822549B0).
// 7 stores
constexpr Fill kNodeListScreen_NodeListScreen0[] = {
    {0, kWord, 0x82011208}, {10212, kWord, 0x0}, {10204, kWord, 0x0, 2, 80}, {10276, kWord, 0x0, 2, 80},
    {10348, kWord, 0x0},
};
// 3 stores
constexpr Fill kNodeListScreen_NodeListScreen1[] = {
    {9672, kSelf, 10360}, {9680, kWord, 0x19, 2, 684},
};

// TitledListScreen::TitledListScreen (sub_82254B38).
// 2 stores
constexpr Fill kTitledListScreen_TitledListScreen0[] = {
    {10820, kWord, 0x39C}, {0, kWord, 0x82011278},
};

// NodeScreen::NodeScreen (sub_82257D50).
// 71 stores
constexpr Fill kNodeScreen_NodeScreen0[] = {
    {0, kWord, 0x82011368}, {10200, kWord, 0x0}, {10192, kWord, 0x0, 2, 80}, {10264, kWord, 0x0},
    {4, kWord, 0xF, 2, 10096}, {10284, kWord, 0xFFFFFFFF, 4, 4}, {10308, kWord, 0xFFFFFFFF, 2, 4}, {10320, kWord, 0xFFFFFFFF},
    {10300, kWord, 0xFFFFFFFF, 2, 4}, {10316, kWord, 0xFFFFFFFF, 2, 20}, {10340, kWord, 0xFFFFFFFF}, {10324, kWord, 0xFFFFFFFF, 3, 4},
    {10392, kWord, 0xFFFFFFFF, 2, 144}, {10540, kWord, 0xFFFFFFFF}, {10276, kWord, 0xFFFFFFFF, 2, 120}, {10400, kWord, 0xFFFFFFFF, 5, 4},
    {10456, kWord, 0xFFFFFFFF, 2, 40}, {10420, kWord, 0xFFFFFFFF, 3, 40}, {10424, kWord, 0xFFFFFFFF, 3, 40}, {10428, kWord, 0xFFFFFFFF, 3, 40},
    {10432, kWord, 0xFFFFFFFF, 3, 40}, {10436, kWord, 0xFFFFFFFF, 3, 40}, {10440, kWord, 0xFFFFFFFF, 3, 40}, {10444, kWord, 0xFFFFFFFF, 3, 40},
    {10448, kWord, 0xFFFFFFFF, 3, 40}, {10452, kWord, 0xFFFFFFFF, 3, 40}, {10352, kWord, 0xFFFFFFFF, 10, 4},
};

// Type6NodeScreen::Type6NodeScreen (sub_82235090).
// 130 stores
constexpr Fill kType6NodeScreen_Type6NodeScreen0[] = {
    {10284, kWord, 0x65}, {0, kWord, 0x82010738}, {10324, kWord, 0x2C}, {10308, kWord, 0x66},
    {10312, kWord, 0x2B}, {10336, kWord, 0x10}, {10340, kWord, 0x11}, {10280, kWord, 0x5},
    {10276, kWord, 0x8, 2, 120}, {8, kWord, 0xB}, {12, kWord, 0xA, 2, 10396}, {10412, kWord, 0xB},
    {10540, kWord, 0x1}, {10348, kWord, 0x1C}, {10328, kWord, 0x2D}, {10416, kWord, 0x1E},
    {10420, kWord, 0x23}, {10344, kWord, 0x1D}, {10424, kWord, 0x28}, {10428, kWord, 0x2D},
    {10440, kWord, 0x3C}, {10432, kWord, 0x32}, {10444, kWord, 0x41}, {10400, kWord, 0x9},
    {10448, kWord, 0x46}, {10464, kWord, 0x29}, {10452, kWord, 0x4B}, {10456, kWord, 0x1F},
    {10460, kWord, 0x24}, {10468, kWord, 0x2E}, {10296, kWord, 0x0}, {10304, kWord, 0x2F},
    {10392, kWord, 0x67}, {10404, kWord, 0x0, 2, 132}, {10548, kWord, 0x0}, {10436, kWord, 0x37},
    {10472, kWord, 0x33}, {10480, kWord, 0x3D}, {10476, kWord, 0x38}, {10484, kWord, 0x42},
    {10488, kWord, 0x47}, {10492, kWord, 0x4C}, {10504, kWord, 0x2A}, {10508, kWord, 0x2F},
    {10496, kWord, 0x20}, {10500, kWord, 0x25}, {10512, kWord, 0x34}, {10516, kWord, 0x39},
    {10520, kWord, 0x3E}, {10524, kWord, 0x43}, {10528, kWord, 0x48}, {10532, kWord, 0x4D},
    {296, kWord, 0x6}, {300, kWord, 0x7}, {304, kWord, 0x8}, {308, kWord, 0x9},
    {156, kWord, 0x1}, {16, kWord, 0x30}, {160, kWord, 0x2}, {20, kWord, 0x31},
    {164, kWord, 0x3}, {24, kWord, 0x32}, {168, kWord, 0x4}, {28, kWord, 0x33},
    {172, kWord, 0x5}, {32, kWord, 0x34}, {176, kWord, 0x6}, {36, kWord, 0x35},
    {180, kWord, 0x7}, {40, kWord, 0x36}, {184, kWord, 0x8}, {44, kWord, 0x37},
    {188, kWord, 0x9}, {48, kWord, 0x38}, {192, kWord, 0xA}, {52, kWord, 0x39},
    {196, kWord, 0xB}, {56, kWord, 0x3A}, {200, kWord, 0xC}, {60, kWord, 0x3B},
    {204, kWord, 0xD}, {64, kWord, 0x3C}, {208, kWord, 0xE}, {68, kWord, 0x3D},
    {212, kWord, 0xF}, {72, kWord, 0x3E}, {216, kWord, 0x10}, {76, kWord, 0x3F},
    {220, kWord, 0x11}, {80, kWord, 0x40}, {224, kWord, 0x12}, {84, kWord, 0x41},
    {228, kWord, 0x13}, {88, kWord, 0x42}, {232, kWord, 0x14}, {92, kWord, 0x43},
    {236, kWord, 0x15}, {96, kWord, 0x44}, {240, kWord, 0x16}, {100, kWord, 0x45},
    {244, kWord, 0x17}, {104, kWord, 0x46}, {248, kWord, 0x18}, {108, kWord, 0x47},
    {252, kWord, 0x19}, {112, kWord, 0x48}, {256, kWord, 0x1A}, {116, kWord, 0x49},
    {260, kWord, 0x1B}, {120, kWord, 0x4A}, {124, kWord, 0x4F}, {128, kWord, 0x50},
    {132, kWord, 0x51}, {136, kWord, 0x52}, {140, kWord, 0x53}, {144, kWord, 0x54},
    {148, kWord, 0x55}, {152, kWord, 0x56}, {10352, kWord, 0x6}, {10356, kWord, 0x7},
    {10360, kWord, 0x8}, {10364, kWord, 0x9}, {10368, kWord, 0xA}, {10372, kWord, 0xB},
    {424, kWord, 0x37}, {420, kWord, 0x6}, {10544, kWord, 0xF},
};

// Type6Screens::Type6Screens (sub_82234B50).
// 1 stores
constexpr Fill kType6Screens_Type6Screens0[] = {
    {0, kWord, 0x82010730},
};
// 1 stores
constexpr Fill kType6Screens_Type6Screens1[] = {
    {52692, kWord, 0x82010720},
};
// 3 stores
constexpr Fill kType6Screens_Type6Screens2[] = {
    {91260, kWord, 0x0}, {91256, kByte, 0x0}, {91264, kWord, 0x0},
};

// Type7Screens::Type7Screens (sub_822446C8).
// 1 stores
constexpr Fill kType7Screens_Type7Screens0[] = {
    {0, kWord, 0x82010C94},
};
// 1 stores
constexpr Fill kType7Screens_Type7Screens1[] = {
    {42352, kWord, 0x82010C84},
};
// 3 stores
constexpr Fill kType7Screens_Type7Screens2[] = {
    {71280, kWord, 0x0}, {71276, kByte, 0x0}, {71284, kWord, 0x0},
};

bool On() { return REXCVAR_GET(native_screen_constructors); }

}  // namespace

// NodeBase::NodeBase.
extern "C" REX_FUNC(sub_82256928) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82256928(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kNodeBase_NodeBase0, args);
  ctx.r3.u64 = object;
}

// PanelGroupBase::PanelGroupBase.
extern "C" REX_FUNC(sub_8224C888) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224C888(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanelGroupBase_PanelGroupBase0, args);
  ctx.r3.u64 = object;
}

// PanelCBase::PanelCBase.
extern "C" REX_FUNC(sub_8224B148) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224B148(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanelCBase_PanelCBase0, args);
  ctx.r3.u64 = object;
}

// PanelDBase::PanelDBase.
extern "C" REX_FUNC(sub_8224A9F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224A9F0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanelDBase_PanelDBase0, args);
  ctx.r3.u64 = object;
}

// PanelEBase::PanelEBase.
extern "C" REX_FUNC(sub_8224C958) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224C958(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanelEBase_PanelEBase0, args);
  ctx.r3.u64 = object;
}

// PanelE::PanelE.
extern "C" REX_FUNC(sub_8222E2A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8222E2A0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanelE_PanelE0, args);
  ctx.r3.u64 = object;
}

// PanelFBase::PanelFBase.
extern "C" REX_FUNC(sub_8224D468) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224D468(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanelFBase_PanelFBase0, args);
  ctx.r3.u64 = object;
}

// Panel6C::Panel6C.
extern "C" REX_FUNC(sub_8222F730) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8222F730(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanel6C_Panel6C0, args);
  ctx.r3.u64 = object;
}

// Panel7C::Panel7C.
extern "C" REX_FUNC(sub_8223F270) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8223F270(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanel7C_Panel7C0, args);
  ctx.r3.u64 = object;
}

// Panel6D::Panel6D.
extern "C" REX_FUNC(sub_8222F8A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8222F8A0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanel6D_Panel6D0, args);
  ctx.r3.u64 = object;
}

// Panel7D::Panel7D.
extern "C" REX_FUNC(sub_8223F450) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8223F450(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanel7D_Panel7D0, args);
  ctx.r3.u64 = object;
}

// Panel6F::Panel6F.
extern "C" REX_FUNC(sub_8222E388) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8222E388(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanel6F_Panel6F0, args);
  ctx.r3.u64 = object;
}

// Panel7F::Panel7F.
extern "C" REX_FUNC(sub_8223DE58) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8223DE58(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanel7F_Panel7F0, args);
  ctx.r3.u64 = object;
}

// PanelGroup::PanelGroup.
extern "C" REX_FUNC(sub_8222E128) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8222E128(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_8224C888(ctx, base);
  guest_memory::Apply(base, object, kPanelGroup_PanelGroup0, args);
  ctx.r3.u64 = object;
}

// PanelGroup6B::PanelGroup6B.
extern "C" REX_FUNC(sub_8222F6C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8222F6C8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_8224C888(ctx, base);
  guest_memory::Apply(base, object, kPanelGroup6B_PanelGroup6B0, args);
  ctx.r3.u64 = object;
}

// PanelGroup7B::PanelGroup7B.
extern "C" REX_FUNC(sub_8223F218) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8223F218(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_8224C888(ctx, base);
  guest_memory::Apply(base, object, kPanelGroup7B_PanelGroup7B0, args);
  ctx.r3.u64 = object;
}

// PanelSetBase::PanelSetBase.
extern "C" REX_FUNC(sub_82248770) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82248770(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPanelSetBase_PanelSetBase0, args);
  ctx.r3.u64 = object;
}

// GroupPair::GroupPair.
extern "C" REX_FUNC(sub_822477C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822477C0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kGroupPair_GroupPair0, args);
  ctx.r3.u64 = object;
}

// MachineScreen::MachineScreen.
extern "C" REX_FUNC(sub_8224A9A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224A9A8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 4;
  sub_82107480(ctx, base);
  guest_memory::Apply(base, object, kMachineScreen_MachineScreen0, args);
  ctx.r3.u64 = object;
}

// Type6Screen0::Type6Screen0.
extern "C" REX_FUNC(sub_82233FE8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82233FE8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_82107480(ctx, base);
  ctx.r3.u64 = object;
}

// Type7Screen0::Type7Screen0.
extern "C" REX_FUNC(sub_82243B48) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82243B48(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 4;
  sub_82107480(ctx, base);
  guest_memory::Apply(base, object, kType7Screen0_Type7Screen00, args);
  ctx.r3.u64 = object;
}

// NodeTable::NodeTable.
extern "C" REX_FUNC(sub_822457A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822457A0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kNodeTable_NodeTable0, args);
  ctx.r3.u64 = object;
}

// PairedNodeTable::PairedNodeTable.
extern "C" REX_FUNC(sub_82245818) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82245818(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPairedNodeTable_PairedNodeTable0, args);
  ctx.r3.u64 = object;
}

// ColorNodeTable::ColorNodeTable.
extern "C" REX_FUNC(sub_822458D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822458D0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kColorNodeTable_ColorNodeTable0, args);
  ctx.r3.u64 = object;
}

// ListScreen::ListScreen.
extern "C" REX_FUNC(sub_8224FC08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224FC08(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 4;
  sub_82107480(ctx, base);
  guest_memory::Apply(base, object, kListScreen_ListScreen0, args);
  ctx.r3.u64 = object;
}

// ListPartBase::ListPartBase.
extern "C" REX_FUNC(sub_8224FD70) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224FD70(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kListPartBase_ListPartBase0, args);
  ctx.r3.u64 = object;
}

// ListPartInnerBase::ListPartInnerBase.
extern "C" REX_FUNC(sub_8224FF58) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8224FF58(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kListPartInnerBase_ListPartInnerBase0, args);
  ctx.r3.u64 = object;
}

// ListPartInner::ListPartInner.
extern "C" REX_FUNC(sub_82250018) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82250018(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kListPartInner_ListPartInner0, args);
  ctx.r3.u64 = object;
}

// ListPart::ListPart.
extern "C" REX_FUNC(sub_82250280) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82250280(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kListPart_ListPart0, args);
  ctx.r3.u64 = object + 20;
  sub_82250018(ctx, base);
  guest_memory::Apply(base, object, kListPart_ListPart1, args);
  ctx.r3.u64 = object;
}

// NodeListScreen::NodeListScreen.
extern "C" REX_FUNC(sub_822549B0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822549B0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_8224FC08(ctx, base);
  guest_memory::Apply(base, object, kNodeListScreen_NodeListScreen0, args);
  ctx.r3.u64 = object + 10360;
  sub_82245818(ctx, base);
  guest_memory::Apply(base, object, kNodeListScreen_NodeListScreen1, args);
  ctx.r3.u64 = object;
}

// TitledListScreen::TitledListScreen.
extern "C" REX_FUNC(sub_82254B38) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82254B38(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_822549B0(ctx, base);
  guest_memory::Apply(base, object, kTitledListScreen_TitledListScreen0, args);
  ctx.r3.u64 = object;
}

// NodeScreen::NodeScreen.
extern "C" REX_FUNC(sub_82257D50) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82257D50(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 460;
  sub_82107480(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_82245818(ctx, base);
  guest_memory::Apply(base, object, kNodeScreen_NodeScreen0, args);
  ctx.r3.u64 = object;
}

// Type6NodeScreen::Type6NodeScreen.
extern "C" REX_FUNC(sub_82235090) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82235090(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_82257D50(ctx, base);
  guest_memory::Apply(base, object, kType6NodeScreen_Type6NodeScreen0, args);
  ctx.r3.u64 = object;
}

// Type6Screens::Type6Screens.
extern "C" REX_FUNC(sub_82234B50) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82234B50(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kType6Screens_Type6Screens0, args);
  ctx.r3.u64 = object + 4;
  sub_82233FE8(ctx, base);
  ctx.r3.u64 = object + 9672;
  sub_82107480(ctx, base);
  ctx.r3.u64 = object + 19432;
  sub_82230D80(ctx, base);
  ctx.r3.u64 = object + 30396;
  sub_82235090(ctx, base);
  ctx.r3.u64 = object + 40948;
  sub_8222E480(ctx, base);
  ctx.r3.u64 = object + 52696;
  sub_82107480(ctx, base);
  guest_memory::Apply(base, object, kType6Screens_Type6Screens1, args);
  ctx.r3.u64 = object + 62336;
  sub_82107480(ctx, base);
  ctx.r3.u64 = object + 71976;
  sub_82107480(ctx, base);
  ctx.r3.u64 = object + 81616;
  sub_82107480(ctx, base);
  guest_memory::Apply(base, object, kType6Screens_Type6Screens2, args);
  ctx.r3.u64 = object;
}

// Type7Screens::Type7Screens.
extern "C" REX_FUNC(sub_822446C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822446C8(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kType7Screens_Type7Screens0, args);
  ctx.r3.u64 = object + 4;
  sub_82243B48(ctx, base);
  ctx.r3.u64 = object + 9664;
  sub_82107480(ctx, base);
  ctx.r3.u64 = object + 19428;
  sub_82240758(ctx, base);
  ctx.r3.u64 = object + 30608;
  sub_8223DF38(ctx, base);
  ctx.r3.u64 = object + 42356;
  sub_82107480(ctx, base);
  guest_memory::Apply(base, object, kType7Screens_Type7Screens1, args);
  ctx.r3.u64 = object + 51996;
  sub_82107480(ctx, base);
  ctx.r3.u64 = object + 61636;
  sub_82107480(ctx, base);
  guest_memory::Apply(base, object, kType7Screens_Type7Screens2, args);
  ctx.r3.u64 = object;
}
