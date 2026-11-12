// The input clients and action mappers as our own C++
// (--native_action_mappers, on by default): the input clients' and their
// lists' constructors, a client added, and an action mapper's stages
// reset.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_action_mappers, true, "SBX",
                    "Our own C++ for the input clients' set-up and the action mappers' stage "
                    "reset instead of the recompiled functions");

namespace {

using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kClientNodeVtable = 0x8206E160;
constexpr uint32_t kInputClientVtable = 0x82007E70;
constexpr uint32_t kClientListVtable = 0x8206E1A0;
constexpr uint32_t kInputClientListVtable = 0x82007EB0;
// The client list (the last one made).
constexpr uint32_t kClientList = 0x82973074;

// A client of the input layer, and its list.
struct ClientNode {
  u32_be vtable;
  u32_be owner;
  u32_be next;
};

struct ClientList {
  u32_be vtable;
  u32_be unknown_4[8];
  u32_be owner;  // The input client list's own.
};

// One of an action mapper's 30 stages.
struct ActionStage {
  f32_be unknown_0[2];
  u32_be unknown_8[2];
  uint8_t unknown_16;
  uint8_t pad_17[3];
};
GUEST_SIZE(ActionStage, 20);

struct ActionMapper {
  uint8_t unknown_0[624];
  uint8_t member_624[780];
  ActionStage stages[30];
};
GUEST_OFFSET(ActionMapper, stages, 1404);

bool On() { return REXCVAR_GET(native_action_mappers); }

void ConstructNode(ClientNode* node, uint32_t owner) {
  node->owner = owner;
  node->next = 0;
  node->vtable = kClientNodeVtable;
}

void ConstructList(ClientList* list) {
  for (u32_be& word : list->unknown_4) {
    word = 0;
  }
  list->vtable = kClientListVtable;
  *At<u32_be>(kClientList) = guest::AddressOf(list);
}

}  // namespace

#define SBX_ACTION_MAPPERS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                      \
  if (!On()) {                              \
    __imp__##name(ctx, base);               \
    return;                                 \
  }                                         \
  Guest g(ctx, base);

// InputClient::InputClient and ClientNode::ClientNode: r3 the client, r4
// its owner. Return r3.
extern "C" REX_FUNC(sub_8210AF88) {
  SBX_ACTION_MAPPERS_FALLBACK(sub_8210AF88)
  ClientNode* client = At<ClientNode>(ctx.r3.u32);
  ConstructNode(client, ctx.r4.u32);
  client->vtable = kInputClientVtable;
}

extern "C" REX_FUNC(sub_82581DE0) {
  SBX_ACTION_MAPPERS_FALLBACK(sub_82581DE0)
  ConstructNode(At<ClientNode>(ctx.r3.u32), ctx.r4.u32);
}

// InputClientList::InputClientList: r3 the list, r4 its owner. Returns r3.
extern "C" REX_FUNC(sub_8210AFD8) {
  SBX_ACTION_MAPPERS_FALLBACK(sub_8210AFD8)
  ClientList* list = At<ClientList>(ctx.r3.u32);
  ConstructList(list);
  list->owner = ctx.r4.u32;
  list->vtable = kInputClientListVtable;
}

// ClientList::ClientList: r3 the list. Returns it.
extern "C" REX_FUNC(sub_82581ED0) {
  SBX_ACTION_MAPPERS_FALLBACK(sub_82581ED0)
  ConstructList(At<ClientList>(ctx.r3.u32));
}

// InputClientList::Add: r4 the client (ClientList::Add, recompiled, with
// r4 as given).
extern "C" REX_FUNC(sub_8210B028) {
  SBX_ACTION_MAPPERS_FALLBACK(sub_8210B028)
  g.Call(sub_82582078, ctx.r4.u32);
}

// ActionMapper::ResetStages: r3 the mapper.
extern "C" REX_FUNC(sub_8210C1B0) {
  SBX_ACTION_MAPPERS_FALLBACK(sub_8210C1B0)
  ActionMapper* mapper = At<ActionMapper>(ctx.r3.u32);
  g.Call(sub_8210BC70, mapper);
  g.Call(sub_8210BBE8, mapper->member_624);
  for (ActionStage& stage : mapper->stages) {
    stage.unknown_0[0] = 0.0f;
    stage.unknown_0[1] = 0.0f;
    stage.unknown_8[0] = 0;
    stage.unknown_8[1] = 0;
    stage.unknown_16 = 0;
  }
}
