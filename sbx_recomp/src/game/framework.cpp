// The app framework's getters and its interfaces' thunks as our own C++
// (--native_framework, on by default), and DebugText::SetPosition.
//
// The run loop's containers and the framework's constructors stay
// recompiled.
//
// The getters below are shared by the linker across classes (one function
// for every class with a word at that offset), so they're named by offset.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_framework, true, "SBX",
                    "Our own C++ for the app framework's getters, its interfaces' thunks and "
                    "DebugText::SetPosition instead of the recompiled functions");

#define SBX_FRAMEWORK_FALLBACK(name)       \
  REX_FUNC_PROLOGUE();                     \
  if (!REXCVAR_GET(native_framework)) {    \
    __imp__##name(ctx, base);              \
    return;                                \
  }                                        \
  guest::Guest g(ctx, base);

// A getter: r3 = the object; r3 = its byte or word at `offset`.
#define SBX_FRAMEWORK_BYTE(name, offset)                         \
  extern "C" REX_FUNC(name) {                                    \
    SBX_FRAMEWORK_FALLBACK(name)                                 \
    ctx.r3.u64 = *guest::At<uint8_t>(ctx.r3.u32 + (offset));     \
  }
#define SBX_FRAMEWORK_WORD(name, offset)                             \
  extern "C" REX_FUNC(name) {                                        \
    SBX_FRAMEWORK_FALLBACK(name)                                     \
    ctx.r3.u64 = *guest::At<guest::u32_be>(ctx.r3.u32 + (offset));   \
  }
// A thunk: r3 = one of the framework's interfaces, `adjust` bytes into the
// framework; the framework's word at `offset`.
#define SBX_FRAMEWORK_THUNK(name, adjust, offset)                                 \
  extern "C" REX_FUNC(name) {                                                     \
    SBX_FRAMEWORK_FALLBACK(name)                                                  \
    ctx.r3.u64 = *guest::At<guest::u32_be>(ctx.r3.u32 - (adjust) + (offset));     \
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

// Where the debug font draws next, and where its lines start.
struct DebugFontPosition {
  guest::u32_be x, y;
  guest::u32_be line_x, line_y;
};
constexpr uint32_t kDebugFontPosition = 0x8296E140;

void SetDebugFontPosition(uint32_t x, uint32_t y) {
  DebugFontPosition* position = guest::At<DebugFontPosition>(kDebugFontPosition);
  position->line_x = x;
  position->line_y = y;
  position->x = x;
  position->y = y;
}

}  // namespace

// DebugText::SetPosition (vtable slot 2): r4, r5 = the x and y words'
// addresses. Leaves r3, r4 = x, y and r11 = the font's position, as the
// original does (its callers may read them).
extern "C" REX_FUNC(sub_82091120) {
  SBX_FRAMEWORK_FALLBACK(sub_82091120)
  const uint32_t x = *guest::At<guest::u32_be>(ctx.r4.u32);
  const uint32_t y = *guest::At<guest::u32_be>(ctx.r5.u32);
  SetDebugFontPosition(x, y);
  ctx.r3.u64 = x;
  ctx.r4.u64 = y;
  ctx.r11.u64 = kDebugFontPosition;
}

// DebugFont::SetPosition: r3, r4 = x, y.
extern "C" REX_FUNC(sub_82571988) {
  SBX_FRAMEWORK_FALLBACK(sub_82571988)
  SetDebugFontPosition(ctx.r3.u32, ctx.r4.u32);
  ctx.r11.u64 = kDebugFontPosition;
}

// --- The framework's and run loop's constructors ---

namespace {

// The framework's 16 interfaces' vtables, in the order they sit in it.
constexpr uint32_t kFrameworkVtables[16] = {
    0x82000B94, 0x82000B90, 0x82000B88, 0x82000B84, 0x82000B80, 0x82000B74,
    0x82000B50, 0x82000B3C, 0x82000B2C, 0x82000B00, 0x82000AF8, 0x82000AF0,
    0x82000AE8, 0x82000AD8, 0x82000AD0, 0x82000ACC,
};
constexpr uint32_t kRunLoopVtable = 0x82000FE0;

struct Framework {
  guest::u32_be interfaces[16];  // Each interface's vtable.
  guest::u32_be unknown_64[5];
  uint8_t unknown_84[4];
  guest::u32_be unknown_88[5];
  uint8_t unknown_108[16];
  guest::u32_be unknown_124[2];
  uint8_t unknown_132;
  uint8_t flag_133;  // Read by App::Run.
  uint8_t flag_134;
};
GUEST_OFFSET(Framework, unknown_124, 124);

struct RunLoop {
  guest::u32_be vtable;
  uint8_t unknown_4[12];  // Two members built by sub_8209A5F8.
  uint8_t unknown_16[16];
  guest::u32_be unknown_32[3];
  uint8_t unknown_44[4];
  uint8_t unknown_48[3];
};

}  // namespace

// RunFrameworkAt: r3 holds the framework's address; it's run (recompiled).
extern "C" REX_FUNC(sub_82090408) {
  SBX_FRAMEWORK_FALLBACK(sub_82090408)
  g.Call(sub_8209ACB8, *guest::At<guest::u32_be>(ctx.r3.u32));
}

// Framework::Framework: r3 the framework. Returns it.
extern "C" REX_FUNC(sub_82090500) {
  SBX_FRAMEWORK_FALLBACK(sub_82090500)
  Framework* framework = guest::At<Framework>(ctx.r3.u32);
  for (int i = 0; i < 16; i++) {
    framework->interfaces[i] = kFrameworkVtables[i];
  }
  for (guest::u32_be& word : framework->unknown_64) {
    word = 0;
  }
  for (guest::u32_be& word : framework->unknown_88) {
    word = 0;
  }
  framework->unknown_124[0] = 0;
  framework->unknown_124[1] = 0;
  framework->unknown_132 = 0;
  framework->flag_133 = 0;
  framework->flag_134 = 0;
}

// RunLoop::RunLoop: r3 the run loop. Returns it.
extern "C" REX_FUNC(sub_8209AAC0) {
  SBX_FRAMEWORK_FALLBACK(sub_8209AAC0)
  RunLoop* loop = guest::At<RunLoop>(ctx.r3.u32);
  loop->vtable = kRunLoopVtable;
  g.Call(sub_8209A5F8, loop->unknown_4);
  g.Call(sub_8209A5F8, loop->unknown_16);
  for (guest::u32_be& word : loop->unknown_32) {
    word = 0;
  }
  for (uint8_t& byte : loop->unknown_48) {
    byte = 0;
  }
  ctx.r3.u64 = guest::AddressOf(loop);
}
