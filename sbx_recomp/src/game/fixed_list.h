// The game's fixed-size lists: N nodes linked by index (signed halfwords,
// -1 for none), a free list and the list itself through the same nodes,
// and N items the nodes' slots index.

#pragma once

#include "game/guest_types.h"

namespace game {

template <int N, typename T>
struct FixedList {
  struct Node {
    guest::i16_be next;  // The next node, or -1.
    guest::i16_be slot;  // Which item it holds.
  };

  Node nodes[N];
  guest::i16_be free_head;
  guest::i16_be unknown_free;  // -1 once reset.
  guest::i16_be head;
  guest::i16_be unknown_head;  // -1 once reset.
  guest::i16_be count;
  guest::i16_be unknown_count;
  T items[N];

  // A node by its index. An index of -1 (or past the end) is used as the
  // game uses it: the memory at that place.
  Node* NodeAt(int32_t index) {
    return guest::Element<Node>(guest::AddressOf(nodes), uint32_t(index));
  }
  T* ItemAt(int32_t slot) {
    return guest::Element<T>(guest::AddressOf(items), uint32_t(slot));
  }

  // The node `steps` along from the head.
  Node* Walk(uint32_t steps) {
    Node* node = NodeAt(head);
    for (uint32_t n = steps; n != 0; n--) {
      node = NodeAt(node->next);
    }
    return node;
  }
  T* ItemAtPosition(uint32_t position) { return ItemAt(Walk(position)->slot); }

  // Every node free, the list empty.
  void Reset() {
    for (int i = 0; i < N; i++) {
      nodes[i].slot = int16_t(i);
      nodes[i].next = int16_t(i + 1);
    }
    nodes[N - 1].next = -1;
    free_head = 0;
    unknown_free = -1;
    head = -1;
    unknown_head = -1;
    count = 0;
  }

  // Takes the first free node and links it in after `position` nodes (at
  // the head for 0). The list isn't checked for being full.
  void Link(int16_t position) {
    guest::i16_be* after = &head;
    if (position != 0) {
      int32_t index = head;
      for (uint32_t n = uint32_t(int32_t(position));;) {
        Node* node = NodeAt(index);
        after = &node->next;
        if (--n == 0) {
          break;
        }
        index = node->next;
      }
    }
    const int16_t taken = free_head;
    Node* node = NodeAt(taken);
    free_head = node->next;
    node->next = *after;
    *after = taken;
    count += 1;
  }

  void Insert(const T& item, uint32_t position) {
    Link(int16_t(position));
    *ItemAtPosition(position) = item;
  }

  template <typename V>
  bool Contains(V value) {
    const T item(value);
    for (uint32_t i = 0; i < uint32_t(int32_t(count)); i++) {
      if (*ItemAtPosition(i) == item) {
        return true;
      }
    }
    return false;
  }
};

}  // namespace game
