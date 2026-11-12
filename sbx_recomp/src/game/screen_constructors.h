// The front end's screen parts the type 6 and 7 menus are made of, as far
// as their constructors show them. Most are tables of node numbers (-1 for
// none) whose meanings aren't worked out, kept as word arrays.

#pragma once

#include "game/game_modes.h"
#include "game/guest_types.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

struct NodeBase {
  u32_be vtable;
  u32_be unknown_4;
};
GUEST_SIZE(NodeBase, 8);

// A panel group (PanelGroupBase and the groups made from it).
struct PanelGroup {
  u32_be vtable;
  u32_be unknown_4;
  i32_be words[7];
};
GUEST_SIZE(PanelGroup, 36);

struct PanelC {
  u32_be vtable;
  u32_be unknown_4;
  i32_be words[67];  // From +8.
  u32_be unknown_276[11];
};
GUEST_SIZE(PanelC, 320);
GUEST_OFFSET(PanelC, unknown_276, 276);

struct PanelD {
  u32_be vtable;
  u32_be unknown_4;
  i32_be words[19];  // From +8.
  uint8_t unknown_84[4];
  u32_be unknown_88;
};
GUEST_OFFSET(PanelD, unknown_88, 88);

struct PanelE {
  u32_be unknown_0;
  u32_be unknown_4;
  i32_be words[36];  // From +8.
  u32_be unknown_152[2];
};
GUEST_SIZE(PanelE, 160);

struct PanelF {
  u32_be vtable;
  u32_be unknown_4;
  uint8_t unknown_8[16];
  i32_be words[29];  // From +24.
  u32_be unknown_140;
};
GUEST_OFFSET(PanelF, words, 24);
GUEST_OFFSET(PanelF, unknown_140, 140);

struct PanelSet {
  u32_be vtable;
  u32_be unknown_4;
  uint8_t unknown_8[4];
  u32_be unknown_12[5];
  uint8_t unknown_32[88];
  u32_be unknown_120;
  uint8_t unknown_124[4];
  u32_be unknown_128;
  uint8_t unknown_132[60];
  u32_be unknown_192;
  uint8_t unknown_196[4];
  u32_be unknown_200[13];  // +224 is -1 when made, the rest 0.
  uint8_t unknown_252[532];
  u32_be unknown_784;
  uint8_t unknown_788[512];
  u32_be unknown_1300;
  uint8_t unknown_1304[512];
  u32_be unknown_1816;
  uint8_t unknown_1820[56];
  u32_be unknown_1876;
  uint8_t unknown_1880[12];
  u32_be unknown_1892[7];
  u32_be unknown_1920;
  uint8_t unknown_1924[4];
  i32_be unknown_1928[11];
};
GUEST_OFFSET(PanelSet, unknown_200, 200);
GUEST_OFFSET(PanelSet, unknown_784, 784);
GUEST_OFFSET(PanelSet, unknown_1816, 1816);
GUEST_OFFSET(PanelSet, unknown_1892, 1892);
GUEST_OFFSET(PanelSet, unknown_1928, 1928);

struct GroupPair {
  u32_be vtable;
  uint8_t unknown_4[4];
  u32_be first;
  u32_be second;
};
GUEST_OFFSET(GroupPair, first, 8);

// A screen with a state machine after its vtable.
struct MachineScreen {
  u32_be vtable;
  StateMachine machine;
  uint8_t unknown_9612[40];
  u32_be unknown_9652;
};
GUEST_OFFSET(MachineScreen, unknown_9652, 9652);

struct NodeTable {
  u32_be vtable;
  u32_be unknown_4;  // A count (25 for a node list screen's).
  i32_be words[76];  // From +8.
  u32_be unknown_312[27];
  i32_be unknown_420[2];
};
GUEST_SIZE(NodeTable, 428);
GUEST_OFFSET(NodeTable, unknown_420, 420);

struct NodePair {
  u32_be unknown_0;
  i32_be unknown_4;
};

struct PairedNodeTable {
  NodeTable table;
  NodePair pairs[4];
};
GUEST_SIZE(PairedNodeTable, 460);

struct ColorNodeTable {
  PairedNodeTable paired;
  uint8_t unknown_460[16];
  i32_be unknown_476[19];
  u32_be colors[8];  // One byte value a word.
};
GUEST_OFFSET(ColorNodeTable, unknown_476, 476);
GUEST_OFFSET(ColorNodeTable, colors, 552);
GUEST_SIZE(ColorNodeTable, 584);

struct ListScreen {
  u32_be vtable;
  StateMachine machine;
  uint8_t unknown_9612[60];
  u32_be table;  // A node list screen's node table.
  u32_be unknown_9676;
  u32_be unknown_9680[2];
  uint8_t unknown_9688[20];
  u32_be unknown_9708;
  i32_be unknown_9712[15];
  u32_be unknown_9772[2];
  uint8_t unknown_9780[4];
  u32_be unknown_9784;
  u32_be unknown_9788;
  i32_be unknown_9792[86];
  u32_be unknown_10136;
  i32_be unknown_10140;
};
GUEST_OFFSET(ListScreen, table, 9672);
GUEST_OFFSET(ListScreen, unknown_9712, 9712);
GUEST_OFFSET(ListScreen, unknown_9792, 9792);
GUEST_OFFSET(ListScreen, unknown_10140, 10140);
GUEST_SIZE(ListScreen, 10144);

struct NodeListScreen {
  ListScreen screen;
  uint8_t unknown_10144[60];
  u32_be unknown_10204;
  uint8_t unknown_10208[4];
  u32_be unknown_10212;
  uint8_t unknown_10216[60];
  u32_be unknown_10276;
  uint8_t unknown_10280[4];
  u32_be unknown_10284;
  uint8_t unknown_10288[60];
  u32_be unknown_10348;
  uint8_t unknown_10352[4];
  u32_be unknown_10356;
  PairedNodeTable nodes;
  u32_be unknown_10820;
};
GUEST_OFFSET(NodeListScreen, unknown_10204, 10204);
GUEST_OFFSET(NodeListScreen, unknown_10356, 10356);
GUEST_OFFSET(NodeListScreen, nodes, 10360);
GUEST_OFFSET(NodeListScreen, unknown_10820, 10820);

struct ListPartInner {
  u32_be vtable;
  uint8_t unknown_4[28];
  u32_be unknown_32[2];
  uint8_t unknown_40[60];
  u32_be unknown_100;
  uint8_t unknown_104[4];
  u32_be unknown_108;
  u32_be unknown_112;
  u32_be unknown_116;
};
GUEST_OFFSET(ListPartInner, unknown_100, 100);
GUEST_OFFSET(ListPartInner, unknown_116, 116);

struct ListPart {
  u32_be vtable;
  u32_be unknown_4[2];
  u32_be unknown_12;
  u32_be inner_address;  // Its own `inner`.
  ListPartInner inner;
};
GUEST_OFFSET(ListPart, inner, 20);

struct NodeScreen {
  PairedNodeTable nodes;
  StateMachine machine;
  uint8_t unknown_10068[32];
  u32_be unknown_10100;
  uint8_t unknown_10104[88];
  u32_be unknown_10192;
  uint8_t unknown_10196[4];
  u32_be unknown_10200;
  uint8_t unknown_10204[60];
  u32_be unknown_10264;
  uint8_t unknown_10268[4];
  u32_be unknown_10272;
  i32_be words[69];  // From +10276.
};
GUEST_OFFSET(NodeScreen, machine, 460);
GUEST_OFFSET(NodeScreen, unknown_10100, 10100);
GUEST_OFFSET(NodeScreen, unknown_10192, 10192);
GUEST_OFFSET(NodeScreen, words, 10276);
GUEST_SIZE(NodeScreen, 10552);

// A screen in the menus' screen sets: a state machine and 32 bytes.
struct MachineSlot {
  StateMachine machine;
  uint8_t unknown_9608[32];
};
GUEST_SIZE(MachineSlot, 9640);

// The same with a vtable before it.
struct VtableMachineSlot {
  u32_be vtable;
  MachineSlot slot;
};
GUEST_SIZE(VtableMachineSlot, 9644);

// A vtable, then a state machine (Type7Screen0).
struct VtableMachine {
  u32_be vtable;
  StateMachine machine;
};
GUEST_SIZE(VtableMachine, 9612);

// The type 6 menus' screens.
struct Type6Screens {
  u32_be vtable;
  StateMachine screen_0;  // Type6Screen0.
  uint8_t unknown_9612[60];
  StateMachine machine_9672;
  uint8_t unknown_19280[152];
  uint8_t list_screen[10964];   // Type6ListScreen (recompiled).
  NodeScreen node_screen;       // Type6NodeScreen.
  uint8_t panel_screen[11744];  // Type6PanelScreen (recompiled).
  VtableMachineSlot screen_52692;
  MachineSlot screens[3];
  uint8_t unknown_91256;
  uint8_t unknown_91257[3];
  u32_be unknown_91260[2];
};
GUEST_OFFSET(Type6Screens, machine_9672, 9672);
GUEST_OFFSET(Type6Screens, list_screen, 19432);
GUEST_OFFSET(Type6Screens, node_screen, 30396);
GUEST_OFFSET(Type6Screens, panel_screen, 40948);
GUEST_OFFSET(Type6Screens, screen_52692, 52692);
GUEST_OFFSET(Type6Screens, screens, 62336);
GUEST_OFFSET(Type6Screens, unknown_91256, 91256);
GUEST_OFFSET(Type6Screens, unknown_91260, 91260);

// The type 7 menus' screens.
struct Type7Screens {
  u32_be vtable;
  VtableMachine screen_0;  // Type7Screen0.
  uint8_t unknown_9616[48];
  StateMachine machine_9664;
  uint8_t unknown_19272[156];
  uint8_t list_screen[11180];   // Type7ListScreen (recompiled).
  uint8_t panel_screen[11744];  // Type7PanelScreen (recompiled).
  VtableMachineSlot screen_42352;
  MachineSlot screens[2];
  uint8_t unknown_71276;
  uint8_t unknown_71277[3];
  u32_be unknown_71280[2];
};
GUEST_OFFSET(Type7Screens, machine_9664, 9664);
GUEST_OFFSET(Type7Screens, list_screen, 19428);
GUEST_OFFSET(Type7Screens, panel_screen, 30608);
GUEST_OFFSET(Type7Screens, screen_42352, 42352);
GUEST_OFFSET(Type7Screens, screens, 51996);
GUEST_OFFSET(Type7Screens, unknown_71276, 71276);
GUEST_OFFSET(Type7Screens, unknown_71280, 71280);

}  // namespace game
