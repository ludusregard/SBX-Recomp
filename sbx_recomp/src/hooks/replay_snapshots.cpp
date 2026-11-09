// The race's snapshots as our own C++ (--native_replay_snapshots, on by
// default): the copies a race's state goes through when the replay saves
// and restores it (the bodies' states, the cars' parts and members, the
// track positions), field by field.
//
// Written from SBX's generated code: each copy's loads and stores in the
// original's order, merged into runs (words, halfwords and bytes as they
// are), the memcpys in their place; the helpers they call (the track
// position's, the matrix rows', the wheels') inline. The floats are copied
// by their bits. The replay's own save and restore (RaceReplay's four,
// RaceSnapshot::Save, Replay::Keep), the record's and member's that loop
// over counts they read, and Member1136::Copy stay recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_replay_snapshots, true, "SBX",
                    "Our own C++ for the race snapshots' copies (the bodies' states, the cars' "
                    "parts and members, the track positions) instead of the recompiled "
                    "functions; the replay's own save and restore stay recompiled");

namespace {

using guest_memory::CopyBytes;
using guest_memory::CopyHalves;
using guest_memory::CopyWords;

bool On() { return REXCVAR_GET(native_replay_snapshots); }

}  // namespace

// Record5232::RestoreTrack.
extern "C" REX_FUNC(sub_82104480) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82104480(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r4 + 6148, r3 + 0, 8);
  guest_memory::Memcpy(ctx, base, r4 + 6160, r3 + 16, 112);
  CopyHalves(base, r4 + 5232, r3 + 128, 8);
  CopyWords(base, r4 + 5248, r3 + 144, 64);
  CopyHalves(base, r4 + 5312, r3 + 208, 4);
  CopyHalves(base, r4 + 5328, r3 + 224, 8);
  CopyWords(base, r4 + 5344, r3 + 240, 64);
  CopyHalves(base, r4 + 5408, r3 + 304, 4);
  CopyHalves(base, r4 + 5424, r3 + 320, 8);
  CopyWords(base, r4 + 5440, r3 + 336, 64);
  CopyHalves(base, r4 + 5504, r3 + 400, 4);
  CopyHalves(base, r4 + 5520, r3 + 416, 8);
  CopyWords(base, r4 + 5536, r3 + 432, 64);
  CopyHalves(base, r4 + 5600, r3 + 496, 4);
  CopyHalves(base, r4 + 5616, r3 + 512, 8);
  CopyWords(base, r4 + 5632, r3 + 528, 64);
  CopyHalves(base, r4 + 5696, r3 + 592, 4);
  ctx.r3.u64 = r4 + 5616;
}

// BodyState::Restore.
extern "C" REX_FUNC(sub_82103320) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82103320(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyBytes(base, r4 + 0, r3 + 0, 1);
  CopyWords(base, r4 + 4, r3 + 4, 8);
  CopyWords(base, r4 + 16, r3 + 16, 36);
  CopyWords(base, r4 + 64, r3 + 64, 52);
  CopyWords(base, r4 + 128, r3 + 128, 40);
  CopyWords(base, r4 + 176, r3 + 176, 212);
  CopyWords(base, r4 + 400, r3 + 400, 116);
  ctx.r3.u64 = r4 + 496;
}

// Member1136::Restore.
extern "C" REX_FUNC(sub_82102DE8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82102DE8(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r4 + 0, r3 + 0, 56);
  CopyWords(base, r4 + 64, r3 + 64, 208);
  CopyBytes(base, r4 + 276, r3 + 272, 1);
  CopyWords(base, r4 + 280, r3 + 276, 56);
  CopyWords(base, r4 + 348, r3 + 332, 84);
  CopyBytes(base, r4 + 432, r3 + 416, 2);
  CopyWords(base, r4 + 436, r3 + 420, 20);
  CopyWords(base, r4 + 464, r3 + 448, 52);
  CopyWords(base, r4 + 528, r3 + 512, 128);
  CopyBytes(base, r4 + 656, r3 + 640, 4);
  CopyWords(base, r4 + 660, r3 + 644, 116);
  CopyWords(base, r4 + 784, r3 + 768, 16);
  CopyBytes(base, r4 + 800, r3 + 784, 2);
  CopyWords(base, r4 + 804, r3 + 788, 20);
  CopyWords(base, r4 + 832, r3 + 816, 52);
  CopyWords(base, r4 + 896, r3 + 880, 128);
  CopyBytes(base, r4 + 1024, r3 + 1008, 4);
  CopyWords(base, r4 + 1028, r3 + 1012, 116);
  CopyWords(base, r4 + 1152, r3 + 1136, 16);
  CopyBytes(base, r4 + 1168, r3 + 1152, 2);
  CopyWords(base, r4 + 1172, r3 + 1156, 20);
  CopyWords(base, r4 + 1200, r3 + 1184, 52);
  CopyWords(base, r4 + 1264, r3 + 1248, 128);
  CopyBytes(base, r4 + 1392, r3 + 1376, 4);
  CopyWords(base, r4 + 1396, r3 + 1380, 116);
  CopyWords(base, r4 + 1520, r3 + 1504, 16);
  CopyBytes(base, r4 + 1536, r3 + 1520, 2);
  CopyWords(base, r4 + 1540, r3 + 1524, 20);
  CopyWords(base, r4 + 1568, r3 + 1552, 52);
  CopyWords(base, r4 + 1632, r3 + 1616, 128);
  CopyBytes(base, r4 + 1760, r3 + 1744, 4);
  CopyWords(base, r4 + 1764, r3 + 1748, 116);
  CopyWords(base, r4 + 1888, r3 + 1872, 120);
  CopyWords(base, r4 + 2016, r3 + 2000, 120);
  CopyWords(base, r4 + 2176, r3 + 2128, 28);
  guest_memory::Memcpy(ctx, base, r4 + 2204, r3 + 2156, 144);
  CopyWords(base, r4 + 2348, r3 + 2300, 32);
  CopyBytes(base, r4 + 2380, r3 + 2332, 8);
  CopyWords(base, r4 + 2388, r3 + 2340, 12);
  CopyBytes(base, r4 + 2400, r3 + 2352, 2);
  CopyWords(base, r4 + 2404, r3 + 2356, 12);
  CopyWords(base, r4 + 2428, r3 + 2368, 12);
  CopyWords(base, r4 + 2416, r3 + 2380, 8);
  CopyBytes(base, r4 + 2424, r3 + 2388, 1);
  CopyWords(base, r4 + 2440, r3 + 2392, 4);
  CopyBytes(base, r4 + 2444, r3 + 2396, 2);
  CopyWords(base, r4 + 2448, r3 + 2400, 8);
  CopyBytes(base, r4 + 2456, r3 + 2408, 1);
  CopyWords(base, r4 + 2464, r3 + 2416, 40);
  CopyBytes(base, r4 + 2504, r3 + 2456, 1);
  CopyWords(base, r4 + 2508, r3 + 2460, 4);
  CopyBytes(base, r4 + 2512, r3 + 2464, 1);
  CopyWords(base, r4 + 2516, r3 + 2468, 108);
  CopyBytes(base, r4 + 2624, r3 + 2576, 2);
  CopyWords(base, r4 + 2628, r3 + 2580, 36);
  CopyBytes(base, r4 + 2664, r3 + 2616, 1);
  CopyWords(base, r4 + 2924, r3 + 2620, 4);
  CopyBytes(base, r4 + 2928, r3 + 2624, 1);
  CopyWords(base, r4 + 2932, r3 + 2628, 16);
  CopyBytes(base, r4 + 2948, r3 + 2644, 1);
  CopyWords(base, r4 + 2952, r3 + 2648, 8);
  CopyBytes(base, r4 + 2960, r3 + 2656, 1);
  CopyWords(base, r4 + 2964, r3 + 2660, 20);
  guest_memory::Memcpy(ctx, base, r4 + 2984, r3 + 2680, 60);
  guest_memory::Memcpy(ctx, base, r4 + 3044, r3 + 2740, 60);
  CopyWords(base, r4 + 3104, r3 + 2800, 24);
  CopyBytes(base, r4 + 3128, r3 + 2824, 1);
  CopyWords(base, r4 + 3132, r3 + 2828, 24);
  CopyWords(base, r4 + 3168, r3 + 2864, 72);
  CopyBytes(base, r4 + 3240, r3 + 2936, 2);
  ctx.r3.u64 = r4 + 3216;
}

// Part464::Restore.
extern "C" REX_FUNC(sub_82102C30) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82102C30(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r4 + 16, r3 + 352, 20);
  CopyWords(base, r4 + 48, r3 + 384, 68);
  CopyWords(base, r4 + 128, r3 + 0, 32);
  CopyWords(base, r4 + 176, r3 + 32, 132);
  CopyWords(base, r4 + 320, r3 + 176, 20);
  CopyWords(base, r4 + 352, r3 + 208, 20);
  CopyWords(base, r4 + 384, r3 + 240, 20);
  CopyWords(base, r4 + 416, r3 + 272, 20);
  CopyWords(base, r4 + 448, r3 + 304, 20);
  CopyWords(base, r4 + 480, r3 + 336, 16);
  ctx.r3.u64 = r4 + 480;
}

// Record5232::SaveTrack.
extern "C" REX_FUNC(sub_821044F0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821044F0(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r3 + 0, r4 + 6148, 8);
  guest_memory::Memcpy(ctx, base, r3 + 16, r4 + 6160, 112);
  CopyHalves(base, r3 + 128, r4 + 5232, 8);
  CopyWords(base, r3 + 144, r4 + 5248, 64);
  CopyHalves(base, r3 + 208, r4 + 5312, 4);
  CopyHalves(base, r3 + 224, r4 + 5328, 8);
  CopyWords(base, r3 + 240, r4 + 5344, 64);
  CopyHalves(base, r3 + 304, r4 + 5408, 4);
  CopyHalves(base, r3 + 320, r4 + 5424, 8);
  CopyWords(base, r3 + 336, r4 + 5440, 64);
  CopyHalves(base, r3 + 400, r4 + 5504, 4);
  CopyHalves(base, r3 + 416, r4 + 5520, 8);
  CopyWords(base, r3 + 432, r4 + 5536, 64);
  CopyHalves(base, r3 + 496, r4 + 5600, 4);
  CopyHalves(base, r3 + 512, r4 + 5616, 8);
  CopyWords(base, r3 + 528, r4 + 5632, 64);
  CopyHalves(base, r3 + 592, r4 + 5696, 4);
  ctx.r3.u64 = r3 + 512;
}

// BodyState::Save.
extern "C" REX_FUNC(sub_82103B68) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82103B68(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyBytes(base, r3 + 0, r4 + 0, 1);
  CopyWords(base, r3 + 4, r4 + 4, 8);
  CopyWords(base, r3 + 16, r4 + 16, 36);
  CopyWords(base, r3 + 64, r4 + 64, 52);
  CopyWords(base, r3 + 128, r4 + 128, 40);
  CopyWords(base, r3 + 176, r4 + 176, 212);
  CopyWords(base, r3 + 400, r4 + 400, 116);
  ctx.r3.u64 = r3 + 496;
}

// Member1136::Save.
extern "C" REX_FUNC(sub_82103638) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82103638(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r3 + 0, r4 + 0, 56);
  CopyWords(base, r3 + 64, r4 + 64, 208);
  CopyBytes(base, r3 + 272, r4 + 276, 1);
  CopyWords(base, r3 + 276, r4 + 280, 56);
  CopyWords(base, r3 + 332, r4 + 348, 84);
  CopyBytes(base, r3 + 416, r4 + 432, 2);
  CopyWords(base, r3 + 420, r4 + 436, 20);
  CopyWords(base, r3 + 448, r4 + 464, 52);
  CopyWords(base, r3 + 512, r4 + 528, 128);
  CopyBytes(base, r3 + 640, r4 + 656, 4);
  CopyWords(base, r3 + 644, r4 + 660, 116);
  CopyWords(base, r3 + 768, r4 + 784, 16);
  CopyBytes(base, r3 + 784, r4 + 800, 2);
  CopyWords(base, r3 + 788, r4 + 804, 20);
  CopyWords(base, r3 + 816, r4 + 832, 52);
  CopyWords(base, r3 + 880, r4 + 896, 128);
  CopyBytes(base, r3 + 1008, r4 + 1024, 4);
  CopyWords(base, r3 + 1012, r4 + 1028, 116);
  CopyWords(base, r3 + 1136, r4 + 1152, 16);
  CopyBytes(base, r3 + 1152, r4 + 1168, 2);
  CopyWords(base, r3 + 1156, r4 + 1172, 20);
  CopyWords(base, r3 + 1184, r4 + 1200, 52);
  CopyWords(base, r3 + 1248, r4 + 1264, 128);
  CopyBytes(base, r3 + 1376, r4 + 1392, 4);
  CopyWords(base, r3 + 1380, r4 + 1396, 116);
  CopyWords(base, r3 + 1504, r4 + 1520, 16);
  CopyBytes(base, r3 + 1520, r4 + 1536, 2);
  CopyWords(base, r3 + 1524, r4 + 1540, 20);
  CopyWords(base, r3 + 1552, r4 + 1568, 52);
  CopyWords(base, r3 + 1616, r4 + 1632, 128);
  CopyBytes(base, r3 + 1744, r4 + 1760, 4);
  CopyWords(base, r3 + 1748, r4 + 1764, 116);
  CopyWords(base, r3 + 1872, r4 + 1888, 120);
  CopyWords(base, r3 + 2000, r4 + 2016, 120);
  CopyWords(base, r3 + 2128, r4 + 2176, 28);
  guest_memory::Memcpy(ctx, base, r3 + 2156, r4 + 2204, 144);
  CopyWords(base, r3 + 2300, r4 + 2348, 32);
  CopyBytes(base, r3 + 2332, r4 + 2380, 8);
  CopyWords(base, r3 + 2340, r4 + 2388, 12);
  CopyBytes(base, r3 + 2352, r4 + 2400, 2);
  CopyWords(base, r3 + 2356, r4 + 2404, 12);
  CopyWords(base, r3 + 2368, r4 + 2428, 12);
  CopyWords(base, r3 + 2380, r4 + 2416, 8);
  CopyBytes(base, r3 + 2388, r4 + 2424, 1);
  CopyWords(base, r3 + 2392, r4 + 2440, 4);
  CopyBytes(base, r3 + 2396, r4 + 2444, 2);
  CopyWords(base, r3 + 2400, r4 + 2448, 8);
  CopyBytes(base, r3 + 2408, r4 + 2456, 1);
  CopyWords(base, r3 + 2416, r4 + 2464, 40);
  CopyBytes(base, r3 + 2456, r4 + 2504, 1);
  CopyWords(base, r3 + 2460, r4 + 2508, 4);
  CopyBytes(base, r3 + 2464, r4 + 2512, 1);
  CopyWords(base, r3 + 2468, r4 + 2516, 108);
  CopyBytes(base, r3 + 2576, r4 + 2624, 2);
  CopyWords(base, r3 + 2580, r4 + 2628, 36);
  CopyBytes(base, r3 + 2616, r4 + 2664, 1);
  CopyWords(base, r3 + 2620, r4 + 2924, 4);
  CopyBytes(base, r3 + 2624, r4 + 2928, 1);
  CopyWords(base, r3 + 2628, r4 + 2932, 16);
  CopyBytes(base, r3 + 2644, r4 + 2948, 1);
  CopyWords(base, r3 + 2648, r4 + 2952, 8);
  CopyBytes(base, r3 + 2656, r4 + 2960, 1);
  CopyWords(base, r3 + 2660, r4 + 2964, 20);
  guest_memory::Memcpy(ctx, base, r3 + 2680, r4 + 2984, 60);
  guest_memory::Memcpy(ctx, base, r3 + 2740, r4 + 3044, 60);
  CopyWords(base, r3 + 2800, r4 + 3104, 24);
  CopyBytes(base, r3 + 2824, r4 + 3128, 1);
  CopyWords(base, r3 + 2828, r4 + 3132, 24);
  CopyWords(base, r3 + 2864, r4 + 3168, 72);
  CopyBytes(base, r3 + 2936, r4 + 3240, 2);
  ctx.r3.u64 = r3 + 2912;
}

// Part464::Save.
extern "C" REX_FUNC(sub_82103480) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82103480(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r3 + 352, r4 + 16, 20);
  CopyWords(base, r3 + 384, r4 + 48, 68);
  CopyWords(base, r3 + 0, r4 + 128, 32);
  CopyWords(base, r3 + 32, r4 + 176, 132);
  CopyWords(base, r3 + 176, r4 + 320, 20);
  CopyWords(base, r3 + 208, r4 + 352, 20);
  CopyWords(base, r3 + 240, r4 + 384, 20);
  CopyWords(base, r3 + 272, r4 + 416, 20);
  CopyWords(base, r3 + 304, r4 + 448, 20);
  CopyWords(base, r3 + 336, r4 + 480, 16);
  ctx.r3.u64 = r3 + 336;
}

// Record5232::CopyTrack.
extern "C" REX_FUNC(sub_82104560) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82104560(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r3 + 0, r4 + 0, 8);
  guest_memory::Memcpy(ctx, base, r3 + 16, r4 + 16, 112);
  CopyHalves(base, r3 + 128, r4 + 128, 8);
  CopyWords(base, r3 + 144, r4 + 144, 64);
  CopyHalves(base, r3 + 208, r4 + 208, 4);
  CopyHalves(base, r3 + 224, r4 + 224, 8);
  CopyWords(base, r3 + 240, r4 + 240, 64);
  CopyHalves(base, r3 + 304, r4 + 304, 4);
  CopyHalves(base, r3 + 320, r4 + 320, 8);
  CopyWords(base, r3 + 336, r4 + 336, 64);
  CopyHalves(base, r3 + 400, r4 + 400, 4);
  CopyHalves(base, r3 + 416, r4 + 416, 8);
  CopyWords(base, r3 + 432, r4 + 432, 64);
  CopyHalves(base, r3 + 496, r4 + 496, 4);
  CopyHalves(base, r3 + 512, r4 + 512, 8);
  CopyWords(base, r3 + 528, r4 + 528, 64);
  CopyHalves(base, r3 + 592, r4 + 592, 4);
  ctx.r3.u64 = r3 + 0;
}

// BodyState::Copy.
extern "C" REX_FUNC(sub_82103CC8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82103CC8(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyBytes(base, r3 + 0, r4 + 0, 1);
  CopyWords(base, r3 + 4, r4 + 4, 8);
  CopyWords(base, r3 + 16, r4 + 16, 36);
  CopyWords(base, r3 + 64, r4 + 64, 52);
  CopyWords(base, r3 + 128, r4 + 128, 40);
  CopyWords(base, r3 + 176, r4 + 176, 212);
  CopyWords(base, r3 + 400, r4 + 400, 116);
  ctx.r3.u64 = r3 + 0;
}

// Part464::Copy.
extern "C" REX_FUNC(sub_82104330) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82104330(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r3 + 0, r4 + 0, 164);
  CopyWords(base, r3 + 176, r4 + 176, 20);
  CopyWords(base, r3 + 208, r4 + 208, 20);
  CopyWords(base, r3 + 240, r4 + 240, 20);
  CopyWords(base, r3 + 272, r4 + 272, 20);
  CopyWords(base, r3 + 304, r4 + 304, 20);
  CopyWords(base, r3 + 336, r4 + 336, 36);
  CopyWords(base, r3 + 384, r4 + 384, 68);
  ctx.r3.u64 = r3 + 0;
}

// Member4080::CopyShapes.
extern "C" REX_FUNC(sub_821045D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821045D0(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r3 + 0, r4 + 0, 164);
  CopyWords(base, r3 + 176, r4 + 176, 20);
  CopyWords(base, r3 + 208, r4 + 208, 20);
  CopyWords(base, r3 + 240, r4 + 240, 20);
  CopyWords(base, r3 + 272, r4 + 272, 20);
  CopyWords(base, r3 + 304, r4 + 304, 20);
  CopyWords(base, r3 + 336, r4 + 336, 36);
  CopyWords(base, r3 + 384, r4 + 384, 68);
  CopyWords(base, r3 + 464, r4 + 464, 164);
  CopyWords(base, r3 + 640, r4 + 640, 20);
  CopyWords(base, r3 + 672, r4 + 672, 20);
  CopyWords(base, r3 + 704, r4 + 704, 20);
  CopyWords(base, r3 + 736, r4 + 736, 20);
  CopyWords(base, r3 + 768, r4 + 768, 20);
  CopyWords(base, r3 + 800, r4 + 800, 36);
  CopyWords(base, r3 + 848, r4 + 848, 68);
  CopyWords(base, r3 + 928, r4 + 928, 100);
  ctx.r3.u64 = r3 + 0;
}

// Member4080::Copy.
extern "C" REX_FUNC(sub_82104708) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82104708(ctx, base);
    return;
  }
  const uint32_t r3 = ctx.r3.u32;
  const uint32_t r4 = ctx.r4.u32;
  CopyWords(base, r3 + 0, r4 + 0, 4);
  CopyWords(base, r3 + 16, r4 + 16, 164);
  CopyWords(base, r3 + 192, r4 + 192, 20);
  CopyWords(base, r3 + 224, r4 + 224, 20);
  CopyWords(base, r3 + 256, r4 + 256, 20);
  CopyWords(base, r3 + 288, r4 + 288, 20);
  CopyWords(base, r3 + 320, r4 + 320, 20);
  CopyWords(base, r3 + 352, r4 + 352, 36);
  CopyWords(base, r3 + 400, r4 + 400, 68);
  CopyWords(base, r3 + 480, r4 + 480, 164);
  CopyWords(base, r3 + 656, r4 + 656, 20);
  CopyWords(base, r3 + 688, r4 + 688, 20);
  CopyWords(base, r3 + 720, r4 + 720, 20);
  CopyWords(base, r3 + 752, r4 + 752, 20);
  CopyWords(base, r3 + 784, r4 + 784, 20);
  CopyWords(base, r3 + 816, r4 + 816, 36);
  CopyWords(base, r3 + 864, r4 + 864, 68);
  CopyWords(base, r3 + 944, r4 + 944, 100);
  CopyWords(base, r3 + 1056, r4 + 1056, 36);
  CopyWords(base, r3 + 1104, r4 + 1104, 24);
  CopyWords(base, r3 + 1136, r4 + 1136, 16);
  ctx.r3.u64 = r3 + 0;
}
