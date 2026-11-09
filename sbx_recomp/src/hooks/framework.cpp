// The app framework's getters and its interfaces' thunks as our own C++
// (--native_framework, on by default), and DebugText::SetPosition.
//
// Written from SBX's generated code; ITC-Recomp's framework/framework.cpp was
// the reference. One differs here: sub_82090858 reads +80 in SBX (ITC's
// reads +88, its framework's input manager). The run loop's containers and
// the constructors ITC also has need its native heap, so they stay
// recompiled.
//
// The getters below are shared by the linker across classes (one function
// for every class with a word at that offset), so they're named by offset.

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_framework, true, "SBX",
                    "Our own C++ for the app framework's getters, its interfaces' thunks and "
                    "DebugText::SetPosition instead of the recompiled functions");

#define SBX_FRAMEWORK_FALLBACK(name)       \
  REX_FUNC_PROLOGUE();                     \
  if (!REXCVAR_GET(native_framework)) {    \
    __imp__##name(ctx, base);              \
    return;                                \
  }

// A getter: r3 = the object; r3 = its byte or word at `offset`.
#define SBX_FRAMEWORK_BYTE(name, offset)                         \
  extern "C" REX_FUNC(name) {                                    \
    SBX_FRAMEWORK_FALLBACK(name)                                 \
    ctx.r3.u64 = guest::LoadU8(base, ctx.r3.u32 + (offset));     \
  }
#define SBX_FRAMEWORK_WORD(name, offset)                         \
  extern "C" REX_FUNC(name) {                                    \
    SBX_FRAMEWORK_FALLBACK(name)                                 \
    ctx.r3.u64 = guest::LoadU32(base, ctx.r3.u32 + (offset));    \
  }
// A thunk: r3 = an interface; the getter at `offset` of the object
// `adjust` bytes before it (the original's addi r3, then a tail call).
#define SBX_FRAMEWORK_THUNK(name, adjust, offset)                          \
  extern "C" REX_FUNC(name) {                                              \
    SBX_FRAMEWORK_FALLBACK(name)                                           \
    ctx.r3.u64 = guest::LoadU32(base, ctx.r3.u32 - (adjust) + (offset));   \
  }

SBX_FRAMEWORK_BYTE(sub_82090840, 133)  // the framework's flag read by App::Run
SBX_FRAMEWORK_BYTE(sub_82090848, 134)  // and its other one
SBX_FRAMEWORK_WORD(sub_82467570, 60)
SBX_FRAMEWORK_WORD(sub_82090850, 64)
SBX_FRAMEWORK_WORD(sub_82090858, 80)
SBX_FRAMEWORK_WORD(sub_82472750, 68)
SBX_FRAMEWORK_WORD(sub_8246CD28, 44)

SBX_FRAMEWORK_THUNK(sub_82090908, 12, 64)  // +36 interface, slot 1: the input manager
SBX_FRAMEWORK_THUNK(sub_82090900, 44, 68)  // +52 interface, slot 2: the renderer
SBX_FRAMEWORK_THUNK(sub_82090910, 28, 44)  // +52 interface, slot 1: the worker pool
SBX_FRAMEWORK_THUNK(sub_820908B8, 16, 68)  // +24 interface, slot 4
SBX_FRAMEWORK_THUNK(sub_82090938, 32, 60)  // +52 interface, slot 3: the scene
SBX_FRAMEWORK_THUNK(sub_820909B0, 28, 68)  // +36 interface, slot 2: the renderer
SBX_FRAMEWORK_THUNK(sub_820909B8, 12, 44)  // +36 interface, slot 4: the worker pool
SBX_FRAMEWORK_THUNK(sub_820909D8, 16, 60)  // +36 interface, slot 3: the scene

namespace {

// The debug font's cursor (+0, +4) and line start (+8, +12).
constexpr uint32_t kDebugFontPosition = 0x8296E140;

void DebugFontSetPosition(uint8_t* base, uint32_t x, uint32_t y) {
  guest::StoreU32(base, kDebugFontPosition + 8, x);
  guest::StoreU32(base, kDebugFontPosition + 12, y);
  guest::StoreU32(base, kDebugFontPosition + 0, x);
  guest::StoreU32(base, kDebugFontPosition + 4, y);
}

}  // namespace

// DebugText::SetPosition (vtable slot 2): r4, r5 = the x and y words'
// addresses; leaves r3, r4 = x, y and r11 = r4 as given, as the original
// (mr r11,r4, then DebugFont::SetPosition sub_82571988, which sets r11 to
// the font's position).
extern "C" REX_FUNC(sub_82091120) {
  SBX_FRAMEWORK_FALLBACK(sub_82091120)
  const uint32_t x = guest::LoadU32(base, ctx.r4.u32);
  const uint32_t y = guest::LoadU32(base, ctx.r5.u32);
  DebugFontSetPosition(base, x, y);
  ctx.r3.u64 = x;
  ctx.r4.u64 = y;
  ctx.r11.u64 = kDebugFontPosition;
}

// DebugFont::SetPosition: r3, r4 = x, y.
extern "C" REX_FUNC(sub_82571988) {
  SBX_FRAMEWORK_FALLBACK(sub_82571988)
  DebugFontSetPosition(base, ctx.r3.u32, ctx.r4.u32);
  ctx.r11.u64 = kDebugFontPosition;
}
