// The archive's list objects as our own C++ (--native_archive, on by
// default): a list node's and a list's constructors.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_archive, true, "SBX",
                    "Our own C++ for the archive's list constructors instead of the "
                    "recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kListNodeVtable = 0x82000A40;
constexpr uint32_t kListVtable = 0x820125F0;

struct ListNode {
  u32_be vtable;
  u32_be owner;
};

struct List {
  u32_be vtable;
  u32_be owner;
  u32_be first;
  u32_be last;
  u32_be count;
};

bool On() { return REXCVAR_GET(native_archive); }

}  // namespace

// ListNode::ListNode: r3 the node, r4 its owner. Returns it.
extern "C" REX_FUNC(sub_822925A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822925A8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  ListNode* node = At<ListNode>(ctx.r3.u32);
  node->owner = ctx.r4.u32;
  node->vtable = kListNodeVtable;
}

// List::List: r3 the list, r4 its owner; empty. Returns it.
extern "C" REX_FUNC(sub_822925C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822925C0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  List* list = At<List>(ctx.r3.u32);
  list->owner = ctx.r4.u32;
  list->vtable = kListVtable;
  list->first = 0;
  list->last = 0;
  list->count = 0;
}
