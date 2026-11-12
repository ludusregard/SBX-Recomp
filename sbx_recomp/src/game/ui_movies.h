// A UI movie: its movie file's tables, its animations' queues and the
// tracks that move its nodes.

#pragma once

#include "game/fixed_list.h"
#include "game/guest_types.h"

namespace game {

using guest::f32_be;
using guest::ptr;
using guest::u32_be;

// A movie file's 8-byte entries, counted from its start: the animations'
// from +32 (entry 4 on, after +16's count), the node track lists', key
// lists' and sub-animations' from +64 (entry 8 on, after the counts at
// +32, +40 and +48).
struct MovieFile {
  struct Entry {
    u32_be value;
    uint8_t unknown_4[4];
  };
  static constexpr uint32_t kAnimationsFirst = 4;
  static constexpr uint32_t kListsFirst = 8;

  uint8_t unknown_0[16];
  u32_be animation_count;
  uint8_t unknown_20[12];
  u32_be node_track_list_count;
  uint8_t unknown_36[4];
  u32_be key_list_count;
  uint8_t unknown_44[4];
  u32_be sub_animation_count;

  // The entry `first + count + index`.
  uint32_t EntryAfter(uint32_t count, uint32_t index, uint32_t first) {
    return guest::Element<Entry>(guest::AddressOf(this), count + index + first)->value;
  }
};
GUEST_OFFSET(MovieFile, animation_count, 16);
GUEST_OFFSET(MovieFile, node_track_list_count, 32);
GUEST_OFFSET(MovieFile, key_list_count, 40);
GUEST_OFFSET(MovieFile, sub_animation_count, 48);

struct MovieAnimation {
  uint8_t unknown_0[12];
  f32_be length;
};
GUEST_OFFSET(MovieAnimation, length, 12);

// A running animation.
struct AnimationSlot {
  uint8_t unknown_0[12];
  u32_be file;
  ptr<MovieAnimation> animation;
  f32_be time;
  u32_be with_next;      // The next in the queue runs with it.
  u32_be backwards;
  u32_be remove_at_end;  // Taken off the queue when it ends.
  u32_be hold;           // Its time is held.
};
GUEST_SIZE(AnimationSlot, 40);
GUEST_OFFSET(AnimationSlot, file, 12);

using AnimationList = FixedList<32, AnimationSlot>;
static_assert(sizeof(AnimationList) == 1420);

// One of the movie's 1,024 named animations (36 bytes; made by
// sub_8211D3D0, destroyed by sub_820913F0).
struct NamedAnimation {
  uint8_t unknown_0[36];
};
GUEST_SIZE(NamedAnimation, 36);

// A node, as far as the movie's lookups read it.
struct MovieNode {
  uint8_t unknown_0[4];
  u32_be unknown_4;
  uint8_t unknown_8[16];
  u32_be unknown_24;
};
GUEST_OFFSET(MovieNode, unknown_24, 24);

struct UiMovie {
  static constexpr int kNamedAnimations = 1024;

  uint8_t unknown_0[16];
  u32_be state_16;
  u32_be state_20;
  uint8_t unknown_24[4];
  uint8_t member_28[12];  // Made by sub_8211AFE8.
  uint8_t member_40[8];   // Made by sub_8211B1E8.
  u32_be unknown_48;
  uint8_t unknown_52[12];
  uint8_t member_64[112];  // Made by sub_82583028.
  NamedAnimation named[kNamedAnimations];
  u32_be unknown_37040;
  AnimationList queue;
  AnimationList looping;
  u32_be node_map[257];  // Looked up with sub_8211B4E0.
};
GUEST_OFFSET(UiMovie, state_16, 16);
GUEST_OFFSET(UiMovie, member_28, 28);
GUEST_OFFSET(UiMovie, member_64, 64);
GUEST_OFFSET(UiMovie, named, 176);
GUEST_OFFSET(UiMovie, unknown_37040, 37040);
GUEST_OFFSET(UiMovie, queue, 37044);
GUEST_OFFSET(UiMovie, looping, 38464);
GUEST_OFFSET(UiMovie, node_map, 39884);

// A node property's descriptor (sub_82585560).
struct PropertyDescriptor {
  enum Type : uint32_t { kFloat = 0, kSigned = 1, kUnsigned = 2 };

  u32_be offset;  // In the node.
  u32_be type;
  uint8_t set;       // Set, not added to.
  uint8_t in_array;  // In the node's array (found with VertexOffset).
};
GUEST_OFFSET(PropertyDescriptor, set, 8);

// A track's key: how it blends to the next (0 a step, 1 a line, 2 and 3
// curves), its value (a float's or a word's bits), a curve's two floats.
struct TrackKey {
  u32_be blend;
  uint8_t unknown_4[4];
  u32_be value;
  f32_be curve_12;
  f32_be curve_16;
};
GUEST_SIZE(TrackKey, 20);

struct MovieTrack {
  u32_be property;
  uint8_t unknown_4[8];
  u32_be index;  // In the node's array.
};
GUEST_OFFSET(MovieTrack, index, 12);

// The node a track moves, as far as ApplyTrack reads it.
struct TrackNode {
  uint8_t unknown_0[112];
  u32_be array;  // VertexOffset's base.
};
GUEST_OFFSET(TrackNode, array, 112);

}  // namespace game
