// The XACT library's objects as our own C++ (--native_xact, on by
// default): the variables', settings', sound banks', wave banks', faders',
// waves' and tracks' constructors (most hold a node pool and event nodes),
// a cue's sound detached, the listeners added, and a few getters.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_xact, true, "SBX",
                    "Our own C++ for the XACT library's constructors, destructors and small "
                    "setters and getters instead of the recompiled functions");

namespace {

using guest::AddressOf;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::i16_be;
using guest::u16_be;
using guest::u32_be;

// An event node: a vtable and a ring it's linked into (itself while
// alone).
struct EventNode {
  u32_be vtable;
  u32_be next;
  u32_be previous;

  void Init(uint32_t class_vtable) {
    vtable = class_vtable;
    next = AddressOf(this);
    previous = AddressOf(this);
  }
};
GUEST_SIZE(EventNode, 12);

// 16 nodes on a free list, and an empty list of the used ones (its tail
// points at its head).
struct NodePool {
  struct Node {
    u32_be value;
    u32_be next;
    u32_be unknown_8;
  };

  u32_be free_head;
  u32_be used_tail;
  u32_be used_head;
  Node nodes[16];

  void Init() {
    used_tail = AddressOf(&used_head);
    used_head = 0;
    free_head = 0;
    for (Node& node : nodes) {
      node.next = free_head;
      free_head = AddressOf(&node);
    }
  }
};
GUEST_SIZE(NodePool, 204);

constexpr uint32_t kEventVtable = 0x82072188;  // XACTEvent's.

struct XACTVariable {
  EventNode node;
  u32_be unknown_12[2];
  f32_be value;
  NodePool pool;
  u32_be unknown_228[4];
  i16_be unknown_244;
  uint8_t pad_246[2];
  u32_be unknown_248;
};
GUEST_OFFSET(XACTVariable, pool, 24);
GUEST_OFFSET(XACTVariable, unknown_248, 248);

struct XACTGlobalSettings {
  u32_be vtable;
  u32_be owner;
  u32_be unknown_8;
  uint8_t unknown_12[88];
  u16_be unknown_100;
  uint8_t pad_102[2];
  u32_be unknown_104[7];
  u32_be unknown_132[4];  // The derived class's.
};
GUEST_OFFSET(XACTGlobalSettings, unknown_104, 104);

// A node pool on its own, with three words after it.
struct XACTNodePool {
  NodePool pool;
  u32_be unknown_204[3];
};

struct XACTCue {
  uint8_t unknown_0[40];
  u32_be unknown_40;
  uint8_t unknown_44[8];
  u32_be sound;
};
GUEST_OFFSET(XACTCue, sound, 52);

struct XACTSoundBank {
  u32_be vtables[3];
  EventNode listener;  // Its first word ends as the fourth vtable.
  u32_be engine;
  NodePool pool;
  u32_be unknown_232[5];
  u32_be unknown_252[4];  // XACTSoundBank's own.
};
GUEST_OFFSET(XACTSoundBank, pool, 28);
GUEST_OFFSET(XACTSoundBank, unknown_252, 252);

struct XACTWaveBank {
  u32_be vtables[2];
  EventNode listener;  // Its first word ends as the third vtable.
  u32_be engine;
  NodePool pool;
  u32_be unknown_228[3];
  EventNode event;
  u32_be unknown_252[2];
};
GUEST_OFFSET(XACTWaveBank, pool, 24);
GUEST_OFFSET(XACTWaveBank, event, 240);

struct XACTFader {
  u32_be vtable;
  f32_be unknown_4;
  u16_be unknown_8;
  uint8_t pad_10[2];
  u32_be unknown_12[2];
  f32_be unknown_20;
};

struct XACTInMemoryWave {
  u32_be vtable;
  uint8_t unknown_4[8];
  u32_be event_vtable;
  uint8_t unknown_16[48];
  u32_be unknown_64[8];  // Two pointers back at the wave among them.
};
GUEST_OFFSET(XACTInMemoryWave, unknown_64, 64);

struct XACTTrackInstance {
  EventNode node;
  u32_be track;
  u32_be owner;
  u32_be unknown_20;
  uint8_t unknown_24;
  uint8_t pad_25[3];
  u32_be unknown_28;
  uint8_t unknown_32;
  uint8_t pad_33[3];
  u32_be unknown_36;
  u32_be unknown_40;
  uint8_t unknown_44[2];
  uint8_t pad_46[2];
  EventNode event;
  u32_be unknown_60;
};
GUEST_OFFSET(XACTTrackInstance, event, 48);

struct XACTWaveEntry {
  u32_be unknown_0;
  uint8_t unknown_4[20];
  u32_be play_length;
  u32_be loop_start;
  u32_be loop_length;
};

bool On() { return REXCVAR_GET(native_xact); }

void ConstructSettingsBase(XACTGlobalSettings* settings, uint32_t owner) {
  settings->owner = owner;
  settings->vtable = 0x82072658;
  settings->unknown_8 = 0;
  settings->unknown_100 = 0;
  for (u32_be& word : settings->unknown_104) {
    word = 0;
  }
}

void ConstructWaveBankRoot(XACTWaveBank* bank, uint32_t engine) {
  bank->listener.Init(kEventVtable);
  bank->vtables[0] = 0x82072E98;
  bank->vtables[1] = 0x82072930;
  bank->listener.vtable = 0x82072E94;
  bank->engine = engine;
  bank->pool.Init();
  for (u32_be& word : bank->unknown_228) {
    word = 0;
  }
  bank->event.Init(kEventVtable);
  bank->unknown_252[0] = 0;
  bank->unknown_252[1] = 0;
}

}  // namespace

#define SBX_XACT_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();            \
  if (!On()) {                    \
    __imp__##name(ctx, base);     \
    return;                       \
  }                               \
  Guest g(ctx, base);

// The constructors: r3 the object (r4 its owner where it has one). Each
// returns r3.

extern "C" REX_FUNC(sub_82587918) {  // XACTWaveEntry::XACTWaveEntry
  SBX_XACT_FALLBACK(sub_82587918)
  At<XACTWaveEntry>(ctx.r3.u32)->unknown_0 = 0;
}

extern "C" REX_FUNC(sub_825A74F0) {  // XACTVariable::XACTVariable
  SBX_XACT_FALLBACK(sub_825A74F0)
  XACTVariable* variable = At<XACTVariable>(ctx.r3.u32);
  variable->value = 0.0f;
  variable->node.Init(0x820721E4);
  variable->unknown_12[0] = 0;
  variable->unknown_12[1] = 0;
  variable->pool.Init();
  for (u32_be& word : variable->unknown_228) {
    word = 0;
  }
  variable->unknown_248 = 0;
  variable->unknown_244 = -1;
}

extern "C" REX_FUNC(sub_825A88C0) {  // XACTGlobalSettingsBase::XACTGlobalSettingsBase
  SBX_XACT_FALLBACK(sub_825A88C0)
  ConstructSettingsBase(At<XACTGlobalSettings>(ctx.r3.u32), ctx.r4.u32);
}

extern "C" REX_FUNC(sub_825A75F0) {  // XACTGlobalSettings::XACTGlobalSettings
  SBX_XACT_FALLBACK(sub_825A75F0)
  XACTGlobalSettings* settings = At<XACTGlobalSettings>(ctx.r3.u32);
  ConstructSettingsBase(settings, ctx.r4.u32);
  settings->vtable = 0x820721E8;
  for (u32_be& word : settings->unknown_132) {
    word = 0;
  }
}

extern "C" REX_FUNC(sub_825A83A8) {  // XACTNodePool::XACTNodePool
  SBX_XACT_FALLBACK(sub_825A83A8)
  XACTNodePool* pool = At<XACTNodePool>(ctx.r3.u32);
  pool->pool.Init();
  for (u32_be& word : pool->unknown_204) {
    word = 0;
  }
}

extern "C" REX_FUNC(sub_825AC9E8) {  // XACTSoundBankBase::XACTSoundBankBase
  SBX_XACT_FALLBACK(sub_825AC9E8)
  XACTSoundBank* bank = At<XACTSoundBank>(ctx.r3.u32);
  bank->listener.Init(kEventVtable);
  bank->vtables[0] = 0x82072870;
  bank->vtables[1] = 0x820728CC;
  bank->vtables[2] = 0x8207286C;
  bank->listener.vtable = 0x820728C4;
  bank->engine = ctx.r4.u32;
  bank->pool.Init();
  for (u32_be& word : bank->unknown_232) {
    word = 0;
  }
}

extern "C" REX_FUNC(sub_825ACD00) {  // XACTSoundBank::XACTSoundBank
  SBX_XACT_FALLBACK(sub_825ACD00)
  XACTSoundBank* bank = At<XACTSoundBank>(ctx.r3.u32);
  g.Call(sub_825AC9E8, bank, ctx.r4.u32);  // XACTSoundBankBase::XACTSoundBankBase
  bank->vtables[0] = 0x820728D8;
  bank->vtables[1] = 0x820728CC;
  bank->vtables[2] = 0x8207286C;
  bank->listener.vtable = 0x820728C8;
  for (u32_be& word : bank->unknown_252) {
    word = 0;
  }
  ctx.r3.u64 = AddressOf(bank);
}

extern "C" REX_FUNC(sub_825B16D8) {  // XACTFader::XACTFader
  SBX_XACT_FALLBACK(sub_825B16D8)
  XACTFader* fader = At<XACTFader>(ctx.r3.u32);
  fader->vtable = 0x82072E44;
  fader->unknown_4 = 1.0f;
  fader->unknown_20 = 1.0f;
  fader->unknown_8 = 0;
  fader->unknown_12[0] = 0;
  fader->unknown_12[1] = 0;
}

extern "C" REX_FUNC(sub_825B2478) {  // XACTWaveBankRoot::XACTWaveBankRoot
  SBX_XACT_FALLBACK(sub_825B2478)
  ConstructWaveBankRoot(At<XACTWaveBank>(ctx.r3.u32), ctx.r4.u32);
}

extern "C" REX_FUNC(sub_825B4318) {  // XACTWaveBankBase::XACTWaveBankBase
  SBX_XACT_FALLBACK(sub_825B4318)
  XACTWaveBank* bank = At<XACTWaveBank>(ctx.r3.u32);
  g.Call(sub_825B2478, bank, ctx.r4.u32);  // XACTWaveBankRoot::XACTWaveBankRoot
  bank->vtables[0] = 0x82072F78;
  bank->vtables[1] = 0x82072930;
  bank->listener.vtable = 0x82072F74;
  ctx.r3.u64 = AddressOf(bank);
}

extern "C" REX_FUNC(sub_825B8E70) {  // XACTInMemoryWave::XACTInMemoryWave
  SBX_XACT_FALLBACK(sub_825B8E70)
  XACTInMemoryWave* wave = At<XACTInMemoryWave>(ctx.r3.u32);
  g.Call(sub_825B7A80, wave);  // XACTWave::XACTWave
  wave->vtable = 0x82073898;
  wave->event_vtable = 0x820738C8;
  for (u32_be& word : wave->unknown_64) {
    word = 0;
  }
  wave->unknown_64[0] = AddressOf(wave);
  wave->unknown_64[4] = AddressOf(wave);
  ctx.r3.u64 = AddressOf(wave);
}

// XACTTrackInstance::XACTTrackInstance: r4 the track, r5 its owner.
extern "C" REX_FUNC(sub_825BAC80) {
  SBX_XACT_FALLBACK(sub_825BAC80)
  XACTTrackInstance* instance = At<XACTTrackInstance>(ctx.r3.u32);
  instance->node.Init(0x820738FC);
  instance->track = ctx.r4.u32;
  instance->owner = ctx.r5.u32;
  instance->unknown_20 = 0;
  instance->unknown_24 = 0;
  instance->unknown_28 = 0;
  instance->unknown_36 = 0;
  instance->unknown_40 = 1;
  instance->unknown_44[0] = 0;
  instance->unknown_44[1] = 0;
  instance->unknown_32 = 0;
  instance->event.Init(kEventVtable);
  instance->unknown_60 = 0;
}

// The destructors: r3 the object, its vtable stepped down. Return r3.
extern "C" REX_FUNC(sub_825B1708) {  // XACTFader::~XACTFader
  SBX_XACT_FALLBACK(sub_825B1708)
  At<XACTFader>(ctx.r3.u32)->vtable = 0x82072E44;
}

extern "C" REX_FUNC(sub_825B5E98) {  // XACTEvent::~XACTEvent
  SBX_XACT_FALLBACK(sub_825B5E98)
  At<EventNode>(ctx.r3.u32)->vtable = kEventVtable;
}

// XACTCue::DetachSound: r3 the cue. Returns it.
extern "C" REX_FUNC(sub_825AA498) {
  SBX_XACT_FALLBACK(sub_825AA498)
  XACTCue* cue = At<XACTCue>(ctx.r3.u32);
  cue->sound = 0;
  cue->unknown_40 = 0;
}

// XACTCue::MakeInstanceVariable: r3 the cue; its variables made
// (recompiled). Returns 0 (S_OK).
extern "C" REX_FUNC(sub_825AB208) {
  SBX_XACT_FALLBACK(sub_825AB208)
  g.Call(sub_825AB0F8, ctx.r3.u32, 0u, 0u, 1u);
  ctx.r3.u64 = 0;
}

// XACTSoundBank::AddListener and XACTWaveBank::AddListener: r3 the bank,
// r4 the listener, onto the bank's listener node (recompiled).
extern "C" REX_FUNC(sub_825ACBA0) {
  SBX_XACT_FALLBACK(sub_825ACBA0)
  g.Call(sub_825AEA28, &At<XACTSoundBank>(ctx.r3.u32)->engine);
}

extern "C" REX_FUNC(sub_825AD9B8) {
  SBX_XACT_FALLBACK(sub_825AD9B8)
  g.Call(sub_825AEA28, &At<XACTWaveBank>(ctx.r3.u32)->engine);
}

// Getters: r3 the object.
extern "C" REX_FUNC(sub_8209A2C8) {  // XACTWaveEntry::GetPlayLength
  SBX_XACT_FALLBACK(sub_8209A2C8)
  ctx.r3.u64 = At<XACTWaveEntry>(ctx.r3.u32)->play_length;
}

extern "C" REX_FUNC(sub_820A88D0) {  // XACTWaveEntry::GetLoopStart
  SBX_XACT_FALLBACK(sub_820A88D0)
  ctx.r3.u64 = At<XACTWaveEntry>(ctx.r3.u32)->loop_start;
}

extern "C" REX_FUNC(sub_82370758) {  // XACTWaveEntry::GetLoopLength
  SBX_XACT_FALLBACK(sub_82370758)
  ctx.r3.u64 = At<XACTWaveEntry>(ctx.r3.u32)->loop_length;
}

extern "C" REX_FUNC(sub_8242B820) {  // MenuList::GetPreviousSelection (shared): +20
  SBX_XACT_FALLBACK(sub_8242B820)
  ctx.r3.u64 = At<u32_be>(ctx.r3.u32)[5];
}

extern "C" REX_FUNC(sub_825BD8F8) {  // XACTPlayWaveEvent::GetLoopCount
  SBX_XACT_FALLBACK(sub_825BD8F8)
  ctx.r3.u64 = At<uint8_t>(ctx.r3.u32)[52];
}
