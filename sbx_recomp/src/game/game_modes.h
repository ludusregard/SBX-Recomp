// The game modes: the state machines every mode's messages go through, the
// modes themselves, and the race mode's members.

#pragma once

#include "game/fixed_list.h"
#include "game/guest_types.h"
#include "game/math.h"
#include "game/model.h"

namespace game {

using guest::i32_be;
using guest::ptr;
using guest::u32_be;

// A state machine's key: the state, then the message's three words.
struct StateKey {
  u32_be words[4];

  bool operator==(const StateKey& other) const {
    for (int i = 0; i < 4; i++) {
      if (words[i] != other.words[i]) {
        return false;
      }
    }
    return true;
  }
};

// What a key leads to: a functor (its object kept inside it), and the state
// to go to (0 to stay).
struct Handler {
  u32_be target;     // The functor's object: `object` here.
  u32_be object[6];  // Its first word is its vtable.
  u32_be next_state;
};
GUEST_SIZE(Handler, 32);

struct StateEntry {
  StateKey key;
  Handler handler;
};
GUEST_SIZE(StateEntry, 48);

struct StateMachine {
  static constexpr uint32_t kEntries = 200;

  StateEntry entries[kEntries];
  u32_be count;
  u32_be state;
};
GUEST_SIZE(StateMachine, 9608);

// The race overlays' machine: a state machine, then its overlays.
struct RaceOverlay {
  uint8_t unknown_0[772];
  u32_be unknown_772;
  u32_be unknown_776;
  u32_be unknown_780;
};

struct RaceOverlaysMachine {
  static constexpr int kOverlays = 25;

  StateMachine machine;
  ptr<RaceOverlay> overlays[kOverlays];
};

// A mode: a base with two members, each with its own vtable.
struct Mode {
  u32_be vtable;
  u32_be member;  // Points at member_vtable's object.
  u32_be member_vtable;
  uint8_t unknown_12[20];
  u32_be member2_vtable;
};
GUEST_OFFSET(Mode, member2_vtable, 32);

struct Modes {
  u32_be vtable;
  u32_be unknown_4;
};

// The object many of the modes' members are built on: a vtable, two words,
// and 18 points (their w set to 1 by its constructor).
struct GameObject {
  u32_be vtable;
  uint8_t unknown_4[12];
  u32_be unknown_16;
  u32_be unknown_20;
  uint8_t unknown_24[8];
  Vector points[14];
  uint8_t unknown_256[16];
  Vector points_272[4];
};
GUEST_SIZE(GameObject, 336);
GUEST_OFFSET(GameObject, vtable, 0);
GUEST_OFFSET(GameObject, points, 32);
GUEST_OFFSET(GameObject, points_272, 272);

struct GameObject384 {
  GameObject object;
  uint8_t unknown_336[16];
  Vector unknown_352;
  uint8_t unknown_368[16];
};
GUEST_SIZE(GameObject384, 384);
GUEST_OFFSET(GameObject384, object, 0);

struct GameObject1104 {
  GameObject object;
  uint8_t unknown_336[752];
  Vector unknown_1088;
};
GUEST_SIZE(GameObject1104, 1104);
GUEST_OFFSET(GameObject1104, object, 0);

struct Buffer28 {
  u32_be vtable;
  uint8_t unknown_4[8];
  i32_be unknown_12;
  i32_be unknown_16;
  u32_be block;  // 32 KB, freed by Buffer28::Free.
  u32_be unknown_24;
};
GUEST_SIZE(Buffer28, 28);
GUEST_OFFSET(Buffer28, vtable, 0);
GUEST_OFFSET(Buffer28, block, 20);

struct XPRHolder {
  u32_be unknown_0;
  XPR first;
  u32_be unknown_28;
  XPR second;
};
GUEST_SIZE(XPRHolder, 56);
GUEST_OFFSET(XPRHolder, first, 4);
GUEST_OFFSET(XPRHolder, second, 32);


struct Member13360 {
  GameObject object;
  uint8_t unknown_336[24];
  i32_be unknown_360;
  uint8_t unknown_364[20];
  Vector unknown_384[3];
  uint8_t unknown_432[16];
  u32_be unknown_448;
  u32_be unknown_452;
  uint8_t unknown_456[42560];
  Buffer28 buffers[4];
  uint8_t unknown_43128[8];
  GameObject384 unknown_43136;
};
GUEST_SIZE(Member13360, 43520);
GUEST_OFFSET(Member13360, object, 0);
GUEST_OFFSET(Member13360, buffers, 43016);

struct Member56896 {
  GameObject object;
  uint8_t unknown_336[10444];
  XPRHolder xprs;
};
GUEST_SIZE(Member56896, 10836);
GUEST_OFFSET(Member56896, object, 0);
GUEST_OFFSET(Member56896, xprs, 10780);

struct Entry20 {
  u32_be words[5];
};
GUEST_SIZE(Entry20, 20);
GUEST_OFFSET(Entry20, words, 0);

struct Member67760 {
  GameObject object;
  uint8_t unknown_336[16];
  FixedList<16, Entry20> list;
  u32_be unknown_748;
  u32_be unknown_752;
  u32_be unknown_756;
};
GUEST_SIZE(Member67760, 760);
GUEST_OFFSET(Member67760, object, 0);
GUEST_OFFSET(Member67760, list, 352);

struct Block1104 {
  uint8_t unknown_0[1088];
  Vector unknown_1088;
};
GUEST_SIZE(Block1104, 1104);

struct RaceModeMember {
  u32_be vtable;
  uint8_t unknown_4[12];
  StateMachine machine;
  uint8_t unknown_9624[24];
  GameObject1104 unknown_9648[2];
  GameObject1104 unknown_11856;
  uint8_t unknown_12960[16];
  GameObject384 unknown_12976;
  Member13360 unknown_13360;
  Member56896 unknown_56880;
  uint8_t unknown_67716[28];
  Member67760 unknown_67744;
  uint8_t unknown_68504[8];
  Block1104 unknown_68512[8];
  uint8_t unknown_77344[16];
  i32_be unknown_77360;
  u32_be unknown_77364[3];
};
GUEST_SIZE(RaceModeMember, 77376);
GUEST_OFFSET(RaceModeMember, vtable, 0);
GUEST_OFFSET(RaceModeMember, machine, 16);


}  // namespace game
