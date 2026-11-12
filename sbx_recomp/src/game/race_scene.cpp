// The race scene as our own C++ (--native_race_scene, on by default): the
// race's scene object (made and freed once a race), its light set, the post
// effects' glow and color parameters it sets, and the race scene machine's
// pause toggle. The post effects' setters are called through their vtables
// as the original does, so --bloom's hooks on them still apply.
// RaceSceneMachine::Init (sub_8215B768) stays recompiled: it builds its 11
// transitions in its stack frame and copies words from there the original
// never writes, which only the original reproduces.
//
// Calls are made from frames the size of the original's, with its return
// addresses, as the original makes them (guest_call::InFrame).

#include "game/race_scene.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_scene, true, "SBX",
                    "Our own C++ for the race scene (its object, light set, the post effects' "
                    "glow and color parameters, the pause toggle) instead of the recompiled "
                    "functions; the scene machine's set-up stays recompiled");

namespace {

using game::Matrix;
using game::PostEffectObject;
using game::RaceScene;
using game::RaceSceneMachine;
using game::SceneCar;
using game::SceneCars;
using game::RaceLight;
using game::RaceLights;
using game::Vector;
using guest::AddressOf;
using guest::At;
using guest::Guest;
using guest::ReturnAddress;
using guest::u32_be;
using guest_call::InFrame;

constexpr uint32_t kScene = 0x826C7D7C;        // [it] the race scene (writable)
constexpr uint32_t kPostEffects = 0x826B29C8;  // [it] the post effects' state (writable)

// The effect parameters (writable): 44-byte entries (11 floats) at
// 0x82627C18 + 44 x (3 x kind + index), and the one for other kinds.
constexpr uint32_t kEffectParams = 0x82627C18;
constexpr uint32_t kOtherEffectParams = 0x82627BEC;
constexpr uint32_t kEffectParamBytes = 44;

// The lights' constants (the image's read-only data).
constexpr float kPointLevel = 0.8f;
constexpr float kSpotLevel = 0.5f;
constexpr float kSpotFalloff = 0.0001f;  // 0x82002530
constexpr float kSpotRange = 0.01f;      // 0x820021EC
constexpr double kInnerAngle = 0.06981316953897476;  // 4 degrees
constexpr double kOuterAngle = 0.04363323003053665;  // 2.5 degrees
constexpr float kColorLow = 500.0f;
constexpr float kColorHigh = -500.0f;
constexpr float kFilterValue = 0.05f;  // 0x820021C4

// The cars' records' destructor (an empty one).
constexpr uint32_t kRecordDestructor = 0x821611F0;

RaceScene* Scene() { return At<RaceScene>(*At<u32_be>(kScene)); }

void Set(Vector& v, float x, float y, float z, float w) {
  v.x = x;
  v.y = y;
  v.z = z;
  v.w = w;
}

// cos (sub_8258E218), rounded to a float, called from `return_address`.
float Cos(Guest& g, double angle, uint32_t return_address) {
  g.ctx.f1.f64 = angle;
  ReturnAddress from(g, return_address);
  g.Call(sub_8258E218);
  return float(g.ctx.f1.f64);
}

// RaceLights::InitEntry: a light for its index; nothing for 11 and up
// (signed). The first three are point lights, the rest spots; each gets a
// color by its index. A spot's cosines are taken from the original's frame
// (176 bytes).
void InitLight(Guest& g, RaceLight* light, int32_t index) {
  if (index >= RaceLights::kLights) {
    return;
  }
  light->kind = 0;
  if (index < 3) {
    light->kind = RaceLight::kPoint;
    Set(light->level, kPointLevel, kPointLevel, kPointLevel, 1.0f);
    Set(light->white, 1.0f, 1.0f, 1.0f, 1.0f);
  } else {
    light->kind = RaceLight::kSpot;
    Set(light->level, kSpotLevel, 0.0f, 0.0f, 1.0f);
    Set(light->white, 1.0f, 1.0f, 1.0f, 1.0f);
    Set(light->spot, kSpotFalloff, kSpotRange, 1.0f, 1.0f);
    float inner = 0.0f, outer = 0.0f;
    InFrame(g.ctx, g.base, 176, 0x8215A4B0, [&](uint32_t) {
      inner = Cos(g, kInnerAngle, 0x8215A4B0);
      outer = Cos(g, kOuterAngle, 0x8215A4C0);
    });
    Set(light->cone, outer, inner, 0.0f, 1.0f);
  }
  const float low = kColorLow, high = kColorHigh;
  switch (index) {
    case 1: Set(light->color, low, low, high, 1.0f); break;
    case 2: Set(light->color, high, low, low, 1.0f); break;
    case 3: Set(light->color, low, low, low, 1.0f); break;
    case 4: Set(light->color, 0.0f, low, high, 1.0f); break;
    case 5: Set(light->color, high, low, 0.0f, 1.0f); break;
    case 6: Set(light->color, low, low, 0.0f, 1.0f); break;
    default: Set(light->color, high, low, high, 1.0f); break;
  }
}

// A post effect, by its number (sub_821325C0).
PostEffectObject* Effect(Guest& g, uint32_t number) {
  return At<PostEffectObject>(g.Call(sub_821325C0, number));
}

// An effect's vtable slot with f1 = `value`, from a frame of
// `frame_bytes` with the original's return address.
void SetFloat(Guest& g, PostEffectObject* effect, uint32_t slot, float value, uint32_t frame_bytes,
              uint32_t return_address) {
  InFrame(g.ctx, g.base, frame_bytes, return_address, [&](uint32_t) {
    g.ctx.f1.f64 = double(value);
    g.CallVirtual(effect, slot);
  });
}

bool SplitScreen() { return At<game::PostEffectsState>(*At<u32_be>(kPostEffects))->split_screen != 0; }

// Glows::SetParams: the 11 floats at `params` given to each glow's slots 2
// to 12 in order: glows 5 and 9 in split screen, else glow 1. r3 = the last
// glow, as the slots leave it.
void SetGlowParams(Guest& g, uint32_t params) {
  constexpr uint32_t kFrame = 112;
  const guest::f32_be* values = At<guest::f32_be>(params);
  auto set_all = [&](PostEffectObject* glow, uint32_t first_return, uint32_t last_return) {
    for (uint32_t k = 0; k < 11; k++) {
      SetFloat(g, glow, 2 + k, values[k], kFrame, k < 10 ? first_return + 24 * k : last_return);
    }
  };
  if (SplitScreen()) {
    PostEffectObject* first = Effect(g, 5);
    PostEffectObject* second = Effect(g, 9);
    set_all(first, 0x8228C4EC, 0x8228C5DC);
    set_all(second, 0x8228C5F4, 0x8228C7E8);
  } else {
    set_all(Effect(g, 1), 0x8228C6F8, 0x8228C7E8);
  }
}

// RaceScene::DestroyCars: each car's arrays destroyed (sub_820900C0, the
// array destructor iterator, with the empty destructor), the last car and
// array first, each from the original's frame and return address.
void DestroyCars(Guest& g, SceneCars* cars) {
  for (int i = 6; i >= 0; i--) {
    SceneCar& car = cars->cars[i];
    uint32_t return_address = 0x8215B4E4;
    auto destroy = [&](auto& array) {
      using Element = std::remove_reference_t<decltype(array[0])>;
      InFrame(g.ctx, g.base, 112, return_address, [&](uint32_t) {
        g.Call(sub_820900C0, array, uint32_t(sizeof(Element)),
               uint32_t(sizeof(array) / sizeof(Element)), kRecordDestructor);
      });
      return_address += 24;
    };
    destroy(car.matrices_3264);
    destroy(car.matrices_2880);
    destroy(car.matrices_2496);
    destroy(car.matrices_2112);
    destroy(car.matrices_1728);
    destroy(car.matrices_1344);
    destroy(car.matrices_960);
    destroy(car.matrices_576);
    destroy(car.matrices_384);
    destroy(car.matrices_192);
    destroy(car.points_128);
    destroy(car.points_80);
    destroy(car.points_32);
  }
}

}  // namespace

#define SBX_RACE_SCENE_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                    \
  if (!REXCVAR_GET(native_race_scene)) {  \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// --- The scene object ---

// RaceScene::RaceScene: r3 the scene. The seven cars' records
// (sub_820DAF08 on each), then w = 1 on the lights' five vectors. Returns
// the scene.
extern "C" REX_FUNC(sub_8215B628) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B628)
  RaceScene* scene = At<RaceScene>(ctx.r3.u32);
  InFrame(ctx, base, 112, 0x8215B64C, [&](uint32_t) {
    for (SceneCar& car : scene->cars.cars) {
      ReturnAddress from(g, 0x8215B64C);
      g.Call(sub_820DAF08, &car);
    }
  });
  ctx.fpscr.disableFlushMode();
  for (RaceLight& light : scene->lights.lights) {
    for (Vector* v : {&light.color, &light.level, &light.white, &light.spot, &light.cone}) {
      v->w = 1.0f;
    }
  }
  ctx.r3.u64 = AddressOf(scene);
}

// RaceScene::Create: [0x826C7D7C] = 0, then the scene made (operator new
// and RaceScene::RaceScene) into it, or 0 when the allocation fails.
extern "C" REX_FUNC(sub_8215B698) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B698)
  u32_be& slot = *At<u32_be>(kScene);
  slot = 0;
  InFrame(ctx, base, 96, 0x8215B6BC, [&](uint32_t) {
    const uint32_t memory = g.Call(sub_8258C000, uint32_t(sizeof(RaceScene)));  // operator new
    if (memory != 0) {
      ReturnAddress from(g, 0x8215B6C8);
      slot = g.Call(sub_8215B628, memory);  // RaceScene::RaceScene
    } else {
      slot = 0;
    }
  });
}

// RaceScene::DestroyCars: r3 the scene's cars (+16).
extern "C" REX_FUNC(sub_8215B4A8) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B4A8)
  DestroyCars(g, At<SceneCars>(ctx.r3.u32));
}

// RaceScene::Destroy: when there's a scene, its cars' records freed and the
// scene deleted (the pointer is left).
extern "C" REX_FUNC(sub_8215B6F0) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B6F0)
  RaceScene* scene = Scene();
  if (scene == nullptr) {
    return;
  }
  InFrame(ctx, base, 96, 0x8215B718, [&](uint32_t) {
    DestroyCars(g, &scene->cars);
    ReturnAddress from(g, 0x8215B720);
    g.Call(sub_8258C868, scene);  // operator delete
  });
}

// --- The light set ---

// RaceLights::InitEntry: r3 the light, r4 its index. r3 left (as cos
// leaves it for a spot).
extern "C" REX_FUNC(sub_8215A370) {
  SBX_RACE_SCENE_FALLBACK(sub_8215A370)
  ctx.fpscr.disableFlushMode();
  InitLight(g, At<RaceLight>(ctx.r3.u32), ctx.r4.s32);
}

// RaceLights::SetUp: r3 the set. Its counts cleared, then the 11 lights set
// up and counted (point lights, spots, both). r3 as the last light's
// set-up leaves it.
extern "C" REX_FUNC(sub_8215AF70) {
  SBX_RACE_SCENE_FALLBACK(sub_8215AF70)
  ctx.fpscr.disableFlushMode();
  RaceLights* set = At<RaceLights>(ctx.r3.u32);
  set->count = 0;
  set->unknown_1072 = 0;
  set->spots = 0;
  set->points = 0;
  InFrame(ctx, base, 112, 0x8215AFA8, [&](uint32_t) {
    for (int32_t i = 0; i < RaceLights::kLights; i++) {
      RaceLight& light = set->lights[i];
      InitLight(g, &light, i);
      if (light.kind == RaceLight::kPoint) {
        set->points += 1;
      } else if (light.kind == RaceLight::kSpot) {
        set->spots += 1;
      } else {
        continue;
      }
      set->count += 1;
    }
  });
}

// --- The post effects' parameters ---

// Glows::SetParams: r3 the 11 floats.
extern "C" REX_FUNC(sub_8228C498) {
  SBX_RACE_SCENE_FALLBACK(sub_8228C498)
  ctx.fpscr.disableFlushMode();
  SetGlowParams(g, ctx.r3.u32);
}

// RaceScene::SetEffectParams: r3 the kind, r4 the index. The glows'
// parameters (entry 3 x kind + index, or the other kinds' for kinds 8 and
// up), then the color filters' slot 8 with 0.05: filters 6 and 10 in split
// screen, else filter 2.
extern "C" REX_FUNC(sub_8228C9A8) {
  SBX_RACE_SCENE_FALLBACK(sub_8228C9A8)
  ctx.fpscr.disableFlushMode();
  constexpr uint32_t kFrame = 112;
  const int32_t kind = ctx.r3.s32;
  const int32_t index = ctx.r4.s32;
  // As the original: the bound tested on kind + 2 below 2 (always in), the
  // entry from kind itself.
  const int32_t tested = kind < 2 ? kind + 2 : kind;
  const uint32_t params = tested < 8 ? kEffectParams + uint32_t(kind * 3 + index) * kEffectParamBytes
                                     : kOtherEffectParams;
  InFrame(ctx, base, kFrame, 0x8228CA00, [&](uint32_t) { SetGlowParams(g, params); });
  if (SplitScreen()) {
    PostEffectObject* first = Effect(g, 6);
    PostEffectObject* second = Effect(g, 10);
    SetFloat(g, first, 8, kFilterValue, kFrame, 0x8228CA4C);
    SetFloat(g, second, 8, kFilterValue, kFrame, 0x8228CA7C);
  } else {
    SetFloat(g, Effect(g, 2), 8, kFilterValue, kFrame, 0x8228CA7C);
  }
}

// --- The scene machine (r3 the machine) ---

// RaceSceneMachine::TogglePause: the pause flipped, then sub_82159698 on the
// scene (the post effects' pause toggle). r3 as it leaves it.
extern "C" REX_FUNC(sub_8215A758) {
  SBX_RACE_SCENE_FALLBACK(sub_8215A758)
  RaceSceneMachine* machine = At<RaceSceneMachine>(ctx.r3.u32);
  machine->paused = machine->paused == 0 ? 1 : 0;
  g.Call(sub_82159698, *At<u32_be>(kScene));
}

// RaceSceneMachine::Resume: when paused, the pause cleared and
// sub_82159698 run on the scene; else nothing (r3 left).
extern "C" REX_FUNC(sub_8215B0D0) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B0D0)
  RaceSceneMachine* machine = At<RaceSceneMachine>(ctx.r3.u32);
  if (machine->paused == 0) {
    return;
  }
  machine->paused = 0;
  g.Call(sub_82159698, *At<u32_be>(kScene));
}
