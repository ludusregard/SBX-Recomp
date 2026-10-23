// Tick rate experiments.
//
// The game never measures elapsed time: each pass of App::Run (sub_8209A8F0)
// advances a fixed step, and the loop is paced by a 2-vblank present
// interval, so it ticks 30 times a second (see "Fixed time step" in
// docs/functions.md). Two switches, both off by default:
//
// --tick_rate_60: the present interval is 1 vblank (sub_82573100 below), so
// the main loop, and with it all game logic, runs 60 times a second. Play
// time, counted in ticks at 30 a second, counts every other tick
// (sub_82107A38, in subtick/per_frame_logic.cpp).
//
// Parked: --subtick (src/subtick/) keeps the game's 30 Hz tick instead (the
// original replays depend on it). These stay for a true tick rate upgrade
// later; see "Tick rate experiments" in docs/functions.md.
//
// --tick_rate_half_step: the step lengths are halved once the XEX is loaded:
// 1/30 -> 1/60 at 0x820029C4 (an .rdata float 8 functions load; see "Fixed
// time step" in docs/functions.md) and the cars' step length at 0x825F17E8 (a
// .data float, 1/60: Vehicle::Tick steps each car twice a tick by it; see
// "Vehicle stepping"). The 1/60 at 0x820029C0 isn't touched: two of its four
// users are HSV to RGB conversions (hue in degrees / 60), and halving it
// swapped the car paint colors.
//
// With only --tick_rate_half_step, whatever runs at half speed is driven by
// these step lengths; with both, whatever still runs at twice the speed isn't.
// --tick_rate_half_step_mask picks which are halved (bit 0 = 1/30, bit 2 =
// the cars' step), to find which one changes what.

#include "tick_rate/tick_rate.h"

#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(tick_rate_60, false, "SBX",
                    "Experimental: run the main loop (and all game logic) at 60 ticks a "
                    "second instead of 30 by presenting every vblank");
REXCVAR_DEFINE_BOOL(tick_rate_half_step, false, "SBX",
                    "Experimental: halve the fixed step lengths (1/30 and the cars' step) "
                    "at load");
REXCVAR_DEFINE_INT32(tick_rate_half_step_mask, 5, "SBX",
                     "Which step lengths --tick_rate_half_step halves: bit 0 = 1/30, bit 2 = "
                     "the cars' step");

namespace tick_rate {

namespace {

// Float bit patterns, as stored (big-endian).
struct StepConstant {
  uint32_t mask_bit;
  uint32_t address;
  uint32_t original;
  uint32_t patched;
  const char* name;
};

constexpr StepConstant kStepConstants[] = {
    {1, 0x820029C4, 0x3D088889, 0x3C888889, "1/30 -> 1/60"},
    {4, 0x825F17E8, 0x3C888889, 0x3C088889, "car step 1/60 -> 1/120"},
};

// .rdata, mapped read-only; .data is writable as it is.
constexpr uint32_t kReadOnlyDataStart = 0x82000600;
constexpr uint32_t kReadOnlyDataEnd = 0x820741E4;

// The present interval Renderer::Init (sub_82093280) asks for: 2 vblanks.
constexpr uint32_t kOriginalVblankInterval = 2;

uint32_t LoadBigEndianU32(const uint8_t* p) {
  uint32_t value;
  std::memcpy(&value, p, 4);
  return __builtin_bswap32(value);
}

void StoreBigEndianU32(uint8_t* p, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(p, &value, 4);
}

}  // namespace

void PatchStepConstants(rex::memory::Memory* memory) {
  if (!REXCVAR_GET(tick_rate_half_step)) {
    return;
  }
  uint32_t mask = uint32_t(REXCVAR_GET(tick_rate_half_step_mask));
  for (const StepConstant& constant : kStepConstants) {
    if (!(mask & constant.mask_bit)) {
      continue;
    }
    uint8_t* host = memory->TranslateVirtual<uint8_t*>(constant.address);
    uint32_t found = LoadBigEndianU32(host);
    if (found != constant.original) {
      REXLOG_WARN("tick_rate_half_step: {:08X} is {:08X}, expected {:08X}; not patched",
                  constant.address, found, constant.original);
      continue;
    }
    bool read_only =
        constant.address >= kReadOnlyDataStart && constant.address < kReadOnlyDataEnd;
    if (!read_only) {
      StoreBigEndianU32(host, constant.patched);
      REXLOG_INFO("tick_rate_half_step: {:08X} {}", constant.address, constant.name);
      continue;
    }
    // .rdata is mapped read-only (rexglue-sdk's XexModule::Load): make the
    // page writable for the write, then put it back.
    rex::memory::BaseHeap* heap = memory->LookupHeap(constant.address);
    uint32_t old_protect = 0;
    if (!heap || !heap->Protect(constant.address, 4,
                                rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite,
                                &old_protect)) {
      REXLOG_ERROR("tick_rate_half_step: couldn't make {:08X} writable; not patched",
                   constant.address);
      continue;
    }
    StoreBigEndianU32(host, constant.patched);
    heap->Protect(constant.address, 4, old_protect);
    REXLOG_INFO("tick_rate_half_step: {:08X} {}", constant.address, constant.name);
  }
}

}  // namespace tick_rate

// sub_82573100: r3 = the video output, r6 = the vblank interval, stored at
// +0x130 (0 and 1 store 1, 2 stores 2, 4 stores 3, 0x80000000 stores 0).
// VideoOutput::WaitForVblank (sub_82573638) waits that many vblanks before
// each present. Called once, from Renderer::Init, with 2.
extern "C" REX_FUNC(sub_82573100) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(tick_rate_60) && ctx.r6.u32 == tick_rate::kOriginalVblankInterval) {
    ctx.r6.u64 = 1;
    REXLOG_INFO("tick_rate_60: vblank interval 2 -> 1");
  }
  __imp__sub_82573100(ctx, base);
}
