// The scene's set-ups as our own C++ (--native_scene, on by default): the
// scene set-up's, the scene assets' and the car scene's constructors, the
// assets' destructor and release, a list's destructor and the pages freed
// in a range.
//
// Written from SBX's generated code: each one's stores as a table in its
// order, its calls in their place. The scene's updates and teardown stay
// recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scene, true, "SBX",
                    "Our own C++ for the scene's constructors and destructors (its set-up, assets, the car scene) instead of the recompiled functions; the scene's updates stay recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// SceneSetUp::SceneSetUp (sub_820D0AE0).
// 2 stores
constexpr Fill kSceneSetUp_SceneSetUp0[] = {
    {36, kWord, 0x0, 2, 4},
};

// SceneAssets::SceneAssets (sub_820C4E00).
// 18 stores
constexpr Fill kSceneAssets_SceneAssets0[] = {
    {24, kWord, 0x0, 6, 4}, {96, kWord, 0x0, 12, 4},
};
// 6 stores
constexpr Fill kSceneAssets_SceneAssets1[] = {
    {208, kWord, 0x0, 6, 4},
};
// 6 stores
constexpr Fill kSceneAssets_SceneAssets2[] = {
    {276, kWord, 0x0, 6, 4},
};
// 6 stores
constexpr Fill kSceneAssets_SceneAssets3[] = {
    {344, kWord, 0x0, 6, 4},
};
// 18 stores
constexpr Fill kSceneAssets_SceneAssets4[] = {
    {928, kByte, 0x0, 3, 1}, {932, kHalf, 0x0, 2, 2}, {936, kByte, 0x0}, {938, kHalf, 0x0, 4, 2},
    {946, kByte, 0x0, 2, 1}, {976, kWord, 0x0, 6, 4},
};
// 6 stores
constexpr Fill kSceneAssets_SceneAssets5[] = {
    {1044, kWord, 0x0, 6, 4},
};
// 228 stores
constexpr Fill kSceneAssets_SceneAssets6[] = {
    {3668, kByte, 0x0, 3, 1}, {3672, kHalf, 0x0, 2, 2}, {3676, kByte, 0x0}, {3678, kHalf, 0x0, 4, 2},
    {3686, kByte, 0x0, 5, 1}, {3692, kHalf, 0x0, 2, 2}, {3696, kByte, 0x0}, {3698, kHalf, 0x0, 4, 2},
    {3706, kByte, 0x0, 5, 1}, {3712, kHalf, 0x0, 2, 2}, {3716, kByte, 0x0}, {3718, kHalf, 0x0, 4, 2},
    {3726, kByte, 0x0, 5, 1}, {3732, kHalf, 0x0, 2, 2}, {3736, kByte, 0x0}, {3738, kHalf, 0x0, 4, 2},
    {3746, kByte, 0x0, 5, 1}, {3752, kHalf, 0x0, 2, 2}, {3756, kByte, 0x0}, {3758, kHalf, 0x0, 4, 2},
    {3766, kByte, 0x0, 5, 1}, {3772, kHalf, 0x0, 2, 2}, {3776, kByte, 0x0}, {3778, kHalf, 0x0, 4, 2},
    {3786, kByte, 0x0, 5, 1}, {3792, kHalf, 0x0, 2, 2}, {3796, kByte, 0x0}, {3798, kHalf, 0x0, 4, 2},
    {3806, kByte, 0x0, 5, 1}, {3812, kHalf, 0x0, 2, 2}, {3816, kByte, 0x0}, {3818, kHalf, 0x0, 4, 2},
    {3826, kByte, 0x0, 5, 1}, {3832, kHalf, 0x0, 2, 2}, {3836, kByte, 0x0}, {3838, kHalf, 0x0, 4, 2},
    {3846, kByte, 0x0, 5, 1}, {3852, kHalf, 0x0, 2, 2}, {3856, kByte, 0x0}, {3858, kHalf, 0x0, 4, 2},
    {3866, kByte, 0x0, 5, 1}, {3872, kHalf, 0x0, 2, 2}, {3876, kByte, 0x0}, {3878, kHalf, 0x0, 4, 2},
    {3886, kByte, 0x0, 5, 1}, {3892, kHalf, 0x0, 2, 2}, {3896, kByte, 0x0}, {3898, kHalf, 0x0, 4, 2},
    {3906, kByte, 0x0, 5, 1}, {3912, kHalf, 0x0, 2, 2}, {3916, kByte, 0x0}, {3918, kHalf, 0x0, 4, 2},
    {3926, kByte, 0x0, 5, 1}, {3932, kHalf, 0x0, 2, 2}, {3936, kByte, 0x0}, {3938, kHalf, 0x0, 4, 2},
    {3946, kByte, 0x0, 5, 1}, {3952, kHalf, 0x0, 2, 2}, {3956, kByte, 0x0}, {3958, kHalf, 0x0, 4, 2},
    {3966, kByte, 0x0, 5, 1}, {3972, kHalf, 0x0, 2, 2}, {3976, kByte, 0x0}, {3978, kHalf, 0x0, 4, 2},
    {3986, kByte, 0x0, 5, 1}, {3992, kHalf, 0x0, 2, 2}, {3996, kByte, 0x0}, {3998, kHalf, 0x0, 4, 2},
    {4006, kByte, 0x0, 5, 1}, {4012, kHalf, 0x0, 2, 2}, {4016, kByte, 0x0}, {4018, kHalf, 0x0, 4, 2},
    {4026, kByte, 0x0, 5, 1}, {4032, kHalf, 0x0, 2, 2}, {4036, kByte, 0x0}, {4038, kHalf, 0x0, 4, 2},
    {4046, kByte, 0x0, 2, 1},
};
// 24 stores
constexpr Fill kSceneAssets_SceneAssets7[] = {
    {4656, kWord, 0x0, 24, 4},
};

// CarScene::CarScene (sub_820C8760).
// 2 stores
constexpr Fill kCarScene_CarScene0[] = {
    {40, kWord, 0x0, 2, 4},
};
// 38 stores
constexpr Fill kCarScene_CarScene1[] = {
    {4896, kWord, 0x0, 2, 8}, {4908, kWord, 0x0, 2, 8}, {4920, kWord, 0x0, 2, 8}, {4932, kWord, 0x0, 2, 8},
    {4944, kWord, 0x0, 2, 8}, {4956, kWord, 0x0, 2, 8}, {4968, kWord, 0x0, 2, 8}, {4980, kWord, 0x0, 2, 8},
    {5132, kImage, 0x82000A20, 22, 16},
};

bool On() { return REXCVAR_GET(native_scene); }

}  // namespace

// SceneSetUp::SceneSetUp.
extern "C" REX_FUNC(sub_820D0AE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D0AE0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneSetUp_SceneSetUp0, args);
  ctx.r3.u64 = object;
}

// FreePagesIn.
extern "C" REX_FUNC(sub_820D0750) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D0750(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = guest::LoadU32(base, object + 0);
  ctx.r4.u64 = 0x0u;
  ctx.r5.u64 = 0x8000u;
  sub_82292D70(ctx, base);
}

// SceneAssets::SceneAssets.
extern "C" REX_FUNC(sub_820C4E00) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C4E00(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets0, args);
  ctx.r3.u64 = object + 144;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 164;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets1, args);
  ctx.r3.u64 = object + 232;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets2, args);
  ctx.r3.u64 = object + 300;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets3, args);
  ctx.r3.u64 = object + 368;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 416;
  sub_820CE6E8(ctx, base);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets4, args);
  ctx.r3.u64 = object + 1000;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets5, args);
  ctx.r3.u64 = object + 1068;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 1184;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 1648;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 2112;
  sub_820CE6E8(ctx, base);
  ctx.r3.u64 = object + 2576;
  sub_820CE6E8(ctx, base);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets6, args);
  ctx.r3.u64 = object + 4096;
  sub_820CE6E8(ctx, base);
  guest_memory::Apply(base, object, kSceneAssets_SceneAssets7, args);
  ctx.r3.u64 = object + 4752;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 4772;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 4792;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object + 4812;
  ctx.r4.u64 = 0xFFFFFFFFFFFFFFFFu;
  sub_8257A150(ctx, base);
  ctx.r3.u64 = object;
}

// SceneAssets::~SceneAssets.
extern "C" REX_FUNC(sub_820C4F58) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C4F58(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 4812;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 4792;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 4772;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 4752;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 4728;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 4704;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 4680;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 4656;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 4096;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 2576;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 2112;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1648;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1184;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 1068;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 1044;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 1000;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 976;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 416;
  sub_820CE750(ctx, base);
  ctx.r3.u64 = object + 368;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 344;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 300;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 276;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 232;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 208;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 164;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 144;
  sub_82579948(ctx, base);
  ctx.r3.u64 = object + 120;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 96;
  sub_8257CA60(ctx, base);
  ctx.r3.u64 = object + 24;
  sub_8257CA60(ctx, base);
}

// CarScene::CarScene.
extern "C" REX_FUNC(sub_820C8760) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C8760(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kCarScene_CarScene0, args);
  ctx.r3.u64 = object + 64;
  sub_820C4E00(ctx, base);
  guest_memory::Apply(base, object, kCarScene_CarScene1, args);
  ctx.r3.u64 = object;
}

// List::~List.
extern "C" REX_FUNC(sub_82579750) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82579750(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_825795A8(ctx, base);
}

// SceneAssets::Release.
extern "C" REX_FUNC(sub_820C4A80) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820C4A80(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 0;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 48;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 72;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 184;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 252;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 320;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 388;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 880;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 904;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 948;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 952;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 1020;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 4048;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 4072;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 4096;
  sub_8257A270(ctx, base);
  ctx.r3.u64 = object + 1088;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3040;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 1112;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3064;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 1136;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3088;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 1160;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3112;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3136;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3592;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3160;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3596;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3184;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3600;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3208;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3604;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3232;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3608;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3256;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3612;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3280;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3616;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3304;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3620;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3328;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3624;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3352;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3628;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3376;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3632;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3400;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3636;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3424;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3640;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3448;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3644;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3472;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3648;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3496;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3652;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3520;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3656;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3544;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3660;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 3568;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 3664;
  sub_820D0750(ctx, base);
  ctx.r3.u64 = object + 4560;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 4584;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 4608;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 4632;
  sub_820D0518(ctx, base);
  ctx.r3.u64 = object + 0;
  sub_820C4880(ctx, base);
}

