// The race overlays' objects as our own C++ (--native_race_overlays, on by
// default): the overlays' constructors, resets and destructors, the small
// setters, and the overlay views' getters. The overlays' updates and
// recordings stay recompiled (src/subtick/per_frame_logic.cpp wraps them for
// --subtick).

#include "game/game_modes.h"
#include "game/race_overlays.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_overlays, true, "SBX",
                    "Our own C++ for the race overlays' set-ups, resets, small setters and getters "
                    "instead of the recompiled functions");

namespace {

using game::CarOverlays;
using game::LightSprite;
using game::Marker;
using game::ModelSprite;
using game::Overlay;
using game::OverlayView;
using game::Particle;
using game::ParticlesB;
using game::SmokeEmitter;
using game::Smoke;
using game::Sparks;
using game::SpriteState;
using game::Trail;
using game::Vector;
using game::ViewDesc;
using guest::At;
using guest::Guest;
using guest::ptr;

bool On() { return REXCVAR_GET(native_race_overlays); }

// The particles' destructor: an empty one.
constexpr uint32_t kParticleDestructor = 0x821611F0;

void SetW(Vector& v) { v.w = 1.0f; }

void SetOrigin(Vector& v) {
  v.x = 0.0f;
  v.y = 0.0f;
  v.z = 0.0f;
  v.w = 1.0f;
}

// What every sprite's reset does to its state.
void ResetState(SpriteState& state) {
  for (Vector& point : state.points) {
    SetOrigin(point);
  }
  state.flags[0] = 0;
  state.flags[1] = 0;
  state.flags[2] = 0;
  state.flags[3] = 1;
  state.unknown_96 = 0;
  state.unknown_100 = 0.0f;
  state.unknown_104 = 0.0f;
  state.unknown_108 = 0;
  state.unknown_112 = 0;
}

// Overlay::Overlay (sub_82126068, ours below): the base's vtable. The
// destructors call it too, to put the base's vtable back.
void ConstructOverlay(Guest& g, void* overlay) { g.Call(sub_82126068, overlay); }

// The C runtime's array destructor: `destructor` on each element, last first.
template <typename T, size_t N>
void DestroyArray(Guest& g, T (&array)[N], uint32_t destructor) {
  g.Call(sub_820900C0, array, uint32_t(sizeof(T)), uint32_t(N), destructor);
}

}  // namespace

#define SBX_RACE_OVERLAYS_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                   \
  if (!On()) {                           \
    __imp__##name(ctx, base);            \
    return;                              \
  }                                      \
  Guest g(ctx, base);

// Overlay::Overlay: r3 the overlay.
extern "C" REX_FUNC(sub_82126068) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82126068)
  At<Overlay>(ctx.r3.u32)->vtable = Overlay::kVtable;
}

// Overlay::SetTextures104: r3 the overlay, r4 the textures.
extern "C" REX_FUNC(sub_82123480) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82123480)
  At<Overlay>(ctx.r3.u32)->textures = ctx.r4.u32;
}

// LightSprite::LightSprite: r3 the sprite. Returns it.
extern "C" REX_FUNC(sub_8211F220) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8211F220)
  LightSprite* sprite = At<LightSprite>(ctx.r3.u32);
  ConstructOverlay(g, sprite);
  sprite->overlay.vtable = LightSprite::kVtable;
  for (Vector& point : sprite->state.points) {
    SetW(point);
  }
  SetW(sprite->unknown_272);
  ctx.r3.u64 = guest::AddressOf(sprite);
}

// LightSprite::Reset: r3 the sprite.
extern "C" REX_FUNC(sub_8211EBB0) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8211EBB0)
  LightSprite* sprite = At<LightSprite>(ctx.r3.u32);
  SetOrigin(sprite->unknown_272);
  ResetState(sprite->state);
  sprite->state.unknown_116 = 4;
}

// ModelSprite::ModelSprite: r3 the sprite. Returns it.
extern "C" REX_FUNC(sub_82122A40) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82122A40)
  ModelSprite* sprite = At<ModelSprite>(ctx.r3.u32);
  ConstructOverlay(g, sprite);
  sprite->overlay.vtable = ModelSprite::kVtable;
  for (Vector& point : sprite->state.points) {
    SetW(point);
  }
  for (Vector& point : sprite->unknown_368) {
    SetW(point);
  }
  for (Vector& point : sprite->unknown_416) {
    SetW(point);
  }
  ctx.r3.u64 = guest::AddressOf(sprite);
}

// ModelSprite::Reset: r3 the sprite.
extern "C" REX_FUNC(sub_8211F5F0) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8211F5F0)
  ModelSprite* sprite = At<ModelSprite>(ctx.r3.u32);
  ResetState(sprite->state);
  sprite->state.unknown_112 = 9;
  sprite->state.unknown_116 = 4;
  sprite->unknown_344 = 9;
  sprite->unknown_348 = 12;
  sprite->unknown_352 = 0;
  for (Vector& point : sprite->unknown_368) {
    SetOrigin(point);
  }
  sprite->unknown_404[0] = -1;
  sprite->unknown_404[1] = -1;
  for (Vector& point : sprite->unknown_416) {
    SetOrigin(point);
  }
}

// Marker::Marker: r3 the marker. Returns it.
extern "C" REX_FUNC(sub_8212C580) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212C580)
  Marker* marker = At<Marker>(ctx.r3.u32);
  ConstructOverlay(g, marker);
  marker->overlay.vtable = Marker::kVtable;
  for (Vector& point : marker->state.points) {
    SetW(point);
  }
  SetW(marker->camera_position);
  ctx.r3.u64 = guest::AddressOf(marker);
}

// Marker::Reset: r3 the marker.
extern "C" REX_FUNC(sub_8212BB98) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212BB98)
  Marker* marker = At<Marker>(ctx.r3.u32);
  SetOrigin(marker->camera_position);
  marker->unknown_288 = 1;
  ResetState(marker->state);
}

// Marker::SetCameraPosition: r3 the marker, r4 the position.
extern "C" REX_FUNC(sub_8212BD78) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212BD78)
  At<Marker>(ctx.r3.u32)->camera_position = *At<Vector>(ctx.r4.u32);
}

// SmokeEmitter::SmokeEmitter: r3 the emitter. Its particles are made by
// their own constructor (Particle::Particle) later.
extern "C" REX_FUNC(sub_8212ACC0) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212ACC0)
  SmokeEmitter* emitter = At<SmokeEmitter>(ctx.r3.u32);
  for (Vector& point : emitter->points_0) {
    SetW(point);
  }
  for (Vector& point : emitter->points_144) {
    SetW(point);
  }
  for (Particle& particle : emitter->particles) {
    SetW(particle.points[0]);
    SetW(particle.points[1]);
  }
}

// Particle::Particle: r3 the particle.
extern "C" REX_FUNC(sub_8212AE38) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212AE38)
  Particle* particle = At<Particle>(ctx.r3.u32);
  SetW(particle->points[0]);
  SetW(particle->points[1]);
}

// Smoke::~Smoke: r3 the smoke. Each emitter's particles destroyed, the last
// emitter first. Returns it.
extern "C" REX_FUNC(sub_8212AD18) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212AD18)
  Smoke* smoke = At<Smoke>(ctx.r3.u32);
  smoke->overlay.vtable = Smoke::kVtable;
  for (int i = 44; i >= 0; i--) {
    DestroyArray(g, smoke->emitters[i].particles, kParticleDestructor);
  }
  ConstructOverlay(g, smoke);
  ctx.r3.u64 = guest::AddressOf(smoke);
}

// Smoke::SetVelocity: r3 the smoke, r4 the velocity.
extern "C" REX_FUNC(sub_8212A2D0) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212A2D0)
  At<Smoke>(ctx.r3.u32)->velocity = *At<Vector>(ctx.r4.u32);
}

// ParticlesB::~ParticlesB: r3 the particles. Each slot's particle
// destroyed, the last first. Returns it.
extern "C" REX_FUNC(sub_8212B9F0) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212B9F0)
  ParticlesB* particles = At<ParticlesB>(ctx.r3.u32);
  particles->overlay.vtable = ParticlesB::kVtable;
  for (int i = 14; i >= 0; i--) {
    g.Call(sub_820900C0, &particles->slots[i].particle, uint32_t(sizeof(Particle)), 1u,
           kParticleDestructor);  // The vector destructor iterator, on one particle.
  }
  ConstructOverlay(g, particles);
  ctx.r3.u64 = guest::AddressOf(particles);
}

// ParticlesB::SetPosition: r3 the particles, r4 the position.
extern "C" REX_FUNC(sub_8212B4D8) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212B4D8)
  At<ParticlesB>(ctx.r3.u32)->position = *At<Vector>(ctx.r4.u32);
}

// Sparks::Sparks: r3 the sparks. Returns them.
extern "C" REX_FUNC(sub_8212D6F8) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212D6F8)
  Sparks* sparks = At<Sparks>(ctx.r3.u32);
  ConstructOverlay(g, sparks);
  sparks->overlay.vtable = Sparks::kVtable;
  for (game::SparkSlot& slot : sparks->sparks) {
    for (Vector& point : slot.spark.points) {
      SetW(point);
    }
    SetW(slot.spark.point_112);
  }
  SetW(sparks->position);
  for (Vector& point : sparks->unknown_28976) {
    SetW(point);
  }
  ctx.r3.u64 = guest::AddressOf(sparks);
}

// Sparks::SetPosition: r3 the sparks, r4 the position.
extern "C" REX_FUNC(sub_8212CDC0) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8212CDC0)
  At<Sparks>(ctx.r3.u32)->position = *At<Vector>(ctx.r4.u32);
}

// Trail::SetLastPosition: r3 the trail, r4 the position.
extern "C" REX_FUNC(sub_82129438) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82129438)
  At<Trail>(ctx.r3.u32)->last_position = *At<Vector>(ctx.r4.u32);
}

// OverlayView::OverlayView: r3 the view.
extern "C" REX_FUNC(sub_82126288) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82126288)
  OverlayView* view = At<OverlayView>(ctx.r3.u32);
  view->vtable = OverlayView::kVtable;
  view->desc.slip = 0;
  view->desc.point = 0;
}

// OverlayView::Set: r3 the view, r4 its description. Returns the view.
extern "C" REX_FUNC(sub_821262A8) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_821262A8)
  At<OverlayView>(ctx.r3.u32)->desc = *At<ViewDesc>(ctx.r4.u32);
}

// OverlayView::WheelSlip: r3 the view; the slip object's answer
// (sub_820BFFB8 on it, the other arguments passed on).
extern "C" REX_FUNC(sub_82126478) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82126478)
  g.Call(sub_820BFFB8, At<OverlayView>(ctx.r3.u32)->desc.slip);
}

// OverlayView::HasPoint: r3 the view; the point object's answer
// (sub_820D9A78 on it, the other arguments passed on).
extern "C" REX_FUNC(sub_82126480) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82126480)
  g.Call(sub_820D9A78, At<OverlayView>(ctx.r3.u32)->desc.point);
}

// ViewDesc::Clear: r3 the description.
extern "C" REX_FUNC(sub_82126278) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82126278)
  ViewDesc* desc = At<ViewDesc>(ctx.r3.u32);
  desc->slip = 0;
  desc->point = 0;
}

// CarOverlays::ClearViews: r3 the car's overlays.
extern "C" REX_FUNC(sub_82125F38) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_82125F38)
  for (guest::u32_be& view : At<CarOverlays>(ctx.r3.u32)->views) {
    view = 0;
  }
}

// RaceOverlays::Clear: r3 the list of 25 overlays; every one unset.
extern "C" REX_FUNC(sub_8215BDE0) {
  SBX_RACE_OVERLAYS_FALLBACK(sub_8215BDE0)
  ptr<game::RaceOverlay>* overlays = At<ptr<game::RaceOverlay>>(ctx.r3.u32);
  for (int i = 0; i < game::RaceOverlaysMachine::kOverlays; i++) {
    overlays[i] = nullptr;
  }
}
