// The race overlays' objects: the sprites drawn over the race world (the
// marker above opponents, the light sprites, the model sprites), the tire
// smoke, the particles and the sparks, all built on one base.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"

namespace game {

using guest::f32_be;
using guest::i32_be;
using guest::u32_be;

// The base every overlay object starts with.
struct Overlay {
  static constexpr uint32_t kVtable = 0x82008568;

  u32_be vtable;
  uint8_t unknown_4[100];
  u32_be textures;  // Set by Overlay::SetTextures104.
  uint8_t unknown_108[20];
};
GUEST_SIZE(Overlay, 128);
GUEST_OFFSET(Overlay, textures, 104);

// A sprite's state, shared by the marker, the light sprites and the model
// sprites (at a different offset in each). A reset sets every point to
// (0, 0, 0, 1) and the flags to 0, 0, 0, 1.
struct SpriteState {
  Vector points[5];
  u32_be flags[4];
  u32_be unknown_96;
  f32_be unknown_100;
  f32_be unknown_104;
  u32_be unknown_108;
  u32_be unknown_112;
  u32_be unknown_116;
};
GUEST_SIZE(SpriteState, 120);
GUEST_OFFSET(SpriteState, flags, 80);

// The marker above an opponent's car.
struct Marker {
  static constexpr uint32_t kVtable = 0x820086EC;

  Overlay overlay;
  SpriteState state;
  uint8_t unknown_248[24];
  Vector camera_position;  // Set by Marker::SetCameraPosition.
  u32_be unknown_288;
};
GUEST_OFFSET(Marker, state, 128);
GUEST_OFFSET(Marker, camera_position, 272);
GUEST_OFFSET(Marker, unknown_288, 288);

struct LightSprite {
  static constexpr uint32_t kVtable = 0x820083D4;

  Overlay overlay;
  uint8_t unknown_128[16];
  SpriteState state;
  uint8_t unknown_264[8];
  Vector unknown_272;
};
GUEST_OFFSET(LightSprite, state, 144);
GUEST_OFFSET(LightSprite, unknown_272, 272);

struct ModelSprite {
  static constexpr uint32_t kVtable = 0x82008478;

  Overlay overlay;
  uint8_t unknown_128[96];
  SpriteState state;
  u32_be unknown_344;
  u32_be unknown_348;
  u32_be unknown_352;
  uint8_t unknown_356[12];
  Vector unknown_368[2];
  uint8_t unknown_400[4];
  i32_be unknown_404[2];
  uint8_t unknown_412[4];
  Vector unknown_416[10];
};
GUEST_OFFSET(ModelSprite, state, 224);
GUEST_OFFSET(ModelSprite, unknown_344, 344);
GUEST_OFFSET(ModelSprite, unknown_368, 368);
GUEST_OFFSET(ModelSprite, unknown_404, 404);
GUEST_OFFSET(ModelSprite, unknown_416, 416);

// A smoke or spark particle: two points and 16 bytes.
struct Particle {
  Vector points[2];
  uint8_t unknown_32[16];
};
GUEST_SIZE(Particle, 48);

// One wheel's smoke: ten points, then its particles.
struct SmokeEmitter {
  Vector points_0[5];
  uint8_t unknown_80[64];
  Vector points_144[5];
  uint8_t unknown_224[16];
  Particle particles[10];
};
GUEST_SIZE(SmokeEmitter, 720);
GUEST_OFFSET(SmokeEmitter, points_144, 144);
GUEST_OFFSET(SmokeEmitter, particles, 240);

// The tire smoke.
struct Smoke {
  static constexpr uint32_t kVtable = 0x8200868C;

  Overlay overlay;
  SmokeEmitter emitters[45];
  uint8_t unknown_32528[16];
  Vector velocity;  // Set by Smoke::SetVelocity.
};
GUEST_OFFSET(Smoke, emitters, 128);
GUEST_OFFSET(Smoke, velocity, 32544);

struct ParticleSlot {
  uint8_t unknown_0[128];
  Particle particle;
};
GUEST_SIZE(ParticleSlot, 176);

struct ParticlesB {
  static constexpr uint32_t kVtable = 0x820086B8;

  Overlay overlay;
  ParticleSlot slots[15];
  uint8_t unknown_2768[16];
  Vector position;  // Set by ParticlesB::SetPosition.
};
GUEST_OFFSET(ParticlesB, slots, 128);
GUEST_OFFSET(ParticlesB, position, 2784);

struct Spark {
  Vector points[5];
  uint8_t unknown_80[32];
  Vector point_112;
};
GUEST_SIZE(Spark, 128);

struct SparkSlot {
  Spark spark;
  uint8_t unknown_128[16];
};
GUEST_SIZE(SparkSlot, 144);

struct Sparks {
  static constexpr uint32_t kVtable = 0x82008724;

  Overlay overlay;
  SparkSlot sparks[200];
  uint8_t unknown_28928[16];
  Vector position;  // Set by Sparks::SetPosition.
  uint8_t unknown_28960[16];
  Vector unknown_28976[31];
};
GUEST_OFFSET(Sparks, sparks, 128);
GUEST_OFFSET(Sparks, position, 28944);
GUEST_OFFSET(Sparks, unknown_28976, 28976);

// The light trail behind a head or tail light.
struct Trail {
  uint8_t unknown_0[64144];
  Vector last_position;  // Set by Trail::SetLastPosition.
};
GUEST_OFFSET(Trail, last_position, 64144);

// Two words a car's overlay views are set from.
struct ViewDesc {
  u32_be slip;
  u32_be point;
  u32_be unknown_8;
};
GUEST_SIZE(ViewDesc, 12);

// What a car's overlays read of it: the wheel slip and a point (from a
// ViewDesc).
struct OverlayView {
  static constexpr uint32_t kVtable = 0x82000A40;

  u32_be vtable;
  ViewDesc desc;
};
GUEST_OFFSET(OverlayView, desc, 4);

// A car's overlay views.
struct CarOverlays {
  uint8_t unknown_0[4];
  u32_be views[25];
};
GUEST_OFFSET(CarOverlays, views, 4);

}  // namespace game
