// The front end's screen constructors as our own C++
// (--native_screen_constructors, on by default): the panels, panel groups
// and sets, node tables, list screens and node screens the type 6 and 7
// menus are made of, and the two screens' sets that make them.
//
// Each constructor sets its vtable and defaults (mostly -1 for no node),
// then the class's own node numbers, and makes its members. Six stay
// recompiled: the two that allocate a member and test it
// (Type6PanelScreen sub_8222E480, Type7PanelScreen sub_8223DF38), the two
// that store through a member they load (Type6ListScreen sub_82230D80,
// Type7ListScreen sub_82240758), PanelSet::PanelSet (sub_822490A8, stores
// through its argument) and ColorListScreen::ColorListScreen (sub_82252A70,
// copies a word it loads).

#include "game/screen_constructors.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_screen_constructors, true, "SBX",
                    "Our own C++ for the front end's screen constructors (panels, groups, "
                    "sets, node tables, list and node screens) instead of the recompiled "
                    "functions");

namespace {

using game::ColorNodeTable;
using game::GroupPair;
using game::ListPart;
using game::ListPartInner;
using game::ListScreen;
using game::MachineScreen;
using game::NodeBase;
using game::NodeListScreen;
using game::NodePair;
using game::NodeScreen;
using game::NodeTable;
using game::PairedNodeTable;
using game::PanelC;
using game::PanelD;
using game::PanelE;
using game::PanelF;
using game::PanelGroup;
using game::PanelSet;
using game::StateMachine;
using game::Type6Screens;
using game::Type7Screens;
using game::VtableMachine;
using guest::At;
using guest::Guest;
using guest::i32_be;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_screen_constructors); }

// The vtables (read-only).
constexpr uint32_t kNodeBaseVtable = 0x82000A40;
constexpr uint32_t kPanelGroupVtable = 0x82010630;
constexpr uint32_t kPanelVtable = 0x82010644;  // PanelGroupBase's, and panels D and F.
constexpr uint32_t kPanelCBaseVtable = 0x82010E70;
constexpr uint32_t kPanel6CVtable = 0x82010634;
constexpr uint32_t kPanel7CVtable = 0x82010B9C;
constexpr uint32_t kPanelSetBaseVtable = 0x82010D74;
constexpr uint32_t kGroupPairVtable = 0x82010D34;
constexpr uint32_t kMachineScreenVtable = 0x82010E18;
constexpr uint32_t kNodeTableVtable = 0x82010CA4;
constexpr uint32_t kPairedNodeTableVtable = 0x82010CAC;
constexpr uint32_t kColorNodeTableVtable = 0x82010CB4;
constexpr uint32_t kListScreenVtable = 0x82010EC8;
constexpr uint32_t kListPartBaseVtable = 0x82010F50;
constexpr uint32_t kListPartInnerBaseVtable = 0x82010F7C;
constexpr uint32_t kListPartInnerVtable = 0x82010FA8;
constexpr uint32_t kListPartVtable = 0x82010FC4;
constexpr uint32_t kNodeListScreenVtable = 0x82011208;
constexpr uint32_t kTitledListScreenVtable = 0x82011278;
constexpr uint32_t kNodeScreenVtable = 0x82011368;
constexpr uint32_t kType6NodeScreenVtable = 0x82010738;
constexpr uint32_t kType6ScreensVtable = 0x82010730;
constexpr uint32_t kType6Screen52692Vtable = 0x82010720;
constexpr uint32_t kType7ScreensVtable = 0x82010C94;
constexpr uint32_t kType7Screen42352Vtable = 0x82010C84;

// A word the panel set's constructor clears (writable).
constexpr uint32_t kPanelSetGlobal = 0x8280EEB4;

// A node list screen's node count.
constexpr uint32_t kNodeListCount = 25;

void ConstructMachine(Guest& g, StateMachine& machine) { g.Call(sub_82107480, &machine); }

template <size_t N>
void Fill(i32_be (&words)[N], int32_t value, size_t from = 0, size_t to = N) {
  for (size_t i = from; i < to; i++) {
    words[i] = value;
  }
}

template <size_t N>
void Fill(u32_be (&words)[N], uint32_t value) {
  for (u32_be& word : words) {
    word = value;
  }
}

void ConstructPanelGroupBase(PanelGroup* group) {
  group->unknown_4 = 0;
  group->vtable = kPanelVtable;
  Fill(group->words, -1);
}

// PanelGroup and the 6B and 7B groups: `count` at +4 and `words[1]`.
void ConstructPanelGroup(Guest& g, PanelGroup* group, uint32_t unknown_4, int32_t word_1) {
  g.Call(sub_8224C888, group);
  group->unknown_4 = unknown_4;
  group->vtable = kPanelGroupVtable;
  group->words[1] = word_1;
  group->words[6] = 0;
}

// PanelCBase: -1 for every node but words 1 (0), 3, 34 and 60 to 62 (left
// as they were).
void ConstructPanelCBase(PanelC* panel) {
  panel->unknown_4 = 1;
  panel->vtable = kPanelCBaseVtable;
  panel->words[0] = -1;
  panel->words[1] = 0;
  panel->words[2] = -1;
  Fill(panel->words, -1, 4, 34);
  Fill(panel->words, -1, 35, 60);
  Fill(panel->words, -1, 63, 67);
  Fill(panel->unknown_276, 0);
}

// Panel6C and Panel7C: the same nodes but for words 3, 4 and 6.
struct PanelCNodes {
  uint32_t vtable;
  int32_t word_3, word_4, word_6;
};
constexpr PanelCNodes kPanel6C = {kPanel6CVtable, 7, 0, 5};
constexpr PanelCNodes kPanel7C = {kPanel7CVtable, 6, 4, 3};

void ConstructPanelC(PanelC* panel, uint32_t r4, const PanelCNodes& nodes) {
  ConstructPanelCBase(panel);
  panel->vtable = nodes.vtable;
  panel->unknown_4 = 0;
  i32_be* w = panel->words;
  w[1] = int32_t(r4);
  w[3] = nodes.word_3;
  w[4] = nodes.word_4;
  w[6] = nodes.word_6;
  w[8] = 0x6D;
  w[13] = 0x70;
  w[15] = 5;
  w[16] = 6;
  w[25] = 0xF;
  w[26] = 0x10;
  w[29] = 0;
  w[30] = 0x2B;
  w[31] = 0xD;
  w[32] = 0xE;
  w[33] = 0xF;
  w[34] = 5;
  w[36] = 0;
  w[38] = 5;
  w[39] = 6;
  w[40] = 7;
  w[41] = 8;
  w[42] = 0xB;
  w[43] = 0xC;
  w[52] = 9;
  w[53] = 0xA;
  w[54] = 0xD;
  w[55] = 0xE;
  w[66] = 4;
  panel->unknown_276[0] = 0x7B;
  panel->unknown_276[1] = 0x70;
}

// PanelDBase: every node -1.
void ConstructPanelDBase(PanelD* panel) {
  panel->unknown_4 = 0;
  panel->vtable = kNodeBaseVtable;
  Fill(panel->words, -1);
  panel->unknown_88 = 0;
}

// Panel6D and Panel7D: their own first nodes.
struct PanelDNodes {
  int32_t word_1, word_4, word_5;
};
constexpr PanelDNodes kPanel6D = {0x16, 0x60, 0x69};
constexpr PanelDNodes kPanel7D = {0x14, 0x5E, 0x67};

void ConstructPanelD(PanelD* panel, uint32_t r4, const PanelDNodes& nodes) {
  ConstructPanelDBase(panel);
  panel->vtable = kPanelVtable;
  panel->unknown_4 = r4;
  i32_be* w = panel->words;
  w[1] = nodes.word_1;
  w[2] = 0x6E;
  w[3] = 0x6F;
  w[4] = nodes.word_4;
  for (int i = 0; i < 4; i++) {
    w[5 + i] = nodes.word_5 + i;
  }
  w[13] = nodes.word_1 - 1;
  w[14] = 0;
  w[15] = 1;
  w[18] = 2;
}

void ConstructPanelEBase(PanelE* panel) {
  panel->unknown_0 = 0;
  panel->unknown_4 = 0;
  Fill(panel->words, -1);
  Fill(panel->unknown_152, 0);
}

void ConstructPanelFBase(PanelF* panel) {
  panel->unknown_4 = 0;
  panel->vtable = kNodeBaseVtable;
  Fill(panel->words, -1);
  panel->unknown_140 = 0;
}

// The node table's defaults: no nodes.
void ConstructNodeTable(NodeTable* table, uint32_t vtable) {
  table->vtable = vtable;
  Fill(table->words, -1);
  Fill(table->unknown_312, 0);
  Fill(table->unknown_420, -1);
}

void ConstructPairedNodeTable(PairedNodeTable* table, uint32_t vtable) {
  ConstructNodeTable(&table->table, vtable);
  for (NodePair& pair : table->pairs) {
    pair.unknown_0 = 0;
    pair.unknown_4 = -1;
  }
}

void ConstructListPartBase(ListPart* part) {
  part->vtable = kListPartBaseVtable;
  part->unknown_12 = 0;
  part->unknown_4[0] = 0;
  part->unknown_4[1] = 0;
  part->inner_address = 0;
}

void ConstructListPartInnerBase(ListPartInner* inner) {
  inner->unknown_32[0] = 0;
  inner->vtable = kListPartInnerBaseVtable;
  inner->unknown_32[1] = 0;
}

}  // namespace

#define SBX_SCREEN_CONSTRUCTORS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                         \
  if (!On()) {                                 \
    __imp__##name(ctx, base);                  \
    return;                                    \
  }                                            \
  Guest g(ctx, base);                          \
  const uint32_t self = ctx.r3.u32;

// Each returns r3, the object.

// NodeBase::NodeBase.
extern "C" REX_FUNC(sub_82256928) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82256928)
  NodeBase* node = At<NodeBase>(self);
  node->unknown_4 = 0;
  node->vtable = kNodeBaseVtable;
}

// PanelGroupBase::PanelGroupBase.
extern "C" REX_FUNC(sub_8224C888) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224C888)
  ConstructPanelGroupBase(At<PanelGroup>(self));
}

// PanelCBase::PanelCBase.
extern "C" REX_FUNC(sub_8224B148) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224B148)
  ConstructPanelCBase(At<PanelC>(self));
}

// PanelDBase::PanelDBase.
extern "C" REX_FUNC(sub_8224A9F0) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224A9F0)
  ConstructPanelDBase(At<PanelD>(self));
}

// PanelEBase::PanelEBase.
extern "C" REX_FUNC(sub_8224C958) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224C958)
  ConstructPanelEBase(At<PanelE>(self));
}

// PanelE::PanelE: r4 into +0.
extern "C" REX_FUNC(sub_8222E2A0) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8222E2A0)
  PanelE* panel = At<PanelE>(self);
  ConstructPanelEBase(panel);
  panel->unknown_0 = ctx.r4.u32;
  panel->unknown_4 = 4;
  i32_be* w = panel->words;
  for (int i = 0; i < 5; i++) {
    w[2 + i] = 4 + i;  // 4 to 8.
  }
  w[13] = 9;
  w[14] = 5;
  w[15] = 0xF;
  w[16] = 0x10;
  w[17] = 0x50;
  w[18] = 0x51;
  w[20] = 0x52;
  w[21] = 0x3E;
  w[22] = 0x43;
  w[23] = 0x44;
  w[24] = 0x48;
  w[26] = 2;
  w[27] = 0x53;
  w[28] = 3;
  for (int i = 0; i < 4; i++) {
    w[29 + i] = 5 + i;  // 5 to 8.
  }
  w[34] = 1;
  w[35] = 3;
}

// PanelFBase::PanelFBase.
extern "C" REX_FUNC(sub_8224D468) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224D468)
  ConstructPanelFBase(At<PanelF>(self));
}

// Panel6C::Panel6C and Panel7C::Panel7C: r4 into words[1].
extern "C" REX_FUNC(sub_8222F730) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8222F730)
  ConstructPanelC(At<PanelC>(self), ctx.r4.u32, kPanel6C);
}

extern "C" REX_FUNC(sub_8223F270) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8223F270)
  ConstructPanelC(At<PanelC>(self), ctx.r4.u32, kPanel7C);
}

// Panel6D::Panel6D and Panel7D::Panel7D: r4 into +4.
extern "C" REX_FUNC(sub_8222F8A0) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8222F8A0)
  ConstructPanelD(At<PanelD>(self), ctx.r4.u32, kPanel6D);
}

extern "C" REX_FUNC(sub_8223F450) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8223F450)
  ConstructPanelD(At<PanelD>(self), ctx.r4.u32, kPanel7D);
}

// Panel6F::Panel6F and Panel7F::Panel7F (the same): r4 into +4.
void ConstructPanelF(PanelF* panel, uint32_t r4) {
  ConstructPanelFBase(panel);
  panel->vtable = kPanelVtable;
  panel->unknown_4 = r4;
  i32_be* w = panel->words;
  w[1] = 3;
  w[2] = 6;
  w[3] = 0;
  w[4] = 0;
  w[5] = 5;
  w[6] = 6;
  w[7] = 7;
  w[15] = 4;
  w[16] = 1;
  w[17] = 0;
  w[18] = 5;
  w[19] = 0xF;
  w[20] = 0x10;
  w[21] = 0x1D;
  w[22] = 0x1E;
  w[23] = 0x4F;
  w[24] = 0x49;
  w[25] = 2;
  w[27] = 0x4B;
  w[28] = 0x53;
}

extern "C" REX_FUNC(sub_8222E388) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8222E388)
  ConstructPanelF(At<PanelF>(self), ctx.r4.u32);
}

extern "C" REX_FUNC(sub_8223DE58) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8223DE58)
  ConstructPanelF(At<PanelF>(self), ctx.r4.u32);
}

// PanelGroup::PanelGroup, PanelGroup6B::PanelGroup6B and
// PanelGroup7B::PanelGroup7B.
extern "C" REX_FUNC(sub_8222E128) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8222E128)
  ConstructPanelGroup(g, At<PanelGroup>(self), 4, 1);
  ctx.r3.u64 = self;
}

extern "C" REX_FUNC(sub_8222F6C8) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8222F6C8)
  ConstructPanelGroup(g, At<PanelGroup>(self), 28, 4);
  ctx.r3.u64 = self;
}

extern "C" REX_FUNC(sub_8223F218) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8223F218)
  ConstructPanelGroup(g, At<PanelGroup>(self), 28, 2);
  ctx.r3.u64 = self;
}

// PanelSetBase::PanelSetBase.
extern "C" REX_FUNC(sub_82248770) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82248770)
  PanelSet* set = At<PanelSet>(self);
  set->vtable = kPanelSetBaseVtable;
  set->unknown_128 = 0;
  set->unknown_120 = 0;
  set->unknown_200[0] = 0;
  set->unknown_192 = 0;
  set->unknown_784 = 0;
  set->unknown_1300 = 0;
  set->unknown_1816 = 0;
  set->unknown_1876 = 0;
  for (u32_be& word : set->unknown_200) {
    word = 0;
  }
  set->unknown_200[6] = 0xFFFFFFFFu;  // +224.
  set->unknown_1892[6] = 0;           // +1916.
  set->unknown_1920 = 1;
  set->unknown_4 = 0;
  Fill(set->unknown_12, 0);
  *At<u32_be>(kPanelSetGlobal) = 0;
  // +1928 to +1968.
  constexpr int32_t kTail[] = {1, -1, 1, 1, 0, 1, -1, 0, -1, -1, -1};
  for (int i = 0; i < 11; i++) {
    set->unknown_1928[i] = kTail[i];
  }
  Fill(set->unknown_1892, 0);
}

// GroupPair::GroupPair: r4 and r5 its two.
extern "C" REX_FUNC(sub_822477C0) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_822477C0)
  GroupPair* pair = At<GroupPair>(self);
  pair->first = ctx.r4.u32;
  pair->second = ctx.r5.u32;
  pair->vtable = kGroupPairVtable;
}

// MachineScreen::MachineScreen.
extern "C" REX_FUNC(sub_8224A9A8) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224A9A8)
  MachineScreen* screen = At<MachineScreen>(self);
  ConstructMachine(g, screen->machine);
  screen->unknown_9652 = 0;
  screen->vtable = kMachineScreenVtable;
  ctx.r3.u64 = self;
}

// Type6Screen0::Type6Screen0: its state machine.
extern "C" REX_FUNC(sub_82233FE8) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82233FE8)
  ConstructMachine(g, *At<StateMachine>(self));
  ctx.r3.u64 = self;
}

// Type7Screen0::Type7Screen0.
extern "C" REX_FUNC(sub_82243B48) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82243B48)
  VtableMachine* screen = At<VtableMachine>(self);
  ConstructMachine(g, screen->machine);
  screen->vtable = kNodeBaseVtable;
  ctx.r3.u64 = self;
}

// NodeTable::NodeTable, PairedNodeTable::PairedNodeTable and
// ColorNodeTable::ColorNodeTable.
extern "C" REX_FUNC(sub_822457A0) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_822457A0)
  ConstructNodeTable(At<NodeTable>(self), kNodeTableVtable);
}

extern "C" REX_FUNC(sub_82245818) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82245818)
  ConstructPairedNodeTable(At<PairedNodeTable>(self), kPairedNodeTableVtable);
}

extern "C" REX_FUNC(sub_822458D0) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_822458D0)
  ColorNodeTable* table = At<ColorNodeTable>(self);
  ConstructPairedNodeTable(&table->paired, kColorNodeTableVtable);
  Fill(table->unknown_476, -1);
  constexpr uint32_t kColors[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xCD, 0xEB, 0x43, 0xFF};
  for (int i = 0; i < 8; i++) {
    table->colors[i] = kColors[i];
  }
}

// ListScreen::ListScreen.
void ConstructListScreen(Guest& g, ListScreen* screen) {
  ConstructMachine(g, screen->machine);
  screen->vtable = kListScreenVtable;
  screen->unknown_9676 = 0;
  screen->unknown_9680[0] = 1;
  screen->unknown_9680[1] = 1;
  screen->unknown_9708 = 0;
  Fill(screen->unknown_9712, -1);
  screen->unknown_9772[0] = 0;
  screen->unknown_9772[1] = 0;
  screen->unknown_9784 = 0;
  screen->unknown_9788 = 1;
  Fill(screen->unknown_9792, -1);
  screen->unknown_10136 = 0;
  screen->unknown_10140 = -1;
}

extern "C" REX_FUNC(sub_8224FC08) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224FC08)
  ConstructListScreen(g, At<ListScreen>(self));
  ctx.r3.u64 = self;
}

// ListPartBase::ListPartBase, ListPartInnerBase::ListPartInnerBase and
// ListPartInner::ListPartInner.
extern "C" REX_FUNC(sub_8224FD70) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224FD70)
  ConstructListPartBase(At<ListPart>(self));
}

extern "C" REX_FUNC(sub_8224FF58) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_8224FF58)
  ConstructListPartInnerBase(At<ListPartInner>(self));
}

extern "C" REX_FUNC(sub_82250018) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82250018)
  ListPartInner* inner = At<ListPartInner>(self);
  ConstructListPartInnerBase(inner);
  inner->vtable = kListPartInnerVtable;
  inner->unknown_108 = 0;
  inner->unknown_100 = 0;
  inner->unknown_112 = 3;
  inner->unknown_116 = 0;
}

// ListPart::ListPart: its inner part made, and pointed to.
extern "C" REX_FUNC(sub_82250280) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82250280)
  ListPart* part = At<ListPart>(self);
  ConstructListPartBase(part);
  part->vtable = kListPartVtable;
  g.Call(sub_82250018, &part->inner);
  part->inner_address = guest::AddressOf(&part->inner);
  part->unknown_12 = 16;
  ctx.r3.u64 = self;
}

// NodeListScreen::NodeListScreen: a list screen with its own node table.
extern "C" REX_FUNC(sub_822549B0) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_822549B0)
  NodeListScreen* screen = At<NodeListScreen>(self);
  g.Call(sub_8224FC08, &screen->screen);
  screen->screen.vtable = kNodeListScreenVtable;
  screen->unknown_10212 = 0;
  screen->unknown_10204 = 0;
  screen->unknown_10284 = 0;
  screen->unknown_10276 = 0;
  screen->unknown_10356 = 0;
  screen->unknown_10348 = 0;
  g.Call(sub_82245818, &screen->nodes);
  screen->screen.table = guest::AddressOf(&screen->nodes);
  screen->screen.unknown_9680[0] = kNodeListCount;
  screen->nodes.table.unknown_4 = kNodeListCount;
  ctx.r3.u64 = self;
}

// TitledListScreen::TitledListScreen.
extern "C" REX_FUNC(sub_82254B38) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82254B38)
  NodeListScreen* screen = At<NodeListScreen>(self);
  g.Call(sub_822549B0, screen);
  screen->unknown_10820 = 0x39C;
  screen->screen.vtable = kTitledListScreenVtable;
  ctx.r3.u64 = self;
}

// NodeScreen::NodeScreen.
extern "C" REX_FUNC(sub_82257D50) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82257D50)
  NodeScreen* screen = At<NodeScreen>(self);
  ConstructMachine(g, screen->machine);
  g.Call(sub_82245818, &screen->nodes);
  NodeTable& table = screen->nodes.table;
  table.vtable = kNodeScreenVtable;
  table.unknown_4 = 0xF;
  screen->unknown_10100 = 0xF;
  screen->unknown_10192 = 0;
  screen->unknown_10200 = 0;
  screen->unknown_10264 = 0;
  screen->unknown_10272 = 0;
  screen->words[0] = -1;
  Fill(screen->words, -1, 2, 17);
  Fill(screen->words, -1, 19, 67);
  ctx.r3.u64 = self;
}

// Type6NodeScreen::Type6NodeScreen: a node screen with the type 6 menus'
// nodes.
extern "C" REX_FUNC(sub_82235090) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82235090)
  NodeScreen* screen = At<NodeScreen>(self);
  g.Call(sub_82257D50, screen);
  NodeTable& table = screen->nodes.table;
  table.vtable = kType6NodeScreenVtable;
  i32_be* t = table.words;
  t[0] = 0xB;
  t[1] = 0xA;
  for (int i = 2; i <= 28; i++) {
    t[i] = 0x30 + (i - 2);  // 0x30 to 0x4A.
  }
  for (int i = 29; i <= 36; i++) {
    t[i] = 0x4F + (i - 29);  // 0x4F to 0x56.
  }
  for (int i = 37; i <= 63; i++) {
    t[i] = 1 + (i - 37);  // 1 to 0x1B.
  }
  for (int i = 72; i <= 75; i++) {
    t[i] = 6 + (i - 72);  // 6 to 9.
  }
  table.unknown_420[0] = 6;
  table.unknown_420[1] = 0x37;
  i32_be* w = screen->words;
  w[0] = 8;
  w[1] = 5;
  w[2] = 0x65;
  w[5] = 0;
  w[7] = 0x2F;
  w[8] = 0x66;
  w[9] = 0x2B;
  w[12] = 0x2C;
  w[13] = 0x2D;
  w[15] = 0x10;
  w[16] = 0x11;
  w[17] = 0x1D;
  w[18] = 0x1C;
  for (int i = 19; i <= 24; i++) {
    w[i] = 6 + (i - 19);  // 6 to 0xB.
  }
  w[29] = 0x67;
  w[30] = 8;
  w[31] = 9;
  w[32] = 0;
  w[33] = 0xA;
  w[34] = 0xB;
  // Three rows of ten: 0x1E, 0x1F and 0x20, each going up by 5.
  for (int row = 0; row < 3; row++) {
    for (int k = 0; k < 10; k++) {
      w[35 + 10 * row + k] = 0x1E + row + 5 * k;
    }
  }
  w[65] = 0;
  w[66] = 1;
  w[67] = 0xF;
  w[68] = 0;
  ctx.r3.u64 = self;
}

// Type6Screens::Type6Screens: the type 6 menus' screens made.
extern "C" REX_FUNC(sub_82234B50) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_82234B50)
  Type6Screens* screens = At<Type6Screens>(self);
  screens->vtable = kType6ScreensVtable;
  g.Call(sub_82233FE8, &screens->screen_0);
  ConstructMachine(g, screens->machine_9672);
  g.Call(sub_82230D80, screens->list_screen);
  g.Call(sub_82235090, &screens->node_screen);
  g.Call(sub_8222E480, screens->panel_screen);
  ConstructMachine(g, screens->screen_52692.slot.machine);
  screens->screen_52692.vtable = kType6Screen52692Vtable;
  for (auto& screen : screens->screens) {
    ConstructMachine(g, screen.machine);
  }
  screens->unknown_91260[0] = 0;
  screens->unknown_91256 = 0;
  screens->unknown_91260[1] = 0;
  ctx.r3.u64 = self;
}

// Type7Screens::Type7Screens: the type 7 menus' screens made.
extern "C" REX_FUNC(sub_822446C8) {
  SBX_SCREEN_CONSTRUCTORS_FALLBACK(sub_822446C8)
  Type7Screens* screens = At<Type7Screens>(self);
  screens->vtable = kType7ScreensVtable;
  g.Call(sub_82243B48, &screens->screen_0);
  ConstructMachine(g, screens->machine_9664);
  g.Call(sub_82240758, screens->list_screen);
  g.Call(sub_8223DF38, screens->panel_screen);
  ConstructMachine(g, screens->screen_42352.slot.machine);
  screens->screen_42352.vtable = kType7Screen42352Vtable;
  for (auto& screen : screens->screens) {
    ConstructMachine(g, screen.machine);
  }
  screens->unknown_71280[0] = 0;
  screens->unknown_71276 = 0;
  screens->unknown_71280[1] = 0;
  ctx.r3.u64 = self;
}
