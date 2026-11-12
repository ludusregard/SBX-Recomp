// The C runtime's small functions as our own C++ (--native_c_runtime, on by
// default): the array constructor and destructor loops, wcslen, wcscmp, the
// ASCII case-blind compare and rand.
//
// The heap, the formatters (printf's family), the parsers, the math kernels
// and the copies (memcpy, memset and theirs) stay recompiled.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_c_runtime, true, "SBX",
                    "Our own C++ for the C runtime's small functions (the array constructor "
                    "and destructor loops, wcslen, wcscmp, the ASCII case-blind compare, rand) "
                    "instead of the recompiled functions");

namespace {

using guest::At;
using guest::Guest;
using guest::u16_be;
using guest::u32_be;

// The C runtime's data for a thread (sub_8258FF70 returns the caller's).
struct ThreadData {
  uint8_t unknown_0[20];
  u32_be rand_seed;
};

bool On() { return REXCVAR_GET(native_c_runtime); }

uint32_t ToLowerAscii(uint32_t c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

}  // namespace

// The vector constructor iterator: r3 the first object, r4 an object's
// size, r5 how many, r6 the constructor, called on each in order.
extern "C" REX_FUNC(sub_82091278) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82091278(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const uint32_t first = ctx.r3.u32, size = ctx.r4.u32, constructor = ctx.r6.u32;
  const int32_t count = ctx.r5.s32;
  for (int32_t i = 0; i < count; i++) {
    g.CallAddress(constructor, first + size * i);
  }
}

// The vector destructor iterator: r3 the first object, r4 an object's
// size, r5 how many, r6 the destructor, called on each from the last back.
extern "C" REX_FUNC(sub_820900C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820900C0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const uint32_t first = ctx.r3.u32, size = ctx.r4.u32, destructor = ctx.r6.u32;
  for (int32_t i = ctx.r5.s32 - 1; i >= 0; i--) {
    g.CallAddress(destructor, first + size * i);
  }
}

// wcslen: r3 the text.
extern "C" REX_FUNC(sub_8258F2F8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8258F2F8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const u16_be* text = At<u16_be>(ctx.r3.u32);
  int32_t length = 0;
  while (text[length] != 0) {
    length++;
  }
  ctx.r3.s64 = length;
}

// wcscmp: r3 and r4 the texts; -1, 0 or 1.
extern "C" REX_FUNC(sub_8258D630) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8258D630(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const u16_be* a = At<u16_be>(ctx.r3.u32);
  const u16_be* b = At<u16_be>(ctx.r4.u32);
  while (*a == *b && *b != 0) {
    a++;
    b++;
  }
  ctx.r3.s64 = *a < *b ? -1 : *a > *b ? 1 : 0;
}

// The ASCII case-blind compare of at most r5 bytes of r3 and r4: the
// difference of the first bytes that differ, 0 for none.
extern "C" REX_FUNC(sub_82592128) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82592128(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const uint8_t* a = At<uint8_t>(ctx.r3.u32);
  const uint8_t* b = At<uint8_t>(ctx.r4.u32);
  int32_t difference = 0;
  for (uint32_t left = ctx.r5.u32; left != 0; left--) {
    const uint32_t x = ToLowerAscii(*a++), y = ToLowerAscii(*b++);
    difference = int32_t(x - y);
    if (difference != 0 || x == 0) {
      break;
    }
  }
  ctx.r3.s64 = difference;
}

// rand: the thread's seed stepped; returns its bits 16 to 30.
extern "C" REX_FUNC(sub_8258F558) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8258F558(ctx, base);
    return;
  }
  Guest g(ctx, base);
  ThreadData* thread = At<ThreadData>(g.Call(sub_8258FF70));
  const uint32_t seed = thread->rand_seed * 214013u + 2531011u;
  thread->rand_seed = seed;
  ctx.r3.u64 = (seed >> 16) & 0x7FFF;
}

// --- The C runtime's start-up and handlers ---

namespace {

// The floating-point conversion helpers printf and scanf call (_cfltcvt's
// table), set at start-up.
constexpr uint32_t kFloatConversionTable = 0x8265B73C;
constexpr uint32_t kFloatConversionHelpers[10] = {
    0x825945E8, 0x82593998, 0x82593988, 0x82593990, 0x82593908,
    0x825945E8, 0x82594598, 0x82593928, 0x82593850, 0x825937B0,
};

// Handlers the runtime keeps (set by the functions below, cleared at
// start-up), and their defaults.
constexpr uint32_t kHandler829735D0 = 0x829735D0;
constexpr uint32_t kHandler829735D8 = 0x829735D8;
constexpr uint32_t kHandler8297378C = 0x8297378C;
constexpr uint32_t kHandler82973788 = 0x82973788;
constexpr uint32_t kHandler82973788Default = 0x82598F08;
constexpr uint32_t kHandler8297A760 = 0x8297A760;
constexpr uint32_t kHandler8265B490 = 0x8265B490;
constexpr uint32_t kHandler8265B490Default = 0x8258FD68;

void SetWord(uint32_t address, uint32_t value) { *At<u32_be>(address) = value; }

}  // namespace

// RoutePosition::~RoutePosition (an empty destructor): r3 left as it is.
extern "C" REX_FUNC(sub_821611F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821611F0(ctx, base);
    return;
  }
}

// _cfltcvt_init: the floating-point conversion helpers' table filled.
extern "C" REX_FUNC(sub_8258C530) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8258C530(ctx, base);
    return;
  }
  Guest g(ctx, base);
  for (int i = 0; i < 10; i++) {
    At<u32_be>(kFloatConversionTable)[i] = kFloatConversionHelpers[i];
  }
}

// _init_pointers: the handlers set to their defaults. Returns 0 (done).
extern "C" REX_FUNC(sub_8258FD88) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8258FD88(ctx, base);
    return;
  }
  Guest g(ctx, base);
  SetWord(kHandler829735D0, 0);
  SetWord(kHandler8297378C, 0);
  SetWord(kHandler8297A760, 0);
  SetWord(kHandler829735D8, 0);
  SetWord(kHandler82973788, kHandler82973788Default);
  SetWord(kHandler8265B490, kHandler8265B490Default);
  ctx.r3.u64 = 0;
}

// _vsnprintf: r3 the buffer, r4 its size, r5 the format, r6 the
// arguments; _vsnprintf_l with no locale.
extern "C" REX_FUNC(sub_82590A78) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82590A78(ctx, base);
    return;
  }
  Guest g(ctx, base);
  g.Call(sub_82590980, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, 0u, ctx.r6.u32);
}

// The handlers' setters: r3 the handler. Returns r3.
#define SBX_HANDLER_SETTER(address, global)   \
  extern "C" REX_FUNC(sub_##address) {        \
    REX_FUNC_PROLOGUE();                      \
    if (!On()) {                              \
      __imp__sub_##address(ctx, base);        \
      return;                                 \
    }                                         \
    Guest g(ctx, base);                       \
    SetWord(global, ctx.r3.u32);              \
  }

SBX_HANDLER_SETTER(82592BB8, kHandler829735D0)
SBX_HANDLER_SETTER(82594890, kHandler829735D8)
SBX_HANDLER_SETTER(82595730, kHandler8297A760)
SBX_HANDLER_SETTER(82598FD0, kHandler8297378C)

#undef SBX_HANDLER_SETTER

// The handler at 0x82973788 put back to its default. Returns r3.
extern "C" REX_FUNC(sub_82598FB8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82598FB8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  SetWord(kHandler82973788, kHandler82973788Default);
}
