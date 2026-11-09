// The boot data's set-up as our own C++ (--native_data_load, on by
// default): the boot's load (once) and the objects it creates (the save
// slot, the race data, the defaults, the input recordings, the replay and
// its records, the data tables), their constructors, the defaults' bit
// sets and the loading screen's load.
//
// Written from SBX's generated code. The objects' sizes: the race data
// 827,904 bytes, the input recordings 2,238,976 (+2238992 cleared), the
// replay 1,334,880 (seven 5,232-byte records from +218240, +215372 = 1).
// The heap, the archive, the defaults' own init, the data tables' load and
// the other boot steps stay recompiled.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_data_load, true, "SBX",
                    "Our own C++ for the boot data's set-up (the boot's load, the objects it "
                    "creates and their constructors, the defaults' bit sets) instead of the "
                    "recompiled functions; the heap and the loads stay recompiled");

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

// LoadingScreen::Load (sub_82245750).
// 2 stores
constexpr Fill kLoadingScreen_Load0[] = {
    {0x8280EEB0, kAbsWord, 0x0}, {0x8280EE68, kAbsWord, 0x0},
};

// Part464::Part464 (sub_82104410).
// 20 stores
constexpr Fill kPart464_Part4640[] = {
    {12, kImage, 0x82000A24, 10, 16}, {188, kImage, 0x82000A24, 6, 32}, {396, kImage, 0x82000A24, 4, 16},
};

// Member4080::Member4080 (sub_82104800).
// 46 stores
constexpr Fill kMember4080_Member40800[] = {
    {28, kImage, 0x82000A24, 10, 16}, {204, kImage, 0x82000A24, 6, 32}, {412, kImage, 0x82000A24, 4, 16}, {492, kImage, 0x82000A24, 10, 16},
    {668, kImage, 0x82000A24, 6, 32}, {876, kImage, 0x82000A24, 4, 16}, {956, kImage, 0x82000A24, 2, 16}, {1068, kImage, 0x82000A24, 2, 16},
    {1116, kImage, 0x82000A24, 2, 32},
};

// Member1136::Member1136 (sub_82102A10).
// 63 stores
constexpr Fill kMember1136_Member11360[] = {
    {12, kImage, 0x82000A24, 3, 16}, {76, kImage, 0x82000A24, 2, 32}, {124, kImage, 0x82000A24, 2, 16}, {348, kImage, 0x82000A24, 4, 16},
    {476, kImage, 0x82000A24, 2, 16}, {524, kImage, 0x82000A24, 4, 16}, {748, kImage, 0x82000A24, 2, 96}, {860, kImage, 0x82000A24, 2, 32},
    {908, kImage, 0x82000A24, 3, 16}, {1116, kImage, 0x82000A24, 2, 96}, {1228, kImage, 0x82000A24, 2, 32}, {1276, kImage, 0x82000A24, 3, 16},
    {1484, kImage, 0x82000A24, 2, 96}, {1596, kImage, 0x82000A24, 2, 32}, {1644, kImage, 0x82000A24, 3, 16}, {1852, kImage, 0x82000A24, 2, 32},
    {1900, kImage, 0x82000A24, 5, 16}, {2012, kImage, 0x82000A24, 6, 16}, {2428, kImage, 0x82000A24, 2, 16}, {2492, kImage, 0x82000A24, 6, 16},
    {2876, kImage, 0x82000A24, 4, 16},
};

// Record5232::Record5232 (sub_82104D90).
// 97 stores
constexpr Fill kRecord5232_Record52320[] = {
    {156, kImage, 0x82000A20, 4, 16}, {252, kImage, 0x82000A20, 4, 16}, {348, kImage, 0x82000A20, 4, 16}, {444, kImage, 0x82000A20, 4, 16},
    {540, kImage, 0x82000A20, 4, 16}, {636, kImage, 0x82000A24, 2, 16}, {684, kImage, 0x82000A24, 3, 32}, {764, kImage, 0x82000A24, 2, 32},
    {812, kImage, 0x82000A24, 2, 16}, {972, kImage, 0x82000A24, 2, 16}, {1020, kImage, 0x82000A24, 2, 80}, {1116, kImage, 0x82000A24, 2, 32},
    {1164, kImage, 0x82000A24, 2, 16}, {1212, kImage, 0x82000A24, 2, 32}, {1260, kImage, 0x82000A24, 2, 16}, {1484, kImage, 0x82000A24, 4, 16},
    {1612, kImage, 0x82000A24, 2, 16}, {1660, kImage, 0x82000A24, 4, 16}, {1884, kImage, 0x82000A24, 2, 96}, {1996, kImage, 0x82000A24, 2, 32},
    {2044, kImage, 0x82000A24, 3, 16}, {2252, kImage, 0x82000A24, 2, 96}, {2364, kImage, 0x82000A24, 2, 32}, {2412, kImage, 0x82000A24, 3, 16},
    {2620, kImage, 0x82000A24, 2, 96}, {2732, kImage, 0x82000A24, 2, 32}, {2780, kImage, 0x82000A24, 3, 16}, {2988, kImage, 0x82000A24, 2, 32},
    {3036, kImage, 0x82000A24, 5, 16}, {3148, kImage, 0x82000A24, 6, 16}, {3564, kImage, 0x82000A24, 2, 16}, {3628, kImage, 0x82000A24, 6, 16},
    {4012, kImage, 0x82000A24, 4, 16},
};

bool On() { return REXCVAR_GET(native_data_load); }

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kRaceData = 0x826B580C;
constexpr uint32_t kInputRecordings = 0x826B582C;
constexpr uint32_t kReplay = 0x826B5830;
constexpr uint32_t kBootLoaded = 0x826B586C;

// operator new (sub_8258C000).
uint32_t New(PPCContext& ctx, uint8_t* base, uint32_t bytes) {
  ctx.r3.u64 = bytes;
  sub_8258C000(ctx, base);
  return ctx.r3.u32;
}

void Call(PPCContext& ctx, uint8_t* base, void (*fn)(PPCContext&, uint8_t*)) { fn(ctx, base); }

// A bit set's words cleared and its first `bits` set (bit i at word i /
// 32, 1 << (i % 32)).
void FillBits(uint8_t* base, uint32_t set, uint32_t words, uint32_t bits) {
  for (uint32_t i = 0; i < words; i++) {
    const uint32_t from = i * 32;
    uint32_t word = 0;
    if (bits >= from + 32) {
      word = 0xFFFFFFFFu;
    } else if (bits > from) {
      word = (1u << (bits - from)) - 1;
    }
    StoreU32(base, set + 4 * i, word);
  }
}

}  // namespace

// LoadingScreen::Load.
extern "C" REX_FUNC(sub_82245750) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82245750(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kLoadingScreen_Load0, args);
  ctx.r3.u64 = 0x582Eu;
  ctx.r10.u64 = 0xFFFFFFFF82810000u;
  sub_82245658(ctx, base);
  ctx.r3.u64 = 0x582Du;
  sub_82245658(ctx, base);
  ctx.r3.u64 = 0x582Fu;
  sub_82245658(ctx, base);
}

// InputRecordings::InputRecordings.
extern "C" REX_FUNC(sub_82105618) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105618(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = object + 2162880;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2168112;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2173344;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2178576;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2183808;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2189040;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2194272;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2202368;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2207600;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2212832;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2218064;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2223296;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2228528;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object + 2233760;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = object;
}

// Replay826D8520::StaticInit.
extern "C" REX_FUNC(sub_825BF238) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_825BF238(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = 0xFFFFFFFF826FD440u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF826FE8B0u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF826FFD20u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF82701190u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF82702600u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF82703A70u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF82704EE0u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF825C0510u;
  sub_8258C7E8(ctx, base);
}

// Replay82816E50::StaticInit.
extern "C" REX_FUNC(sub_825BF340) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_825BF340(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  ctx.r3.u64 = 0xFFFFFFFF828452D0u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF82846740u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF82847BB0u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF82849020u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF8284A490u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF8284B900u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF8284CD70u;
  sub_82104D90(ctx, base);
  ctx.r3.u64 = 0xFFFFFFFF825C0550u;
  sub_8258C7E8(ctx, base);
}

// Part464::Part464.
extern "C" REX_FUNC(sub_82104410) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82104410(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kPart464_Part4640, args);
  ctx.r3.u64 = object;
}

// Member4080::Member4080.
extern "C" REX_FUNC(sub_82104800) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82104800(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kMember4080_Member40800, args);
  ctx.r3.u64 = object;
}

// Member1136::Member1136.
extern "C" REX_FUNC(sub_82102A10) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82102A10(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kMember1136_Member11360, args);
  ctx.r3.u64 = object;
}

// Record5232::Record5232.
extern "C" REX_FUNC(sub_82104D90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82104D90(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kRecord5232_Record52320, args);
  ctx.r3.u64 = object + 4080;
  sub_82104800(ctx, base);
  ctx.r3.u64 = object;
}


// Bits224::Fill (7 words, 199 bits) and Bits480::Fill (15 words, 454).
extern "C" REX_FUNC(sub_820CCAB0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820CCAB0(ctx, base);
    return;
  }
  FillBits(base, ctx.r3.u32, 7, 199);
}

extern "C" REX_FUNC(sub_820CCB08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820CCB08(ctx, base);
    return;
  }
  FillBits(base, ctx.r3.u32, 15, 454);
}

// RaceData::Create: the race data made (sub_82102340) into [0x826B580C],
// or 0.
extern "C" REX_FUNC(sub_82102390) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82102390(ctx, base);
    return;
  }
  const uint32_t data = New(ctx, base, 827904);
  if (data == 0) {
    StoreU32(base, kRaceData, 0);
    return;
  }
  ctx.r3.u64 = data;
  sub_82102340(ctx, base);
  StoreU32(base, kRaceData, ctx.r3.u32);
}

// InputRecordings::Create: made into [0x826B582C] (0 when the allocation
// fails), and its +2238992 cleared.
extern "C" REX_FUNC(sub_82105678) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82105678(ctx, base);
    return;
  }
  uint32_t recordings = New(ctx, base, 2239008);
  if (recordings != 0) {
    ctx.r3.u64 = recordings;
    sub_82105618(ctx, base);
    recordings = ctx.r3.u32;
  }
  StoreU32(base, kInputRecordings, recordings);
  StoreU32(base, recordings + 2238992, 0);
  ctx.r3.u64 = recordings;
}

// Replay::Create: made with its seven records (Record5232::Record5232)
// into [0x826B5830] (0 when the allocation fails), and its +215372 = 1.
extern "C" REX_FUNC(sub_821056D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821056D0(ctx, base);
    return;
  }
  const uint32_t replay = New(ctx, base, 1334880);
  if (replay != 0) {
    for (uint32_t i = 0; i < 7; i++) {
      ctx.r3.u64 = replay + 218240 + 5232 * i;
      sub_82104D90(ctx, base);
    }
  }
  StoreU32(base, kReplay, replay);
  StoreU32(base, replay + 215372, 1);
}

// BootData::CreateObjects: save slot 0, the race data, sub_82102330, the
// defaults, the input recordings, the replay, the data tables' init and
// load.
extern "C" REX_FUNC(sub_821059D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821059D8(ctx, base);
    return;
  }
  ctx.r3.u64 = 0;
  Call(ctx, base, sub_82105938);
  Call(ctx, base, sub_82102390);
  Call(ctx, base, sub_82102330);
  Call(ctx, base, sub_820CCB60);
  Call(ctx, base, sub_82105678);
  Call(ctx, base, sub_821056D0);
  Call(ctx, base, sub_82116F18);
  Call(ctx, base, sub_821173A8);
}

// BootData::Load: once (0x826B586C), the boot's steps between two calls of
// sub_822927E0 with the original's 128-byte frame's +80.
extern "C" REX_FUNC(sub_821067A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_821067A0(ctx, base);
    return;
  }
  if (LoadU32(base, kBootLoaded) != 0) {
    return;
  }
  StoreU32(base, kBootLoaded, 1);
  guest::StackFrame frame(ctx, 128);
  const uint32_t local = frame.Local(80);
  ctx.r3.u64 = local;
  Call(ctx, base, sub_822927E0);
  Call(ctx, base, sub_820A0A88);
  Call(ctx, base, sub_8210AF20);
  Call(ctx, base, sub_821059D8);
  Call(ctx, base, sub_82245750);
  Call(ctx, base, sub_8228E6E0);
  Call(ctx, base, sub_820C9700);
  Call(ctx, base, sub_820CC048);
  ctx.r3.u64 = local;
  Call(ctx, base, sub_822927E0);
}
