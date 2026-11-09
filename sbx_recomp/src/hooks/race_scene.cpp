// The race scene as our own C++ (--native_race_scene, on by default): the
// race's scene object (made and freed once a race), its light set, the post
// effects' glow and color parameters it sets, and the race scene machine's
// pause toggle.
//
// Written from SBX's generated code. The scene object is at [0x826C7D7C]
// (31,168 bytes); the post effects' split-screen flag at [[0x826B29C8] + 8].
// The post effects' setters are called through their vtables as the
// original does, so --bloom's hooks on them still apply.
// RaceSceneMachine::Init (sub_8215B768) stays recompiled: it builds its 11
// transitions in its stack frame and copies words from there the original
// never writes, which only the original reproduces.

#include "guest.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(native_race_scene, true, "SBX",
                    "Our own C++ for the race scene (its object, light set, the post effects' "
                    "glow and color parameters, the pause toggle) instead of the recompiled "
                    "functions; the scene machine's set-up stays recompiled");

namespace {

constexpr uint32_t kScene = 0x826C7D7C;  // [it] the race scene
constexpr uint32_t kSceneBytes = 31168;
constexpr uint32_t kPostEffects = 0x826B29C8;  // [it] + 8: split screen
constexpr uint32_t kPaused = 9608;  // the machine's pause toggle

// The image's constants the original reads (lfs, lfd).
constexpr uint32_t kOne = 0x82000A20;        // 1.0f
constexpr uint32_t kZero = 0x82000A24;       // 0.0f
constexpr uint32_t kPointLevel = 0x82001068;
constexpr uint32_t kSpotX = 0x82001BDC;
constexpr uint32_t kSpot2 = 0x82002530;
constexpr uint32_t kSpot3 = 0x820021EC;
constexpr uint32_t kInnerAngle = 0x820096C8;  // a double
constexpr uint32_t kOuterAngle = 0x82002FC0;  // a double
constexpr uint32_t kColorLow = 0x82001F98;
constexpr uint32_t kColorHigh = 0x820096D0;
constexpr uint32_t kFilterValue = 0x820021C4;

// The effect parameters: 44-byte entries (11 floats) at 0x82627C18 + 44 x
// (3 x kind + index), and the one for other kinds.
constexpr uint32_t kEffectParams = 0x82627C18;
constexpr uint32_t kOtherEffectParams = 0x82627BEC;

// sub_820900C0's destructor for the cars' records (an empty one).
constexpr uint32_t kRecordDestructor = 0x821611F0;

uint32_t Scene(uint8_t* base) { return guest::LoadU32(base, kScene); }

uint32_t Word(uint8_t* base, uint32_t address) { return guest::LoadU32(base, address); }

// Four words (floats as the original stores them).
void Store4(uint8_t* base, uint32_t at, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  guest::StoreU32(base, at + 0, a);
  guest::StoreU32(base, at + 4, b);
  guest::StoreU32(base, at + 8, c);
  guest::StoreU32(base, at + 12, d);
}

// cos (sub_8258E218) of the double at `address`, rounded to a float (frsp).
uint32_t Cos(PPCContext& ctx, uint8_t* base, uint32_t address, uint32_t lr) {
  ctx.f1.u64 = REX_LOAD_U64(address);
  ctx.lr = lr;
  sub_8258E218(ctx, base);
  const float value = float(ctx.f1.f64);
  uint32_t bits;
  std::memcpy(&bits, &value, 4);
  return bits;
}

// RaceLights::InitEntry: a light entry (96 bytes) for its index; nothing
// for 11 and up (signed). +0 the kind: 1 for indices below 3 (+32 its
// level, +48 white), else 2 (a spot: +32, +48 white, +64, +80 the cosines
// of its outer and inner angles); +16 its color by the index. The cosines
// are taken from the original's frame (176 bytes).
void InitEntry(PPCContext& ctx, uint8_t* base, uint32_t entry, int32_t index) {
  if (index >= 11) {
    return;
  }
  const uint32_t one = Word(base, kOne);
  const uint32_t zero = Word(base, kZero);
  guest::StoreU32(base, entry + 0, 0);
  if (index < 3) {
    const uint32_t level = Word(base, kPointLevel);
    guest::StoreU32(base, entry + 0, 1);
    Store4(base, entry + 32, level, level, level, one);
    Store4(base, entry + 48, one, one, one, one);
  } else {
    guest::StoreU32(base, entry + 0, 2);
    Store4(base, entry + 32, Word(base, kSpotX), zero, zero, one);
    Store4(base, entry + 48, one, one, one, one);
    Store4(base, entry + 64, Word(base, kSpot2), Word(base, kSpot3), one, one);
    uint32_t inner = 0, outer = 0;
    guest_call::InFrame(ctx, base, 176, 0x8215A4B0, [&](uint32_t) {
      inner = Cos(ctx, base, kInnerAngle, 0x8215A4B0);
      outer = Cos(ctx, base, kOuterAngle, 0x8215A4C0);
    });
    Store4(base, entry + 80, outer, inner, zero, one);
  }
  const uint32_t low = Word(base, kColorLow);
  const uint32_t high = Word(base, kColorHigh);
  switch (index) {
    case 1: Store4(base, entry + 16, low, low, high, one); break;
    case 2: Store4(base, entry + 16, high, low, low, one); break;
    case 3: Store4(base, entry + 16, low, low, low, one); break;
    case 4: Store4(base, entry + 16, zero, low, high, one); break;
    case 5: Store4(base, entry + 16, high, low, zero, one); break;
    case 6: Store4(base, entry + 16, low, low, zero, one); break;
    default: Store4(base, entry + 16, high, low, high, one); break;
  }
}

// A post effect (sub_821325C0: the effect by its number).
uint32_t Effect(PPCContext& ctx, uint8_t* base, uint32_t number) {
  ctx.r3.u64 = number;
  sub_821325C0(ctx, base);
  return ctx.r3.u32;
}

// An effect's vtable slot at `offset` with f1 = the float at `value`, from
// the caller's frame with `lr` the original's return address.
void SetFloat(PPCContext& ctx, uint8_t* base, uint32_t effect, uint32_t offset, uint32_t value,
              uint32_t frame_bytes, uint32_t lr) {
  guest_call::InFrame(ctx, base, frame_bytes, lr, [&](uint32_t) {
    ctx.r3.u64 = effect;
    ctx.f1.f64 = double(guest::LoadFloat(base, value));
    REX_CALL_INDIRECT_FUNC(guest::LoadU32(base, guest::LoadU32(base, effect) + offset));
  });
}

bool SplitScreen(uint8_t* base) { return Word(base, Word(base, kPostEffects) + 8) != 0; }

// Glows::SetParams: the 11 floats at `params` given to each glow's slots 2
// to 12 (+8 to +48) in order: glows 5 and 9 in split screen, else glow 1.
// r3 = the last glow, as the slots leave it.
void SetGlowParams(PPCContext& ctx, uint8_t* base, uint32_t params) {
  constexpr uint32_t kFrame = 112;
  auto set_all = [&](uint32_t glow, uint32_t first_lr, uint32_t last_lr) {
    for (uint32_t k = 0; k < 11; k++) {
      SetFloat(ctx, base, glow, 8 + 4 * k, params + 4 * k, kFrame,
               k < 10 ? first_lr + 24 * k : last_lr);
    }
  };
  if (SplitScreen(base)) {
    const uint32_t first = Effect(ctx, base, 5);
    const uint32_t second = Effect(ctx, base, 9);
    set_all(first, 0x8228C4EC, 0x8228C5DC);
    set_all(second, 0x8228C5F4, 0x8228C7E8);
  } else {
    set_all(Effect(ctx, base, 1), 0x8228C6F8, 0x8228C7E8);
  }
}

// RaceScene::DestroyCars: the seven car slots' record arrays (sub_820900C0,
// the array destructor iterator, with the empty destructor), last to first.
void DestroyCars(PPCContext& ctx, uint8_t* base, uint32_t cars) {
  // Each slot's arrays: offset from the slot's +2896, element bytes, count.
  struct Array {
    int32_t offset;
    uint32_t bytes;
    uint32_t count;
  };
  static constexpr Array kArrays[] = {
      {384, 64, 6},   {0, 64, 6},      {-384, 64, 6},   {-768, 64, 6},  {-1152, 64, 6},
      {-1536, 64, 6}, {-1920, 64, 6},  {-2304, 64, 6},  {-2496, 64, 3}, {-2688, 64, 3},
      {-2752, 16, 3}, {-2800, 16, 3},  {-2848, 16, 3},
  };
  uint32_t slot = cars + 30032 + 2896;
  for (int i = 0; i < 7; i++) {
    slot -= 4288;
    uint32_t lr = 0x8215B4E4;
    for (const Array& array : kArrays) {
      guest_call::InFrame(ctx, base, 112, lr, [&](uint32_t) {
        ctx.r3.u64 = uint32_t(int32_t(slot) + array.offset);
        ctx.r4.u64 = array.bytes;
        ctx.r5.u64 = array.count;
        ctx.r6.u64 = kRecordDestructor;
        sub_820900C0(ctx, base);
      });
      lr += 24;
    }
  }
}

}  // namespace

#define SBX_RACE_SCENE_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                    \
  if (!REXCVAR_GET(native_race_scene)) {  \
    __imp__##name(ctx, base);             \
    return;                               \
  }

// --- The scene object ---

// RaceScene::RaceScene: r3 = the scene. The seven car slots' records
// (sub_820DAF08 on +48 + 4,288 i), then the w of the 11 light entries' five
// vectors (+30108 + 96 i, 16 apart) = 1.0. r3 = the scene.
extern "C" REX_FUNC(sub_8215B628) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B628)
  const uint32_t scene = ctx.r3.u32;
  guest_call::InFrame(ctx, base, 112, 0x8215B64C, [&](uint32_t) {
    for (uint32_t i = 0; i < 7; i++) {
      ctx.r3.u64 = scene + 48 + 4288 * i;
      ctx.lr = 0x8215B64C;
      sub_820DAF08(ctx, base);
    }
  });
  ctx.fpscr.disableFlushMode();
  const uint32_t one = Word(base, kOne);
  for (uint32_t i = 0; i < 11; i++) {
    for (uint32_t k = 0; k < 5; k++) {
      guest::StoreU32(base, scene + 30108 + 96 * i + 16 * k, one);
    }
  }
  ctx.r3.u64 = scene;
}

// RaceScene::Create: [kScene] = 0, then the scene made (operator new and
// RaceScene::RaceScene) into it, or 0 when the allocation fails.
extern "C" REX_FUNC(sub_8215B698) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B698)
  guest::StoreU32(base, kScene, 0);
  guest_call::InFrame(ctx, base, 96, 0x8215B6BC, [&](uint32_t) {
    ctx.r3.u64 = kSceneBytes;
    sub_8258C000(ctx, base);  // operator new
    if (ctx.r3.u32 != 0) {
      ctx.lr = 0x8215B6C8;
      sub_8215B628(ctx, base);
      guest::StoreU32(base, kScene, ctx.r3.u32);
    } else {
      guest::StoreU32(base, kScene, 0);
    }
  });
}

// RaceScene::DestroyCars: r3 = the scene's +16.
extern "C" REX_FUNC(sub_8215B4A8) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B4A8)
  DestroyCars(ctx, base, ctx.r3.u32);
}

// RaceScene::Destroy: when there's a scene, its cars' records freed and the
// scene deleted (the pointer is left).
extern "C" REX_FUNC(sub_8215B6F0) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B6F0)
  const uint32_t scene = Scene(base);
  if (scene == 0) {
    return;
  }
  guest_call::InFrame(ctx, base, 96, 0x8215B718, [&](uint32_t) {
    DestroyCars(ctx, base, scene + 16);
    ctx.r3.u64 = scene;
    ctx.lr = 0x8215B720;
    sub_8258C868(ctx, base);  // operator delete
  });
}

// --- The light set ---

// RaceLights::InitEntry: r3 = the entry, r4 = its index. r3 left (as cos
// leaves it for a spot).
extern "C" REX_FUNC(sub_8215A370) {
  SBX_RACE_SCENE_FALLBACK(sub_8215A370)
  ctx.fpscr.disableFlushMode();
  InitEntry(ctx, base, ctx.r3.u32, ctx.r4.s32);
}

// RaceLights::SetUp: r3 = the set. +0, +1072, +8 and +4 = 0, then the 11
// entries (+16 + 96 i) set up and counted: +4 the kind 1s, +8 the kind 2s,
// +0 both. r3 as the last entry's set-up leaves it.
extern "C" REX_FUNC(sub_8215AF70) {
  SBX_RACE_SCENE_FALLBACK(sub_8215AF70)
  ctx.fpscr.disableFlushMode();
  const uint32_t set = ctx.r3.u32;
  guest::StoreU32(base, set + 0, 0);
  guest::StoreU32(base, set + 1072, 0);
  guest::StoreU32(base, set + 8, 0);
  guest::StoreU32(base, set + 4, 0);
  guest_call::InFrame(ctx, base, 112, 0x8215AFA8, [&](uint32_t) {
    for (int32_t i = 0; i < 11; i++) {
      const uint32_t entry = set + 16 + 96 * uint32_t(i);
      InitEntry(ctx, base, entry, i);
      const uint32_t kind = Word(base, entry);
      if (kind == 1) {
        guest::StoreU32(base, set + 4, Word(base, set + 4) + 1);
      } else if (kind == 2) {
        guest::StoreU32(base, set + 8, Word(base, set + 8) + 1);
      } else {
        continue;
      }
      guest::StoreU32(base, set + 0, Word(base, set + 0) + 1);
    }
  });
}

// --- The post effects' parameters ---

// Glows::SetParams: r3 = the 11 floats.
extern "C" REX_FUNC(sub_8228C498) {
  SBX_RACE_SCENE_FALLBACK(sub_8228C498)
  ctx.fpscr.disableFlushMode();
  SetGlowParams(ctx, base, ctx.r3.u32);
}

// RaceScene::SetEffectParams: r3 = the kind, r4 = the index. The glows'
// parameters (entry 3 x kind + index, or the other kinds' for kinds 8 and
// up), then the color filters' slot 8 (+32) with the float at 0x820021C4:
// filters 6 and 10 in split screen, else filter 2.
extern "C" REX_FUNC(sub_8228C9A8) {
  SBX_RACE_SCENE_FALLBACK(sub_8228C9A8)
  ctx.fpscr.disableFlushMode();
  constexpr uint32_t kFrame = 112;
  const int32_t kind = ctx.r3.s32;
  const int32_t index = ctx.r4.s32;
  // As the original: the bound tested on kind + 2 below 2 (always in), the
  // entry from kind itself.
  const int32_t tested = kind < 2 ? kind + 2 : kind;
  const uint32_t params =
      tested < 8 ? kEffectParams + uint32_t(kind * 3 + index) * 44 : kOtherEffectParams;
  guest_call::InFrame(ctx, base, kFrame, 0x8228CA00,
                      [&](uint32_t) { SetGlowParams(ctx, base, params); });
  if (SplitScreen(base)) {
    const uint32_t first = Effect(ctx, base, 6);
    const uint32_t second = Effect(ctx, base, 10);
    SetFloat(ctx, base, first, 32, kFilterValue, kFrame, 0x8228CA4C);
    SetFloat(ctx, base, second, 32, kFilterValue, kFrame, 0x8228CA7C);
  } else {
    SetFloat(ctx, base, Effect(ctx, base, 2), 32, kFilterValue, kFrame, 0x8228CA7C);
  }
}

// --- The scene machine (r3 = the machine) ---

// RaceSceneMachine::TogglePause: +9608 flipped (1 when it was 0, else 0),
// then sub_82159698 on the scene (the post effects' pause toggle). r3 as it
// leaves it.
extern "C" REX_FUNC(sub_8215A758) {
  SBX_RACE_SCENE_FALLBACK(sub_8215A758)
  const uint32_t machine = ctx.r3.u32;
  guest::StoreU32(base, machine + kPaused, Word(base, machine + kPaused) == 0 ? 1 : 0);
  ctx.r3.u64 = Scene(base);
  sub_82159698(ctx, base);
}

// RaceSceneMachine::Resume: when +9608 is set, it's cleared and
// sub_82159698 runs on the scene; else nothing (r3 left).
extern "C" REX_FUNC(sub_8215B0D0) {
  SBX_RACE_SCENE_FALLBACK(sub_8215B0D0)
  const uint32_t machine = ctx.r3.u32;
  if (Word(base, machine + kPaused) == 0) {
    return;
  }
  guest::StoreU32(base, machine + kPaused, 0);
  ctx.r3.u64 = Scene(base);
  sub_82159698(ctx, base);
}
