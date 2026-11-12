// The app's state layers as our own C++ (--native_app_states, on by
// default): a message sent, the state layers' constructors and their
// interface's thunks, and the app states' updates.

#include "game/std_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_app_states, true, "SBX",
                    "Our own C++ for the app's state layers (their constructors, thunks and "
                    "updates) instead of the recompiled functions");

namespace {

using game::PtrList;
using guest::At;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kStateLayerVtable = 0x820028B4;
constexpr uint32_t kStateLayerInterfaceVtable = 0x82002884;

// A state layer: its own vtable, an interface (with its own vtable), its
// owner and two lists.
struct StateLayer {
  u32_be vtable;
  u32_be interface_vtable;  // The interface the thunks below come in by.
  u32_be owner;
  PtrList list_a;
  PtrList list_b;
  uint8_t member_36[4];  // Built by sub_820ACF40.
};
GUEST_OFFSET(StateLayer, list_a, 12);
GUEST_OFFSET(StateLayer, member_36, 36);

struct App {
  uint8_t unknown_0[56];
  uint8_t messages[4];  // Its message queue.
};

struct AppStates {
  uint8_t unknown_0[8];
  u32_be layer;  // The state layer updated.
};

bool On() { return REXCVAR_GET(native_app_states); }

void ConstructStateLayer(Guest& g, StateLayer* layer, uint32_t owner) {
  layer->vtable = kStateLayerVtable;
  layer->interface_vtable = kStateLayerInterfaceVtable;
  layer->owner = owner;
  g.Call(sub_82099420, &layer->list_a);  // PtrList::PtrList
  g.Call(sub_82099420, &layer->list_b);
  g.Call(sub_820ACF40, layer->member_36);
}

// The layer from its interface's address.
uint32_t LayerOf(uint32_t interface_address) {
  return interface_address - offsetof(StateLayer, interface_vtable);
}

}  // namespace

#define SBX_APP_STATES_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  Guest g(ctx, base);

// App::SendMessage: r3 the app, r4 on as given; onto its message queue.
extern "C" REX_FUNC(sub_820950D8) {
  SBX_APP_STATES_FALLBACK(sub_820950D8)
  g.Call(sub_82094B08, At<App>(ctx.r3.u32)->messages);
}

// StateLayer::StateLayer: r3 the layer, r4 its owner. Returns it.
extern "C" REX_FUNC(sub_820ACFA0) {
  SBX_APP_STATES_FALLBACK(sub_820ACFA0)
  StateLayer* layer = At<StateLayer>(ctx.r3.u32);
  ConstructStateLayer(g, layer, ctx.r4.u32);
  ctx.r3.u64 = guest::AddressOf(layer);
}

// StateLayer::StateLayer_2: the same, then r5 set on it (recompiled).
extern "C" REX_FUNC(sub_820ADA60) {
  SBX_APP_STATES_FALLBACK(sub_820ADA60)
  StateLayer* layer = At<StateLayer>(ctx.r3.u32);
  const uint32_t r5 = ctx.r5.u32;
  ConstructStateLayer(g, layer, ctx.r4.u32);
  g.Call(sub_820AD958, layer, r5);
  ctx.r3.u64 = guest::AddressOf(layer);
}

// The interface's thunks: r3 the interface; the layer's own function.
extern "C" REX_FUNC(sub_820AD5E8) {  // StateLayer_state_slot_5
  SBX_APP_STATES_FALLBACK(sub_820AD5E8)
  g.Call(sub_820AD508, LayerOf(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_820ADFA8) {  // StateLayer_state_slot_8
  SBX_APP_STATES_FALLBACK(sub_820ADFA8)
  g.Call(sub_820ADD88, LayerOf(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_820ADFB0) {  // StateLayer_state_slot_9
  SBX_APP_STATES_FALLBACK(sub_820ADFB0)
  g.Call(sub_820ADE98, LayerOf(ctx.r3.u32));
}

// AppStates::OnUpdate and OnUpdateToggled: r3 the app states; their layer
// updated (recompiled).
extern "C" REX_FUNC(sub_820AE040) {
  SBX_APP_STATES_FALLBACK(sub_820AE040)
  g.Call(sub_820ADD88, At<AppStates>(ctx.r3.u32)->layer);
}

extern "C" REX_FUNC(sub_820AE048) {
  SBX_APP_STATES_FALLBACK(sub_820AE048)
  g.Call(sub_820ADE98, At<AppStates>(ctx.r3.u32)->layer);
}
