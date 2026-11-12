// The input layer's objects as our own C++ (--native_input, on by default):
// the input processors' and the logical and physical pads' constructors,
// and the pads' values.

#include "game/math.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_input, true, "SBX",
                    "Our own C++ for the input processors' and pads' constructors and the "
                    "pads' values instead of the recompiled functions");

namespace {

using game::Vector;
using guest::AddressOf;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kInputProcessorBaseVtable = 0x820699E0;
constexpr uint32_t kInputProcessorVtable = 0x82068EA4;
constexpr uint32_t kLogicalPadVtable = 0x820699CC;
constexpr uint32_t kPhysicalPadVtable = 0x820699D4;

struct InputProcessor {
  u32_be vtable;
  uint8_t unknown_4[12];
  f32_be unknown_16[2];  // 0.5 each.
  f32_be unknown_24[2];
  Vector points[6];
  uint8_t unknown_128[136];
  u32_be unknown_264[3];
  u32_be unknown_276;  // 20.
  u32_be unknown_280;  // 4.
  uint8_t unknown_284[128];
};
GUEST_OFFSET(InputProcessor, points, 32);
GUEST_OFFSET(InputProcessor, unknown_284, 284);

struct LogicalPad {
  u32_be vtable;
  u32_be unknown_4[8];
  uint8_t values[4];  // The pad's values, as the game reads them.
};
GUEST_OFFSET(LogicalPad, values, 36);

struct PhysicalPad {
  u32_be vtable;
  u32_be unknown_4;
  uint8_t values[160];  // The controller's values.
  f32_be unknown_168;
  u32_be unknown_172[2];
  f32_be unknown_180;
  u32_be unknown_184[2];
};
GUEST_OFFSET(PhysicalPad, unknown_168, 168);

bool On() { return REXCVAR_GET(native_input); }

void ConstructProcessorBase(InputProcessor* processor) {
  processor->vtable = kInputProcessorBaseVtable;
  processor->unknown_16[0] = 0.5f;
  processor->unknown_16[1] = 0.5f;
  processor->unknown_24[0] = 0.0f;
  processor->unknown_24[1] = 0.0f;
  for (Vector& point : processor->points) {
    point.w = 1.0f;
  }
  for (u32_be& word : processor->unknown_264) {
    word = 0;
  }
  processor->unknown_276 = 20;
  processor->unknown_280 = 4;
  for (uint8_t& byte : processor->unknown_284) {
    byte = 0;
  }
}

}  // namespace

#define SBX_INPUT_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();             \
  if (!On()) {                     \
    __imp__##name(ctx, base);      \
    return;                        \
  }                                \
  Guest g(ctx, base);

// InputProcessor::InputProcessor: r3 the processor. Returns it.
extern "C" REX_FUNC(sub_825729F8) {
  SBX_INPUT_FALLBACK(sub_825729F8)
  InputProcessor* processor = At<InputProcessor>(ctx.r3.u32);
  g.Call(sub_82577950, processor);  // InputProcessorBase::InputProcessorBase
  processor->vtable = kInputProcessorVtable;
  ctx.r3.u64 = AddressOf(processor);
}

// InputProcessorBase::InputProcessorBase: r3 the processor. Returns it.
extern "C" REX_FUNC(sub_82577950) {
  SBX_INPUT_FALLBACK(sub_82577950)
  ConstructProcessorBase(At<InputProcessor>(ctx.r3.u32));
}

// LogicalPad::LogicalPad: r3 the pad. Returns it.
extern "C" REX_FUNC(sub_82577370) {
  SBX_INPUT_FALLBACK(sub_82577370)
  LogicalPad* pad = At<LogicalPad>(ctx.r3.u32);
  pad->vtable = kLogicalPadVtable;
  for (u32_be& word : pad->unknown_4) {
    word = 0;
  }
}

// PhysicalPad::PhysicalPad: r3 the pad. Returns it.
extern "C" REX_FUNC(sub_825773E8) {
  SBX_INPUT_FALLBACK(sub_825773E8)
  PhysicalPad* pad = At<PhysicalPad>(ctx.r3.u32);
  pad->vtable = kPhysicalPadVtable;
  pad->unknown_4 = 0;
  pad->unknown_168 = 0.0f;
  pad->unknown_172[0] = 0;
  pad->unknown_172[1] = 0;
  pad->unknown_180 = 0.0f;
  pad->unknown_184[0] = 0;
  pad->unknown_184[1] = 0;
}

// LogicalPad::Values and PhysicalPad::Values: r3 the pad.
extern "C" REX_FUNC(sub_82577368) {
  SBX_INPUT_FALLBACK(sub_82577368)
  ctx.r3.u64 = AddressOf(At<LogicalPad>(ctx.r3.u32)->values);
}

extern "C" REX_FUNC(sub_82582398) {
  SBX_INPUT_FALLBACK(sub_82582398)
  ctx.r3.u64 = AddressOf(At<PhysicalPad>(ctx.r3.u32)->values);
}
