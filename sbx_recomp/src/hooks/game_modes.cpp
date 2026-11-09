// The game modes' state machines as our own C++ (--native_game_modes, on
// by default): the handler table every mode's messages go through (a key,
// its handler found, added), the machines' and modes' constructors, and
// the race overlays machine's flags.
//
// Written from SBX's generated code. A state machine holds up to 200
// entries of 48 bytes and their count at +9600. An entry: +0 its 16-byte
// key (four words), +16 its handler (a functor: +16 points at its object
// at +20, whose first word is its vtable; +44 its last word). The table's
// dispatch, StateMachine::Handle (sub_82106410), keeps title_screen.cpp's
// override (autosave and the boot mode log), which calls ours below in
// place of the recompiled function; the race mode's and the overlays
// machine's own set-ups stay recompiled.

#include "guest.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_game_modes, true, "SBX",
                    "Our own C++ for the game modes' state machines (a message's handler "
                    "found and added, the machines' and modes' constructors, the race "
                    "overlays' flags) instead of the recompiled functions");

namespace {

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kEntryBytes = 48;
constexpr uint32_t kEntries = 200;
constexpr uint32_t kCount = 9600;
constexpr uint32_t kEmptyHandlerVtable = 0x82006954;

// Find's default handler (an empty one), made once.
constexpr uint32_t kDefaultHandler = 0x826B5848;
constexpr uint32_t kDefaultHandlerMade = 0x826B5868;  // bit 0

// Modes::Modes's globals.
constexpr uint32_t kModesVtable = 0x82006A88;
constexpr uint32_t kModesOwner = 0x825F3264;   // and +4: both the owner
constexpr uint32_t kModesWord = 0x826B586C;
constexpr uint32_t kModesWord2 = 0x826AA81C;
constexpr uint32_t kModesWords = 0x826AA820;  // 7 words
constexpr uint32_t kModesWord3 = 0x826AA850;
constexpr uint32_t kModesWord4 = 0x826B5870;

// Mode::Mode's vtables.
constexpr uint32_t kModeBaseVtable = 0x82006ADC;
constexpr uint32_t kModeVtable = 0x82006990;
constexpr uint32_t kModeMemberVtable = 0x82006AB8;
constexpr uint32_t kModeMember2Vtable = 0x82006984;

// The race overlays machine: 25 overlays at +9608.
constexpr uint32_t kOverlays = 9608;
constexpr uint32_t kOverlayCount = 25;

// StateKey::Equals (sub_821061D0): the four words.
bool KeyEquals(uint8_t* base, uint32_t a, uint32_t b) {
  for (uint32_t i = 0; i < 16; i += 4) {
    if (LoadU32(base, a + i) != LoadU32(base, b + i)) {
      return false;
    }
  }
  return true;
}

// StateMachine::IndexOf (sub_82106230): the key's entry, or -1.
int32_t IndexOf(uint8_t* base, uint32_t machine, uint32_t key) {
  const uint32_t count = LoadU32(base, machine + kCount);
  for (uint32_t i = 0; i < count; i++) {
    if (KeyEquals(base, machine + kEntryBytes * i, key)) {
      return int32_t(i);
    }
  }
  return -1;
}

// StateMachine::Add (sub_821062E8): r4 the key, r5 the handler (a functor
// as an entry's +16 holds one). A key that's there gets the handler's
// object (its six words from +4) and last word; else a new entry (the
// count up by one) with the key and the handler, its +16 pointing at its
// own object.
void Add(uint8_t* base, uint32_t machine, uint32_t key, uint32_t handler) {
  const int32_t index = IndexOf(base, machine, key);
  if (index != -1) {
    const uint32_t to = machine + kEntryBytes * uint32_t(index) + 16;
    for (uint32_t i = 0; i < 24; i += 4) {
      StoreU32(base, to + 4 + i, LoadU32(base, handler + 4 + i));
    }
    StoreU32(base, to + 0, to + 4);
    StoreU32(base, to + 28, LoadU32(base, handler + 28));
    return;
  }
  // The original builds the entry in its frame first (sub_82106290), then
  // copies it in (sub_82106170): every word is read before any is written.
  uint32_t words[4 + 6 + 1];
  for (uint32_t i = 0; i < 4; i++) {
    words[i] = LoadU32(base, key + 4 * i);
  }
  for (uint32_t i = 0; i < 6; i++) {
    words[4 + i] = LoadU32(base, handler + 4 + 4 * i);
  }
  words[10] = LoadU32(base, handler + 28);
  const uint32_t count = LoadU32(base, machine + kCount);
  StoreU32(base, machine + kCount, count + 1);
  const uint32_t entry = machine + kEntryBytes * count;
  for (uint32_t i = 0; i < 4; i++) {
    StoreU32(base, entry + 4 * i, words[i]);
  }
  for (uint32_t i = 0; i < 6; i++) {
    StoreU32(base, entry + 20 + 4 * i, words[4 + i]);
  }
  StoreU32(base, entry + 16, entry + 20);
  StoreU32(base, entry + 44, words[10]);
}

// StateMachine::Find (sub_82106380): the key's handler, or the default
// (empty) one, made on the first call.
uint32_t Find(uint8_t* base, uint32_t machine, uint32_t key) {
  const uint32_t made = LoadU32(base, kDefaultHandlerMade);
  if ((made & 1) == 0) {
    StoreU32(base, kDefaultHandlerMade, made | 1);
    StoreU32(base, kDefaultHandler + 4, kEmptyHandlerVtable);
    StoreU32(base, kDefaultHandler + 0, kDefaultHandler + 4);
    StoreU32(base, kDefaultHandler + 28, 0);
  }
  const int32_t index = IndexOf(base, machine, key);
  return index == -1 ? kDefaultHandler : machine + kEntryBytes * uint32_t(index) + 16;
}

// The overlays: fn(overlay) for each one set.
template <typename Fn>
void EachOverlay(uint8_t* base, uint32_t machine, Fn fn) {
  for (uint32_t i = 0; i < kOverlayCount; i++) {
    const uint32_t slot = machine + kOverlays + 4 * i;
    if (LoadU32(base, slot) != 0) {
      fn(slot);
    }
  }
}

bool On() { return REXCVAR_GET(native_game_modes); }

}  // namespace

// Modes::Modes: r3 the modes, r4 their owner.
extern "C" REX_FUNC(sub_82106980) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82106980(ctx, base);
    return;
  }
  const uint32_t modes = ctx.r3.u32, owner = ctx.r4.u32;
  StoreU32(base, modes + 0, kModesVtable);
  StoreU32(base, kModesOwner + 0, owner);
  StoreU32(base, kModesOwner + 4, owner);
  StoreU32(base, modes + 4, 0);
  StoreU32(base, kModesWord, 0);
  StoreU32(base, kModesWord2, 0);
  for (uint32_t i = 0; i < 7; i++) {
    StoreU32(base, kModesWords + 4 * i, 0);
  }
  StoreU32(base, kModesWord3, 0);
  StoreU32(base, kModesWord4, 0);
}

extern "C" REX_FUNC(sub_821061D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821061D0(ctx, base);
    return;
  }
  ctx.r3.u64 = KeyEquals(base, ctx.r3.u32, ctx.r4.u32) ? 1 : 0;
}

extern "C" REX_FUNC(sub_82106230) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82106230(ctx, base);
    return;
  }
  ctx.r3.u64 = uint64_t(int64_t(IndexOf(base, ctx.r3.u32, ctx.r4.u32)));
}

extern "C" REX_FUNC(sub_821062E8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821062E8(ctx, base);
    return;
  }
  Add(base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);
}

extern "C" REX_FUNC(sub_82106380) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82106380(ctx, base);
    return;
  }
  ctx.r3.u64 = Find(base, ctx.r3.u32, ctx.r4.u32);
}

// StateMachine::StateMachine: the 200 entries' keys cleared and their
// handlers empty.
extern "C" REX_FUNC(sub_82107230) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82107230(ctx, base);
    return;
  }
  const uint32_t machine = ctx.r3.u32;
  for (uint32_t i = 0; i < kEntries; i++) {
    const uint32_t entry = machine + kEntryBytes * i;
    const uint32_t object = entry + 20;
    for (uint32_t w = 0; w < 20; w += 4) {
      StoreU32(base, entry + w, 0);
    }
    uint32_t handler = 0;
    if (object != 0) {
      handler = object;
      StoreU32(base, object, kEmptyHandlerVtable);
    }
    StoreU32(base, entry + 44, 0);
    StoreU32(base, entry + 16, handler);
  }
}

// Mode::Mode.
extern "C" REX_FUNC(sub_82106D98) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82106D98(ctx, base);
    return;
  }
  const uint32_t mode = ctx.r3.u32;
  StoreU32(base, mode + 0, kModeBaseVtable);
  StoreU32(base, mode + 4, 0);
  uint32_t member = 0;
  if (mode + 8 != 0) {
    member = mode + 8;
    StoreU32(base, mode + 8, kModeMemberVtable);
  }
  StoreU32(base, mode + 4, member);
  StoreU32(base, mode + 32, kModeMember2Vtable);
  StoreU32(base, mode + 0, kModeVtable);
  StoreU32(base, mode + 32, kModeMember2Vtable);
}

// RaceOverlaysMachine::SetFlags: each overlay's +772 and +776 = 1.
extern "C" REX_FUNC(sub_8215BCB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8215BCB8(ctx, base);
    return;
  }
  EachOverlay(base, ctx.r3.u32, [&](uint32_t slot) {
    StoreU32(base, LoadU32(base, slot) + 772, 1);
    StoreU32(base, LoadU32(base, slot) + 776, 1);
  });
}

// RaceOverlaysMachine::ClearFlags: +772, +776 and +780 = 0.
extern "C" REX_FUNC(sub_8215BDA0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8215BDA0(ctx, base);
    return;
  }
  EachOverlay(base, ctx.r3.u32, [&](uint32_t slot) {
    StoreU32(base, LoadU32(base, slot) + 772, 0);
    StoreU32(base, LoadU32(base, slot) + 776, 0);
    StoreU32(base, LoadU32(base, slot) + 780, 0);
  });
}

// RaceOverlaysMachine::SetFlag820 (sub_8215BCF0): +772 = 1, +776 = 0.
extern "C" REX_FUNC(sub_8215BCF0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8215BCF0(ctx, base);
    return;
  }
  EachOverlay(base, ctx.r3.u32, [&](uint32_t slot) {
    StoreU32(base, LoadU32(base, slot) + 772, 1);
    StoreU32(base, LoadU32(base, slot) + 776, 0);
  });
}

// RaceOverlaysMachine::ResetOverlays: each overlay's vtable slot 3 (+12),
// then its +780 = 1.
extern "C" REX_FUNC(sub_8215BD30) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8215BD30(ctx, base);
    return;
  }
  EachOverlay(base, ctx.r3.u32, [&](uint32_t slot) {
    ctx.r3.u64 = LoadU32(base, slot);
    REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, ctx.r3.u32) + 12));
    StoreU32(base, LoadU32(base, slot) + 780, 1);
  });
}

// StateMachine::Handle: r3 the machine, r4 the message (three words). The
// key (the state at +9604, then the message's words) built in the
// original's 112-byte frame at +80, the transition found
// (StateMachine::Find, sub_82106380), and its next state (+28) taken when
// it has one. Returns the transition. Called from title_screen.cpp's
// override, which logs the modes and watches them for autosave.
void hooks::NativeGameModes_Handle(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_82106410(ctx, base);
    return;
  }
  const uint32_t machine = ctx.r3.u32, message = ctx.r4.u32;
  guest::StackFrame frame(ctx, 112);
  const uint32_t key = frame.Local(80);
  guest::StoreU32(base, key + 4, guest::LoadU32(base, message + 0));
  guest::StoreU32(base, key + 8, guest::LoadU32(base, message + 4));
  guest::StoreU32(base, key + 12, guest::LoadU32(base, message + 8));
  guest::StoreU32(base, key + 0, guest::LoadU32(base, machine + 9604));
  ctx.r3.u64 = machine;
  ctx.r4.u64 = key;
  sub_82106380(ctx, base);
  const uint32_t next = guest::LoadU32(base, ctx.r3.u32 + 28);
  if (next != 0) {
    guest::StoreU32(base, machine + 9604, next);
  }
}
