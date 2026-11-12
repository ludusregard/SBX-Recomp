// The game modes as our own C++ (--native_game_modes, on by default): the
// state machines every mode's messages go through (a key's handler found
// and added), the machines' and modes' constructors, the race mode's
// members' constructors and destructors, and the race overlays' flags
// (game/game_modes.h).
//
// StateMachine::Handle keeps title_screen.cpp's override (autosave and the
// boot mode log), which calls ours below in place of the recompiled
// function. The race mode's and the overlays machine's own set-ups stay
// recompiled.

#include "game/game_modes.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_game_modes, true, "SBX",
                    "Our own C++ for the game modes' state machines (a message's handler "
                    "found and added, the machines' and modes' constructors, the race "
                    "overlays' flags) instead of the recompiled functions");

namespace {

// A global Member13360 copies to +452 (31 in the game file), in its
// writable data.
constexpr uint32_t kSetting452 = 0x82608B14;

using game::Buffer28;
using game::GameObject;
using game::Handler;
using game::RaceModeMember;
using game::RaceOverlaysMachine;
using game::StateEntry;
using game::StateKey;
using game::StateMachine;
using game::Vector;
using guest::AddressOf;
using guest::At;
using guest::Guest;
using guest::u32_be;

// A handler that does nothing.
constexpr uint32_t kEmptyHandlerVtable = 0x82006954;
// StateMachine::Find's default handler (an empty one), made once.
constexpr uint32_t kDefaultHandler = 0x826B5848;
constexpr uint32_t kDefaultHandlerMade = 0x826B5868;  // Bit 0.

// Modes::Modes's globals.
constexpr uint32_t kModesVtable = 0x82006A88;
constexpr uint32_t kModesOwner = 0x825F3264;  // Twice.
constexpr uint32_t kModesWord = 0x826B586C;
constexpr uint32_t kModesWord2 = 0x826AA81C;
constexpr uint32_t kModesWords = 0x826AA820;  // Seven words.
constexpr uint32_t kModesWord3 = 0x826AA850;
constexpr uint32_t kModesWord4 = 0x826B5870;

// The classes' vtables.
constexpr uint32_t kModeVtable = 0x82006990;
constexpr uint32_t kModeMemberVtable = 0x82006AB8;
constexpr uint32_t kModeMember2Vtable = 0x82006984;
constexpr uint32_t kRaceModeMemberVtable = 0x82006AF8;
constexpr uint32_t kObject1104Vtable = 0x82006ACC;
constexpr uint32_t kObject1104Vtable2 = 0x82006AC4;
constexpr uint32_t kObject384Vtable = 0x82006ABC;
constexpr uint32_t kMember13360Vtable = 0x820092F0;
constexpr uint32_t kMember56896Vtable = 0x82006AD4;
constexpr uint32_t kMember67760Vtable = 0x820092F8;
constexpr uint32_t kBuffer28Vtable = 0x820092D0;

bool On() { return REXCVAR_GET(native_game_modes); }

// --- The state machines ---

int32_t IndexOf(StateMachine* machine, const StateKey& key) {
  for (uint32_t i = 0; i < machine->count; i++) {
    if (machine->entries[i].key == key) {
      return int32_t(i);
    }
  }
  return -1;
}

// A handler copied in: its object and next state; its target pointed at
// its own object.
void SetHandler(Handler& to, const Handler& from) {
  for (int i = 0; i < 6; i++) {
    to.object[i] = from.object[i];
  }
  to.target = AddressOf(to.object);
  to.next_state = from.next_state;
}

// StateMachine::Add: the key's handler set, a new entry for a new key.
void Add(StateMachine* machine, const StateKey& key, const Handler& handler) {
  const int32_t index = IndexOf(machine, key);
  if (index != -1) {
    SetHandler(machine->entries[index].handler, handler);
    return;
  }
  // Both are read before the entry's written (the game builds it on its
  // stack, then copies it in), so they may point into the table.
  const StateKey new_key = key;
  Handler new_handler;
  SetHandler(new_handler, handler);
  StateEntry& entry = machine->entries[machine->count];
  machine->count += 1;
  entry.key = new_key;
  SetHandler(entry.handler, new_handler);
}

// StateMachine::Find: the key's handler, or the default (empty) one, made
// on the first call.
Handler* Find(StateMachine* machine, const StateKey& key) {
  u32_be* made = At<u32_be>(kDefaultHandlerMade);
  Handler* fallback = At<Handler>(kDefaultHandler);
  if ((*made & 1) == 0) {
    *made |= 1;
    fallback->object[0] = kEmptyHandlerVtable;
    fallback->target = AddressOf(fallback->object);
    fallback->next_state = 0;
  }
  const int32_t index = IndexOf(machine, key);
  return index == -1 ? fallback : &machine->entries[index].handler;
}

// Every entry's key cleared and its handler an empty one.
void ConstructStateMachine(StateMachine* machine) {
  for (StateEntry& entry : machine->entries) {
    entry.key = StateKey{};
    entry.handler.object[0] = kEmptyHandlerVtable;
    entry.handler.next_state = 0;
    entry.handler.target = AddressOf(entry.handler.object);
  }
}

// --- The race mode's members ---

void ConstructObject(GameObject* object, uint32_t vtable) {
  object->unknown_16 = 0;
  object->unknown_20 = 0;
  for (Vector& point : object->points) {
    point.w = 1.0f;
  }
  for (Vector& point : object->points_272) {
    point.w = 1.0f;
  }
  object->vtable = vtable;
}

void ConstructObject1104(game::GameObject1104* object, uint32_t vtable) {
  ConstructObject(&object->object, vtable);
  object->unknown_1088.w = 1.0f;
}

void ConstructObject384(game::GameObject384* object) {
  ConstructObject(&object->object, kObject384Vtable);
  object->unknown_352.w = 1.0f;
}

// Buffer28::Free: its block given back.
void FreeBuffer(Guest& g, Buffer28* buffer) { g.Call(sub_821506B8, buffer); }

void ResetBuffer(Guest& g, Buffer28* buffer) {
  FreeBuffer(g, buffer);
  buffer->unknown_12 = -1;
  buffer->unknown_16 = -1;
  buffer->block = 0;
  buffer->unknown_24 = 0;
}

void ConstructMember13360(Guest& g, game::Member13360* member) {
  ConstructObject(&member->object, kMember13360Vtable);
  for (Vector& v : member->unknown_384) {
    v.w = 1.0f;
  }
  for (Buffer28& buffer : member->buffers) {
    buffer.vtable = kBuffer28Vtable;
    buffer.unknown_12 = -1;
    buffer.unknown_16 = -1;
    buffer.block = 0;
    buffer.unknown_24 = 0;
  }
  ConstructObject384(&member->unknown_43136);
  member->unknown_360 = -1;
  member->unknown_448 = 0;
  member->unknown_452 = *At<u32_be>(kSetting452);
  for (Buffer28& buffer : member->buffers) {
    ResetBuffer(g, &buffer);
  }
}

void ConstructXPRHolder(game::XPRHolder* holder) {
  holder->first = game::XPR{};
  holder->second = game::XPR{};
}

void ConstructMember56896(game::Member56896* member) {
  ConstructObject(&member->object, kMember56896Vtable);
  ConstructXPRHolder(&member->xprs);
}

void ConstructMember67760(game::Member67760* member) {
  ConstructObject(&member->object, kMember67760Vtable);
  member->list.Reset();
  member->unknown_748 = 0;
  member->unknown_752 = 0;
  member->unknown_756 = 2;
}

void ConstructRaceModeMember(Guest& g, RaceModeMember* member) {
  g.Call(sub_82107230, &member->machine);  // StateMachine::StateMachine
  member->machine.count = 0;
  member->machine.state = 0;
  member->vtable = kRaceModeMemberVtable;
  ConstructObject1104(&member->unknown_9648[0], kObject1104Vtable);
  ConstructObject1104(&member->unknown_9648[1], kObject1104Vtable);
  ConstructObject1104(&member->unknown_11856, kObject1104Vtable2);
  ConstructObject384(&member->unknown_12976);
  g.Call(sub_82153110, &member->unknown_13360);  // Member13360::Member13360
  g.Call(sub_82106BC8, &member->unknown_56880);  // Member56896::Member56896
  ConstructMember67760(&member->unknown_67744);
  for (game::Block1104& block : member->unknown_68512) {
    block.unknown_1088.w = 1.0f;
  }
  member->unknown_77360 = -1;
  for (u32_be& word : member->unknown_77364) {
    word = 0;
  }
}

// The overlays that are set.
template <typename Fn>
void EachOverlay(RaceOverlaysMachine* machine, Fn fn) {
  for (auto& overlay : machine->overlays) {
    if (overlay != nullptr) {
      fn(overlay.get());
    }
  }
}

}  // namespace

#define SBX_GAME_MODES_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  Guest g(ctx, base);

// Modes::Modes: r3 the modes, r4 their owner.
extern "C" REX_FUNC(sub_82106980) {
  SBX_GAME_MODES_FALLBACK(sub_82106980)
  game::Modes* modes = At<game::Modes>(ctx.r3.u32);
  const uint32_t owner = ctx.r4.u32;
  modes->vtable = kModesVtable;
  At<u32_be>(kModesOwner)[0] = owner;
  At<u32_be>(kModesOwner)[1] = owner;
  modes->unknown_4 = 0;
  *At<u32_be>(kModesWord) = 0;
  *At<u32_be>(kModesWord2) = 0;
  for (int i = 0; i < 7; i++) {
    At<u32_be>(kModesWords)[i] = 0;
  }
  *At<u32_be>(kModesWord3) = 0;
  *At<u32_be>(kModesWord4) = 0;
}

// StateKey::Equals: r3 and r4 the keys.
extern "C" REX_FUNC(sub_821061D0) {
  SBX_GAME_MODES_FALLBACK(sub_821061D0)
  ctx.r3.u64 = *At<StateKey>(ctx.r3.u32) == *At<StateKey>(ctx.r4.u32);
}

// StateMachine::IndexOf: r3 the machine, r4 the key; its entry or -1.
extern "C" REX_FUNC(sub_82106230) {
  SBX_GAME_MODES_FALLBACK(sub_82106230)
  ctx.r3.s64 = IndexOf(At<StateMachine>(ctx.r3.u32), *At<StateKey>(ctx.r4.u32));
}

// StateMachine::Add: r3 the machine, r4 the key, r5 the handler.
extern "C" REX_FUNC(sub_821062E8) {
  SBX_GAME_MODES_FALLBACK(sub_821062E8)
  Add(At<StateMachine>(ctx.r3.u32), *At<StateKey>(ctx.r4.u32), *At<Handler>(ctx.r5.u32));
}

// StateMachine::Find: r3 the machine, r4 the key; the handler.
extern "C" REX_FUNC(sub_82106380) {
  SBX_GAME_MODES_FALLBACK(sub_82106380)
  ctx.r3.u64 = AddressOf(Find(At<StateMachine>(ctx.r3.u32), *At<StateKey>(ctx.r4.u32)));
}

// StateMachine::StateMachine: r3 the machine.
extern "C" REX_FUNC(sub_82107230) {
  SBX_GAME_MODES_FALLBACK(sub_82107230)
  ConstructStateMachine(At<StateMachine>(ctx.r3.u32));
}

// Mode::Mode: r3 the mode.
extern "C" REX_FUNC(sub_82106D98) {
  SBX_GAME_MODES_FALLBACK(sub_82106D98)
  game::Mode* mode = At<game::Mode>(ctx.r3.u32);
  mode->member_vtable = kModeMemberVtable;
  mode->member = AddressOf(&mode->member_vtable);
  mode->member2_vtable = kModeMember2Vtable;
  mode->vtable = kModeVtable;
}

// RaceOverlaysMachine's flags: r3 the machine; each overlay's three flags.
extern "C" REX_FUNC(sub_8215BCB8) {  // RaceOverlaysMachine::SetFlags
  SBX_GAME_MODES_FALLBACK(sub_8215BCB8)
  EachOverlay(At<RaceOverlaysMachine>(ctx.r3.u32), [](game::RaceOverlay* overlay) {
    overlay->unknown_772 = 1;
    overlay->unknown_776 = 1;
  });
}

extern "C" REX_FUNC(sub_8215BDA0) {  // RaceOverlaysMachine::ClearFlags
  SBX_GAME_MODES_FALLBACK(sub_8215BDA0)
  EachOverlay(At<RaceOverlaysMachine>(ctx.r3.u32), [](game::RaceOverlay* overlay) {
    overlay->unknown_772 = 0;
    overlay->unknown_776 = 0;
    overlay->unknown_780 = 0;
  });
}

extern "C" REX_FUNC(sub_8215BCF0) {  // RaceOverlaysMachine::SetFlag820
  SBX_GAME_MODES_FALLBACK(sub_8215BCF0)
  EachOverlay(At<RaceOverlaysMachine>(ctx.r3.u32), [](game::RaceOverlay* overlay) {
    overlay->unknown_772 = 1;
    overlay->unknown_776 = 0;
  });
}

// RaceOverlaysMachine::ResetOverlays: each overlay's Reset (its virtual
// slot 3), then its third flag set.
extern "C" REX_FUNC(sub_8215BD30) {
  SBX_GAME_MODES_FALLBACK(sub_8215BD30)
  EachOverlay(At<RaceOverlaysMachine>(ctx.r3.u32), [&](game::RaceOverlay* overlay) {
    g.CallVirtual(overlay, 3);
    overlay->unknown_780 = 1;
  });
}

// StateMachine::Handle: r3 the machine, r4 the message (three words). The
// handler for the state and message found, and its next state taken when
// it has one. Returns the handler. Called from title_screen.cpp's override.
void hooks::NativeGameModes_Handle(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_82106410(ctx, base);
    return;
  }
  Guest g(ctx, base);
  StateMachine* machine = At<StateMachine>(ctx.r3.u32);
  const u32_be* message = At<u32_be>(ctx.r4.u32);
  // Find takes the key by address, so it's built on the guest stack.
  guest::StackFrame frame(ctx, 112);
  StateKey* key = At<StateKey>(frame.Local(80));
  key->words[1] = message[0];
  key->words[2] = message[1];
  key->words[3] = message[2];
  key->words[0] = machine->state;
  Handler* handler = At<Handler>(g.Call(sub_82106380, machine, key));  // StateMachine::Find
  if (handler->next_state != 0) {
    machine->state = handler->next_state;
  }
}

// XPRHolder::XPRHolder: r3 the holder. Returns it.
extern "C" REX_FUNC(sub_82106928) {
  SBX_GAME_MODES_FALLBACK(sub_82106928)
  ConstructXPRHolder(At<game::XPRHolder>(ctx.r3.u32));
}

// Member13360::~Member13360 (its buffers): r3 the member.
extern "C" REX_FUNC(sub_82106B78) {
  SBX_GAME_MODES_FALLBACK(sub_82106B78)
  game::Member13360* member = At<game::Member13360>(ctx.r3.u32);
  for (int i = 3; i >= 0; i--) {
    g.Call(sub_82150C90, &member->buffers[i]);  // Buffer28::~Buffer28
  }
}

// Member56896::Member56896: r3 the member. Returns it.
extern "C" REX_FUNC(sub_82106BC8) {
  SBX_GAME_MODES_FALLBACK(sub_82106BC8)
  game::Member56896* member = At<game::Member56896>(ctx.r3.u32);
  ConstructObject(&member->object, kMember56896Vtable);
  g.Call(sub_82106928, &member->xprs);  // XPRHolder::XPRHolder
  ctx.r3.u64 = AddressOf(member);
}

// RaceModeMember::~RaceModeMember: r3 the member.
extern "C" REX_FUNC(sub_82106C68) {
  SBX_GAME_MODES_FALLBACK(sub_82106C68)
  RaceModeMember* member = At<RaceModeMember>(ctx.r3.u32);
  member->vtable = kRaceModeMemberVtable;
  member->unknown_56880.xprs.second = game::XPR{};
  g.Call(sub_8257CA60, &member->unknown_56880.xprs.first);  // XPR::~XPR
  g.Call(sub_82106B78, &member->unknown_13360);             // Member13360::~Member13360
}

// RaceModeMember::RaceModeMember: r3 the member. Returns it.
extern "C" REX_FUNC(sub_82107290) {
  SBX_GAME_MODES_FALLBACK(sub_82107290)
  RaceModeMember* member = At<RaceModeMember>(ctx.r3.u32);
  ConstructRaceModeMember(g, member);
  ctx.r3.u64 = AddressOf(member);
}

// MachineMember::MachineMember: r3 a state machine, made and emptied.
// Returns it.
extern "C" REX_FUNC(sub_82108280) {
  SBX_GAME_MODES_FALLBACK(sub_82108280)
  StateMachine* machine = At<StateMachine>(ctx.r3.u32);
  g.Call(sub_82107230, machine);  // StateMachine::StateMachine
  machine->count = 0;
  machine->state = 0;
  ctx.r3.u64 = AddressOf(machine);
}

// Buffer28::~Buffer28 and Buffer28::Reset: r3 the buffer.
extern "C" REX_FUNC(sub_82150C90) {
  SBX_GAME_MODES_FALLBACK(sub_82150C90)
  Buffer28* buffer = At<Buffer28>(ctx.r3.u32);
  buffer->vtable = kBuffer28Vtable;
  FreeBuffer(g, buffer);
}

extern "C" REX_FUNC(sub_82150CA0) {
  SBX_GAME_MODES_FALLBACK(sub_82150CA0)
  ResetBuffer(g, At<Buffer28>(ctx.r3.u32));
}

// Member13360::Member13360: r3 the member. Returns it.
extern "C" REX_FUNC(sub_82153110) {
  SBX_GAME_MODES_FALLBACK(sub_82153110)
  game::Member13360* member = At<game::Member13360>(ctx.r3.u32);
  ConstructMember13360(g, member);
  ctx.r3.u64 = AddressOf(member);
}

// Member67760::Member67760: r3 the member. Returns it.
extern "C" REX_FUNC(sub_82153D78) {
  SBX_GAME_MODES_FALLBACK(sub_82153D78)
  ConstructMember67760(At<game::Member67760>(ctx.r3.u32));
}
