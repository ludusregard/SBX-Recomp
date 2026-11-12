// The C++ library's containers as the game instantiated them on its heap.

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::u32_be;
using guest::ptr;

// std::string. Short text is kept inside the string; longer text in a block
// of capacity + 1 bytes from operator new. The text always ends with a 0.
struct String {
  static constexpr uint32_t kInPlaceBytes = 16;
  static constexpr uint32_t kMaxLength = 0xFFFFFFFE;

  u32_be allocator;  // Never read.
  union {
    char in_place[kInPlaceBytes];
    ptr<char> block;
  };
  u32_be length;
  u32_be capacity;  // 15 while the text is in place.

  bool HasBlock() const { return capacity >= kInPlaceBytes; }
  char* Text() { return HasBlock() ? block.get() : in_place; }

  // Whether `text` points into this string's own text.
  bool Contains(const char* text) {
    return text >= Text() && text < Text() + length;
  }

  void SetLength(uint32_t new_length) {
    length = new_length;
    Text()[new_length] = 0;
  }
};
GUEST_SIZE(String, 28);
GUEST_OFFSET(String, length, 20);

// A node of std::list<void*>.
struct PtrListNode {
  ptr<PtrListNode> next;
  ptr<PtrListNode> previous;
  u32_be value;
};
GUEST_SIZE(PtrListNode, 12);

// std::list<void*>: a ring of nodes through a head node that holds no
// value. The head links to itself while the list is empty.
struct PtrList {
  u32_be allocator;  // Never read.
  ptr<PtrListNode> head;
  u32_be size;
};
GUEST_SIZE(PtrList, 12);

// A list iterator as the game passes it in one register: the list in the
// high word, the node in the low one.
struct PtrListIterator {
  ptr<PtrList> list;
  ptr<PtrListNode> node;
};
GUEST_SIZE(PtrListIterator, 8);

}  // namespace game
