// The cars' sounds as our own C++ (--native_car_sounds, on by default): the
// engine sound's and car sound's constructors, an engine layer's start, and
// the car sound's channels stopped, released, paused and resumed. The
// sounds' updates stay recompiled.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_car_sounds, true, "SBX",
                    "Our own C++ for the car sounds' set-up and their channels' stop, release, "
                    "pause and resume instead of the recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kChannelVtable = 0x82000A40;
constexpr uint32_t kEngineLayerVtable = 0x82007FB0;

// One playing sound of a car's.
struct SoundChannel {
  u32_be vtable;
  u32_be unknown_4[9];
};
GUEST_SIZE(SoundChannel, 40);

// An engine sound's layer: its own words, then its channel.
struct EngineLayer {
  u32_be vtable;
  u32_be unknown_4[4];
  uint8_t unknown_20[24];
  SoundChannel channel;
};
GUEST_SIZE(EngineLayer, 84);

struct EngineSound {
  EngineLayer layers[2];
};

struct CarSound {
  uint8_t unknown_0[176];
  u32_be buffer;  // Freed on release.
  uint8_t unknown_180[228];
  EngineSound engine;
  uint8_t unknown_576[8];
  SoundChannel channel_584;
  uint8_t unknown_624[52];
  SoundChannel channel_676;
  uint8_t unknown_716[20];
  SoundChannel channel_736;
  uint8_t unknown_776[16];
  SoundChannel channel_792;
  uint8_t unknown_832[8];
  SoundChannel channel_840;
  uint8_t unknown_880[8];
  SoundChannel channel_888;
  uint8_t unknown_928[24];
  SoundChannel channel_952;
  uint8_t unknown_992[24];
  SoundChannel channel_1016;
  uint8_t unknown_1056[16];
  SoundChannel channel_1072;
  uint8_t unknown_1112[24];
  SoundChannel channel_1136;
  uint8_t unknown_1176[72];
  u32_be playing;
};
GUEST_OFFSET(CarSound, engine, 408);
GUEST_OFFSET(CarSound, channel_584, 584);
GUEST_OFFSET(CarSound, channel_1136, 1136);
GUEST_OFFSET(CarSound, playing, 1248);

bool On() { return REXCVAR_GET(native_car_sounds); }

void ConstructChannel(SoundChannel* channel) {
  channel->vtable = kChannelVtable;
  for (u32_be& word : channel->unknown_4) {
    word = 0;
  }
}

void ConstructEngineSound(EngineSound* sound) {
  for (EngineLayer& layer : sound->layers) {
    layer.vtable = kEngineLayerVtable;
    ConstructChannel(&layer.channel);
  }
}

// The car's channels, in the order the game stops and releases them, and
// the order it pauses and resumes them.
template <typename Fn>
void EachChannelForStop(CarSound* sound, Fn fn) {
  for (SoundChannel* channel :
       {&sound->engine.layers[0].channel, &sound->engine.layers[1].channel,
        &sound->channel_584, &sound->channel_676, &sound->channel_736, &sound->channel_840,
        &sound->channel_792, &sound->channel_888, &sound->channel_952, &sound->channel_1016,
        &sound->channel_1072, &sound->channel_1136}) {
    fn(channel);
  }
}

template <typename Fn>
void EachChannelForPause(CarSound* sound, Fn fn) {
  for (SoundChannel* channel :
       {&sound->engine.layers[0].channel, &sound->engine.layers[1].channel,
        &sound->channel_952, &sound->channel_584, &sound->channel_676, &sound->channel_736,
        &sound->channel_840, &sound->channel_792, &sound->channel_888, &sound->channel_1016,
        &sound->channel_1072, &sound->channel_1136}) {
    fn(channel);
  }
}

}  // namespace

#define SBX_CAR_SOUNDS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                  \
  if (!On()) {                          \
    __imp__##name(ctx, base);           \
    return;                             \
  }                                     \
  Guest g(ctx, base);

// EngineSound::EngineSound: r3 the sound. Returns it.
extern "C" REX_FUNC(sub_8210CF40) {
  SBX_CAR_SOUNDS_FALLBACK(sub_8210CF40)
  ConstructEngineSound(At<EngineSound>(ctx.r3.u32));
}

// CarSound::CarSound: r3 the sound. Returns it.
extern "C" REX_FUNC(sub_8210CFB8) {
  SBX_CAR_SOUNDS_FALLBACK(sub_8210CFB8)
  CarSound* sound = At<CarSound>(ctx.r3.u32);
  ConstructEngineSound(&sound->engine);
  for (SoundChannel* channel :
       {&sound->channel_584, &sound->channel_676, &sound->channel_736, &sound->channel_792,
        &sound->channel_840, &sound->channel_888, &sound->channel_952, &sound->channel_1016,
        &sound->channel_1072, &sound->channel_1136}) {
    ConstructChannel(channel);
  }
}

// EngineLayer::Start: r3 the layer, r5 passed on. Its words cleared, then
// its channel's four parts started (recompiled).
extern "C" REX_FUNC(sub_8210D760) {
  SBX_CAR_SOUNDS_FALLBACK(sub_8210D760)
  EngineLayer* layer = At<EngineLayer>(ctx.r3.u32);
  const uint32_t r5 = ctx.r5.u32;
  for (u32_be& word : layer->unknown_4) {
    word = 0;
  }
  for (uint8_t& byte : layer->unknown_20) {
    byte = 0;
  }
  for (uint32_t part = 0; part < 4; part++) {
    g.Call(sub_8210D1D8, &layer->channel, r5, part, 1u);
  }
}

// CarSound::Release: r3 the sound; every channel stopped and its buffer
// freed.
extern "C" REX_FUNC(sub_8210E260) {
  SBX_CAR_SOUNDS_FALLBACK(sub_8210E260)
  CarSound* sound = At<CarSound>(ctx.r3.u32);
  EachChannelForStop(sound, [&](SoundChannel* channel) {
    g.Call(sub_8210D670, channel);  // SoundChannel::Stop
  });
  g.Call(sub_8258C940, sound->buffer);  // free
  sound->playing = 0;
}

// CarSound::Stop: r3 the sound; every channel stopped.
extern "C" REX_FUNC(sub_8210E308) {
  SBX_CAR_SOUNDS_FALLBACK(sub_8210E308)
  CarSound* sound = At<CarSound>(ctx.r3.u32);
  EachChannelForStop(sound, [&](SoundChannel* channel) {
    g.Call(sub_8210D670, channel);  // SoundChannel::Stop
  });
  sound->playing = 0;
}

// CarSound::PauseAll and ResumeAll: r3 the sound.
extern "C" REX_FUNC(sub_8210E3A8) {
  SBX_CAR_SOUNDS_FALLBACK(sub_8210E3A8)
  EachChannelForPause(At<CarSound>(ctx.r3.u32), [&](SoundChannel* channel) {
    g.Call(sub_8210D590, channel);  // SoundChannel::Pause
  });
}

extern "C" REX_FUNC(sub_8210E440) {
  SBX_CAR_SOUNDS_FALLBACK(sub_8210E440)
  EachChannelForPause(At<CarSound>(ctx.r3.u32), [&](SoundChannel* channel) {
    g.Call(sub_8210D610, channel);  // SoundChannel::Resume
  });
}
