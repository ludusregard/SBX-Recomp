// The sound layer's objects as our own C++ (--native_sound_cues, on by
// default): the bank holders', emitter's and listener's constructors, a
// sound's start and 3D flag, a bank pair's release, and the audio
// manager's listener setters.

#include "game/math.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_sound_cues, true, "SBX",
                    "Our own C++ for the sound layer's constructors and small setters "
                    "instead of the recompiled functions");

namespace {

using guest::AddressOf;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::i32_be;
using guest::ptr;
using guest::u32_be;

struct Vector3 {
  f32_be x, y, z;
};

// Where the sounds are heard from: which way it faces, where it is and
// how fast it's moving, then the distances sounds fade over.
struct Listener {
  Vector3 front;
  Vector3 top;
  Vector3 position;
  Vector3 velocity;
  f32_be near_distance;
  f32_be far_distance;
};
GUEST_SIZE(Listener, 56);

struct AudioManager {
  uint8_t unknown_0[12];
  uint8_t engine[1444];
  Listener listener;
};
GUEST_OFFSET(AudioManager, listener, 1456);

struct SoundBankHolder {
  u32_be unknown_0[3];
};

struct WaveBankHolder {
  u32_be unknown_0[3];
  i32_be unknown_12;
};

struct Sound {
  uint8_t unknown_0[8];
  uint8_t is_3d;
  uint8_t unknown_9[2];
  uint8_t started;
};

struct BankPair {
  uint8_t unknown_0[69];
  uint8_t released;
  uint8_t unknown_70[6];
  u32_be sound_bank;
};
GUEST_OFFSET(BankPair, sound_bank, 76);

// A sound's emitter, with its cone and distance curves kept inside it.
struct Emitter {
  // A curve's point: its index and its value.
  struct Point {
    uint8_t index;
    uint8_t pad_1[3];
    f32_be value;
  };
  // A curve: how many points, and where they are.
  struct Curve {
    uint8_t count;
    uint8_t pad_1[3];
    u32_be points;
  };

  u32_be unknown_0;
  uint8_t unknown_4[4];
  u32_be cone_address;  // `cone` below.
  f32_be unknown_12[2];
  f32_be unknown_20;
  f32_be unknown_24;
  f32_be unknown_28;
  f32_be unknown_32[7];
  u32_be channel_count;
  f32_be unknown_64;
  u32_be curve_address;  // `curve_values` below.
  u32_be unknown_72[5];
  f32_be unknown_92[2];
  f32_be cone[8];
  f32_be curve_values[6];
  f32_be unknown_156;
  uint8_t unknown_160[4];
  Point points_a[6];
  Curve curve_a;
  Point point_b;
  uint8_t unknown_228[40];
  Curve curve_b;
  // Three references, each a kind and a curve (or another reference).
  struct Reference {
    uint8_t kind;
    uint8_t pad_1[3];
    u32_be target;
  } references[3];
};
GUEST_OFFSET(Emitter, cone, 100);
GUEST_OFFSET(Emitter, curve_values, 132);
GUEST_OFFSET(Emitter, points_a, 164);
GUEST_OFFSET(Emitter, curve_a, 212);
GUEST_OFFSET(Emitter, point_b, 220);
GUEST_OFFSET(Emitter, curve_b, 268);
GUEST_OFFSET(Emitter, references, 276);

bool On() { return REXCVAR_GET(native_sound_cues); }

void ConstructEmitter(Emitter* emitter) {
  emitter->unknown_0 = 0;
  emitter->cone_address = AddressOf(emitter->cone);
  emitter->unknown_12[0] = 0.0f;
  emitter->unknown_12[1] = 0.0f;
  emitter->unknown_20 = 1.0f;
  emitter->unknown_24 = 0.0f;
  emitter->unknown_28 = 1.0f;
  for (f32_be& v : emitter->unknown_32) {
    v = 0.0f;
  }
  emitter->channel_count = 1;
  emitter->unknown_64 = 0.0f;
  emitter->curve_address = AddressOf(emitter->curve_values);
  for (u32_be& word : emitter->unknown_72) {
    word = 0;
  }
  emitter->unknown_92[0] = 1.0f;
  emitter->unknown_92[1] = 1.0f;
  static constexpr float kCone[8] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f};
  for (int i = 0; i < 8; i++) {
    emitter->cone[i] = kCone[i];
  }
  emitter->curve_values[0] = 0.0f;
  emitter->curve_values[5] = 1.0f;
  emitter->unknown_156 = 1.0f;

  // Curve A: six points of 1.0.
  for (int i = 0; i < 6; i++) {
    emitter->points_a[i].index = uint8_t(i);
    emitter->points_a[i].value = 1.0f;
  }
  emitter->curve_a.count = 6;
  emitter->curve_a.points = AddressOf(emitter->points_a);
  // Curve B: one point of 1.0.
  emitter->point_b.index = 0;
  emitter->point_b.value = 1.0f;
  emitter->curve_b.count = 1;
  emitter->curve_b.points = AddressOf(&emitter->point_b);
  emitter->references[0].kind = 0;
  emitter->references[0].target = AddressOf(&emitter->curve_a);
  emitter->references[1].kind = 1;
  emitter->references[1].target = AddressOf(&emitter->curve_b);
  emitter->references[2].kind = 1;
  emitter->references[2].target = AddressOf(&emitter->references[0]);
}

void ConstructListener(Listener* listener) {
  listener->front = {0.0f, 0.0f, 1.0f};
  listener->top = {0.0f, 1.0f, 0.0f};
  listener->position = {0.0f, 0.0f, 10.0f};
  listener->velocity = {0.0f, 0.0f, 0.0f};
  listener->near_distance = 100.0f;
  listener->far_distance = 200.0f;
}

}  // namespace

#define SBX_SOUND_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();             \
  if (!On()) {                     \
    __imp__##name(ctx, base);      \
    return;                        \
  }                                \
  Guest g(ctx, base);

// The constructors: r3 the object. Each returns it.
extern "C" REX_FUNC(sub_82105B48) {  // SoundBankHolder::SoundBankHolder
  SBX_SOUND_FALLBACK(sub_82105B48)
  for (u32_be& word : At<SoundBankHolder>(ctx.r3.u32)->unknown_0) {
    word = 0;
  }
}

extern "C" REX_FUNC(sub_82582B00) {  // WaveBankHolder::WaveBankHolder
  SBX_SOUND_FALLBACK(sub_82582B00)
  WaveBankHolder* holder = At<WaveBankHolder>(ctx.r3.u32);
  for (u32_be& word : holder->unknown_0) {
    word = 0;
  }
  holder->unknown_12 = -1;
}

extern "C" REX_FUNC(sub_825820C8) {  // Emitter::Emitter
  SBX_SOUND_FALLBACK(sub_825820C8)
  ConstructEmitter(At<Emitter>(ctx.r3.u32));
}

extern "C" REX_FUNC(sub_82582B60) {  // Listener::Listener
  SBX_SOUND_FALLBACK(sub_82582B60)
  ConstructListener(At<Listener>(ctx.r3.u32));
}

// Sound::Start: r3 the sound. Returns it.
extern "C" REX_FUNC(sub_82117558) {
  SBX_SOUND_FALLBACK(sub_82117558)
  At<Sound>(ctx.r3.u32)->started = 1;
}

// Sound::Set3D: r3 the sound, r4 the flag's address. Returns r3.
extern "C" REX_FUNC(sub_821176D0) {
  SBX_SOUND_FALLBACK(sub_821176D0)
  At<Sound>(ctx.r3.u32)->is_3d = *At<uint8_t>(ctx.r4.u32);
}

// BankPair::Release: r3 the pair, marked released. Returns it.
extern "C" REX_FUNC(sub_821178C8) {
  SBX_SOUND_FALLBACK(sub_821178C8)
  At<BankPair>(ctx.r3.u32)->released = 1;
}

// BankPair::SoundBank: r3 the pair.
extern "C" REX_FUNC(sub_821178D8) {
  SBX_SOUND_FALLBACK(sub_821178D8)
  ctx.r3.u64 = At<BankPair>(ctx.r3.u32)->sound_bank;
}

// AudioManager::GetEngine: r3 the manager.
extern "C" REX_FUNC(sub_82118010) {
  SBX_SOUND_FALLBACK(sub_82118010)
  ctx.r3.u64 = AddressOf(At<AudioManager>(ctx.r3.u32)->engine);
}

// The listener's setters: r3 the manager, r4 (and r5) the vectors. Each
// returns the listener.
extern "C" REX_FUNC(sub_82117F70) {  // AudioManager::SetListenerPosition
  SBX_SOUND_FALLBACK(sub_82117F70)
  Listener* listener = &At<AudioManager>(ctx.r3.u32)->listener;
  listener->position = *At<Vector3>(ctx.r4.u32);
  ctx.r3.u64 = AddressOf(listener);
}

extern "C" REX_FUNC(sub_82117F78) {  // AudioManager::SetListenerOrientation
  SBX_SOUND_FALLBACK(sub_82117F78)
  Listener* listener = &At<AudioManager>(ctx.r3.u32)->listener;
  listener->front = *At<Vector3>(ctx.r4.u32);
  listener->top = *At<Vector3>(ctx.r5.u32);
  ctx.r3.u64 = AddressOf(listener);
}

extern "C" REX_FUNC(sub_82117F80) {  // AudioManager::SetListenerVelocity
  SBX_SOUND_FALLBACK(sub_82117F80)
  Listener* listener = &At<AudioManager>(ctx.r3.u32)->listener;
  listener->velocity = *At<Vector3>(ctx.r4.u32);
  ctx.r3.u64 = AddressOf(listener);
}
