// The C++ library's list of pointers as our own C++ (--native_std_list, on
// by default): the list the game's managers keep their listeners and
// objects in (game/std_types.h, PtrList).
//
// Nodes come from operator new and go back with operator delete, both
// recompiled. The error paths (an allocation count too big, a list too
// long, an iterator that isn't the list's) are left to the recompiled
// functions, run from the start.

#include "game/std_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_std_list, true, "SBX",
                    "Our own C++ for the C++ library's list of pointers (a node allocated, a "
                    "list made, insert, erase) instead of the recompiled functions; operator "
                    "new and delete stay recompiled");

namespace {

using game::PtrList;
using game::PtrListIterator;
using game::PtrListNode;
using guest::At;
using guest::Guest;

constexpr uint32_t kMaxSize = 0x3FFFFFFF;

bool On() { return REXCVAR_GET(native_std_list); }

void* OperatorNew(Guest& g, uint32_t bytes) { return At<void>(g.Call(sub_8258C000, bytes)); }
void OperatorDelete(Guest& g, void* block) { g.Call(sub_8258C868, block); }

PtrListNode* NewNode(Guest& g) {
  return At<PtrListNode>(g.Call(sub_820993A8, 1u, 0u));  // PtrList::AllocateNodes
}

void IncreaseSize(Guest& g, PtrList* list, uint32_t count) {
  g.Call(sub_82099A40, list, count);  // PtrList::IncSize
}

// Makes the list empty: a head node linked to itself.
void Construct(Guest& g, PtrList* list) {
  PtrListNode* head = NewNode(g);
  head->next = head;
  head->previous = head;
  list->head = head;
  list->size = 0;
}

// Links a new node holding `value` in before `where`.
void Insert(Guest& g, PtrList* list, PtrListNode* where, uint32_t value) {
  PtrListNode* previous = where->previous;
  PtrListNode* node = NewNode(g);
  node->next = where;
  node->previous = previous;
  node->value = value;
  IncreaseSize(g, list, 1);
  where->previous = node;
  previous->next = node;
}

// Unlinks and frees `node` (the head is left alone). Returns the node
// after it.
PtrListNode* Erase(Guest& g, PtrList* list, PtrListNode* node) {
  PtrListNode* next = node->next;
  if (node != list->head) {
    node->previous->next = node->next;
    node->next->previous = node->previous;
    OperatorDelete(g, node);
    --list->size;
  }
  return next;
}

}  // namespace

// PtrList::AllocateNodes: r3 the count. A count whose bytes don't fit a
// word throws (the original's path).
extern "C" REX_FUNC(sub_820993A8) {
  REX_FUNC_PROLOGUE();
  const uint32_t count = ctx.r3.u32;
  if (!On() || (count != 0 && 0xFFFFFFFFu / count < sizeof(PtrListNode))) {
    __imp__sub_820993A8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  OperatorNew(g, count * sizeof(PtrListNode));
}

// PtrList::IncSize: r3 the list, r4 how many. A size that would pass the
// most throws (the original's path).
extern "C" REX_FUNC(sub_82099A40) {
  REX_FUNC_PROLOGUE();
  Guest g(ctx, base);
  PtrList* list = At<PtrList>(ctx.r3.u32);
  const uint32_t count = ctx.r4.u32;
  if (!On() || kMaxSize - list->size < count) {
    __imp__sub_82099A40(ctx, base);
    return;
  }
  list->size += count;
}

// PtrList::PtrList: r3 the list. Returns it.
extern "C" REX_FUNC(sub_82099420) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82099420(ctx, base);
    return;
  }
  Guest g(ctx, base);
  PtrList* list = At<PtrList>(ctx.r3.u32);
  Construct(g, list);
  ctx.r3.u64 = guest::AddressOf(list);
}

// PtrList::Insert: r3 the list, r4 the node to put it before, r5 the
// value's address.
extern "C" REX_FUNC(sub_82099AE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82099AE0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  Insert(g, At<PtrList>(ctx.r3.u32), At<PtrListNode>(ctx.r4.u32),
         *At<guest::u32_be>(ctx.r5.u32));
}

// PtrList::Erase: r3 where the result iterator goes, r4 the list, r5 the
// iterator. The result is the iterator moved to the next node. An iterator
// with no list, or at its own list's head, traps (the original's path).
extern "C" REX_FUNC(sub_82118970) {
  REX_FUNC_PROLOGUE();
  Guest g(ctx, base);
  PtrListIterator* result = At<PtrListIterator>(ctx.r3.u32);
  PtrList* list = At<PtrList>(ctx.r4.u32);
  PtrList* iterator_list = At<PtrList>(uint32_t(ctx.r5.u64 >> 32));
  PtrListNode* node = At<PtrListNode>(ctx.r5.u32);
  if (!On() || iterator_list == nullptr || node == iterator_list->head) {
    __imp__sub_82118970(ctx, base);
    return;
  }
  result->list = iterator_list;
  result->node = Erase(g, list, node);
  ctx.r3.u64 = guest::AddressOf(result);
}
