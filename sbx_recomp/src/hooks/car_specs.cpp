// The car parameter tables as our own C++ (--native_car_specs, on by
// default): assets 72 to 83, loaded once at boot into one block and
// byte-swapped by the loads' callbacks, then read by the getters all
// through the game; and Car::Refresh, which builds a car's working copy of
// its spec, tires, aero parts and wheels from the tables and its settings.
//
// Written from SBX's generated code, with ITC-Recomp's car_specs as the
// reference. The swaps, the callbacks and the getters are graded A (the same
// instructions), and their C++ follows ITC's. The car's own fields aren't:
// SBX's car struct has its settings 4 bytes earlier than ITC's from +1652 on
// (the engine stage at +1652, ITC +1656; the tire types and grades at +1708
// to +1720; the inch-ups at +1728 and +1736; the wheels at +1724 and +1732;
// the aero levels at +1740 to +1780), so Car::Refresh, Car::CopySpec and
// Car::CopyAeroParts here are written from SBX's code. Tire::InchUp (the
// tire's size and grip for the inch-ups, a long float sequence with SBX's
// own constants) and memcpy stay recompiled, called from here.
//
// The parameter object at [0x826B2B28] (468 bytes): +0 the loads pending,
// +8 table 73 (the wheels, 48 bytes a record), +12 table 72 (the specs,
// 8432 bytes each), +16 to +48 tables 74 to 82 (the parts), +52 table 83,
// +56 the spec count, +60 the wheel count, +64 + 4 x model the first record
// of each wheel model, +464 the last model + 1.

#include "guest.h"

#include <rex/cvar.h>

#include <bit>

REXCVAR_DEFINE_BOOL(native_car_specs, true, "SBX",
                    "Our own C++ for the car parameter tables (their loads' byte swaps, the "
                    "getters, the aero parts) and Car::Refresh instead of the recompiled "
                    "functions; Tire::InchUp and memcpy stay recompiled");

namespace {

constexpr uint32_t kParams = 0x826B2B28;
constexpr uint32_t kSpecBytes = 8432;
constexpr uint32_t kWheelBytes = 48;
constexpr uint32_t kOne = 0x82000A20;   // 1.0f
constexpr uint32_t kZero = 0x82000A24;  // 0.0f

uint32_t Params(uint8_t* base) { return guest::LoadU32(base, kParams); }

// --- The swaps -------------------------------------------------------------
//
// The originals swap each word through a float register (lfs, stfs to the
// stack, the bytes stored reversed), which on the 360 keeps every word's
// bits as they are. The integer is swapped here, which does the same. The
// recompiled round trip doesn't quite: a word that reads as a signaling
// NaN comes out quieted (its 0x00400000 bit set), so with the flag off such
// a word differs from the 360's. Each returns what the original leaves in
// r3.

void SwapWords(uint8_t* base, uint32_t address, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    const uint32_t a = address + 4 * i;
    guest::StoreU32(base, a, std::byteswap(guest::LoadU32(base, a)));
  }
}

void SwapHalf(uint8_t* base, uint32_t address) {
  guest::StoreU16(base, address, std::byteswap(guest::LoadU16(base, address)));
}

// sub_820C94F0: three words, then +12 = 1.0.
uint32_t SwapVector(uint8_t* base, uint32_t p) {
  const uint32_t one = guest::LoadU32(base, kOne);
  SwapWords(base, p, 3);
  guest::StoreU32(base, p + 12, one);
  return p;
}

uint32_t SwapMatrix(uint8_t* base, uint32_t p) { SwapWords(base, p, 16); return p; }  // sub_820C9CB0
uint32_t Swap5(uint8_t* base, uint32_t p) { SwapWords(base, p, 5); return p; }        // sub_820CAD90
uint32_t Swap3(uint8_t* base, uint32_t p) { SwapWords(base, p, 3); return p; }        // sub_820CA220
uint32_t Swap4(uint8_t* base, uint32_t p) { SwapWords(base, p, 4); return p; }        // sub_820CB150

// Wheel::SwapColor (sub_820CB2D0): two halves and a word.
uint32_t SwapColor(uint8_t* base, uint32_t p) {
  SwapHalf(base, p);
  SwapHalf(base, p + 4);
  SwapWords(base, p + 8, 1);
  return p;
}

uint32_t SwapRecord83(uint8_t* base, uint32_t p) { SwapWords(base, p, 10); return p; }  // sub_820CB3F8

// EngineStage::Swap (sub_820CA780): +8 to +196; +0 to +7 and +200 left.
uint32_t SwapEngineStage(uint8_t* base, uint32_t p) { SwapWords(base, p + 8, 48); return p; }

// Tire::Swap (sub_820CAA48): +8 to +88; +0 to +7 left.
uint32_t SwapTire(uint8_t* base, uint32_t p) { SwapWords(base, p + 8, 21); return p; }

uint32_t SwapBlock1At368(uint8_t* base, uint32_t p) { SwapWords(base, p, 40); return p; }  // sub_820C9F38

// sub_820CACC0: five words, +20 to +52 = 0.0, +56.
uint32_t SwapBlock2At8(uint8_t* base, uint32_t p) {
  const uint32_t zero = guest::LoadU32(base, kZero);
  SwapWords(base, p, 5);
  for (uint32_t i = 0; i < 9; i++) {
    guest::StoreU32(base, p + 20 + 4 * i, zero);
  }
  SwapWords(base, p + 56, 1);
  return p;
}

// Paint::Swap (sub_820CB1F8): 13 paints of 20 bytes from +8, five halves
// each, and the second shade (+12 to +15) by the paint's type (+0); then +268.
uint32_t SwapPaints(uint8_t* base, uint32_t p) {
  for (uint32_t slot = p + 8, n = 0; n < 13; n++, slot += 20) {
    for (uint32_t offset = 0; offset <= 16; offset += 4) {
      SwapHalf(base, slot + offset);
    }
    const uint16_t type = guest::LoadU16(base, slot);
    if (type == 0) {
      guest::StoreU16(base, slot + 12, 0);
      guest::StoreU8(base, slot + 14, 0);
      guest::StoreU8(base, slot + 15, 25);
    } else if (type == 1) {
      const int32_t saturation = guest::LoadU8(base, slot + 6);
      guest::StoreU16(base, slot + 12, guest::LoadU16(base, slot + 4));
      guest::StoreU8(base, slot + 14, uint8_t(saturation * 60 / 100));
      guest::StoreU8(base, slot + 15, 204);
    }
  }
  SwapWords(base, p + 268, 1);
  return p;
}

// Wheel::Swap (sub_820CB328): its two colors, +32 to +47; r3 = p + 20.
uint32_t SwapWheel(uint8_t* base, uint32_t p) {
  SwapColor(base, p + 8);
  SwapColor(base, p + 20);
  SwapWords(base, p + 32, 4);
  return p + 20;
}

// sub_820CA098: r3 = p + 16.
uint32_t SwapBlock1At264(uint8_t* base, uint32_t p) {
  Swap3(base, p);
  Swap3(base, p + 16);
  SwapWords(base, p + 32, 15);
  return p + 16;
}

// sub_820CA2A0: r3 = p + 368.
uint32_t SwapBlock1(uint8_t* base, uint32_t p) {
  for (uint32_t offset = 16; offset <= 80; offset += 16) {
    SwapVector(base, p + offset);
  }
  SwapMatrix(base, p + 96);
  SwapMatrix(base, p + 160);
  Swap5(base, p + 224);
  Swap5(base, p + 244);
  SwapBlock1At264(base, p + 264);
  Swap3(base, p + 356);
  SwapBlock1At368(base, p + 368);
  SwapWords(base, p + 528, 28);
  return p + 368;
}

// sub_820CAE60: r3 = the last byte its loop loaded (the one at +124).
uint32_t SwapBlock2At132(uint8_t* base, uint32_t p) {
  SwapWords(base, p, 10);
  for (uint32_t offset = 40; offset < 100; offset += 20) {
    Swap5(base, p + offset);
  }
  SwapWords(base, p + 100, 17);
  return guest::LoadU8(base, p + 124);
}

// sub_820CBC10: r3 = p + 300.
uint32_t SwapBlock2(uint8_t* base, uint32_t p) {
  SwapBlock2At8(base, p + 8);
  SwapWords(base, p + 72, 14);
  SwapBlock2At132(base, p + 132);
  Swap4(base, p + 300);
  return p + 300;
}

// CarSpec::Swap (sub_820CBF08): the two blocks, 16 engine stages, 21 front
// and 21 rear tires, the paints, the stock wheel; r3 = p + 8400.
uint32_t SwapCarSpec(uint8_t* base, uint32_t p) {
  SwapBlock1(base, p);
  SwapBlock2(base, p + 640);
  for (uint32_t i = 0; i < 16; i++) {
    SwapEngineStage(base, p + 980 + 204 * i);
  }
  for (uint32_t i = 0; i < 21; i++) {
    SwapTire(base, p + 4244 + 92 * i);
    SwapTire(base, p + 6176 + 92 * i);
  }
  SwapPaints(base, p + 8108);
  return SwapWheel(base, p + 8380);
}

// An asset's size from its TOC entry (+16 x id): the unpacked size at +24,
// or the stored one at +20 when that's 0.
uint32_t AssetSize(uint8_t* base, uint32_t toc, uint32_t id) {
  const uint32_t unpacked = guest::LoadU32(base, toc + 16 * id + 24);
  return unpacked != 0 ? unpacked : guest::LoadU32(base, toc + 16 * id + 20);
}

// The archive's TOC (sub_8209D988, recompiled), for asset `id`.
uint32_t Toc(PPCContext& ctx, uint8_t* base, uint32_t id) {
  ctx.r3.u64 = id;
  sub_8209D988(ctx, base);
  return ctx.r3.u32;
}

void CountDown(uint8_t* base, uint32_t object) {
  guest::StoreU32(base, object, guest::LoadU32(base, object) - 1);
}

// --- The getters -----------------------------------------------------------

uint32_t GetCarSpec(uint8_t* base, uint32_t id) {
  return guest::LoadU32(base, Params(base) + 12) + kSpecBytes * id;
}

// Wheel number n (1 up), or 0.
uint32_t GetWheel(uint8_t* base, int32_t n) {
  if (n < 1) {
    return 0;
  }
  return guest::LoadU32(base, Params(base) + 8) + kWheelBytes * uint32_t(n) - kWheelBytes;
}

// The spec's tire record: type 0 is record 0, else type x 5 + grade - 5;
// front tires from +4244, rear from +6176, 92 bytes each.
uint32_t GetTire(uint32_t spec, int32_t type, uint32_t grade, int32_t rear) {
  const uint32_t index = type == 0 ? 0 : uint32_t(type) * 5 + grade - 5;
  return index * 92 + spec + (rear == 0 ? 4244 : 6176);
}

// The car's parts tables: tables 74 to 82, offset for car `car` (their
// records 280, 280, 280, 280, 168, 168, 84, 108 and 72 bytes).
void GetCarParts(uint8_t* base, uint32_t car, uint32_t parts[9]) {
  static constexpr uint32_t kStrides[9] = {280, 280, 280, 280, 168, 168, 84, 108, 72};
  const uint32_t params = Params(base);
  for (uint32_t i = 0; i < 9; i++) {
    parts[i] = guest::LoadU32(base, params + 16 + 4 * i) + kStrides[i] * car;
  }
}

// --- The aero parts --------------------------------------------------------

// AeroPart::Swap (sub_820CB468), seven words, and Swap36 (sub_820CB588),
// nine.
void SwapAeroPart(uint8_t* base, uint32_t p, uint32_t size) {
  SwapWords(base, p, size / 4);
}

void Memcpy(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t from, uint32_t size) {
  ctx.r3.u64 = to;
  ctx.r4.u64 = from;
  ctx.r5.u64 = size;
  sub_8258CB90(ctx, base);
}

// AeroParts::Get74 to Get82: the record for `level` (1 up) of table `table`
// copied to `out` and swapped; with no level, the defaults (+0 to +12 0.0,
// +16 and +20 1.0, +24 and for the 36-byte records +28 and +32 zero).
void AeroPart(PPCContext& ctx, uint8_t* base, uint32_t out, const uint32_t parts[9],
              int32_t level, uint32_t table, uint32_t size, bool use) {
  if (use) {
    Memcpy(ctx, base, out, parts[table] + size * uint32_t(level) - size, size);
    SwapAeroPart(base, out, size);
    return;
  }
  const uint32_t zero = guest::LoadU32(base, kZero);
  const uint32_t one = guest::LoadU32(base, kOne);
  for (uint32_t offset = 0; offset < 16; offset += 4) {
    guest::StoreU32(base, out + offset, zero);
  }
  guest::StoreU32(base, out + 16, one);
  guest::StoreU32(base, out + 20, one);
  guest::StoreU32(base, out + 24, 0);
  if (size == 36) {
    guest::StoreU32(base, out + 28, 0);
    guest::StoreU32(base, out + 32, 0);
  }
}

struct AeroGetter {
  uint32_t table, size;
  bool positive_only;  // Get77: a level above 0 (ble), the others not 0
};
// By SBX's addresses.
constexpr AeroGetter kGet74{0, 28, false}, kGet75{1, 28, false}, kGet76{2, 28, false},
    kGet77{3, 28, true}, kGet78{4, 28, false}, kGet79{5, 28, false}, kGet80{6, 28, false},
    kGet81{7, 36, false}, kGet82{8, 36, false};

void Get(PPCContext& ctx, uint8_t* base, const AeroGetter& g, uint32_t out,
         const uint32_t parts[9], int32_t level) {
  AeroPart(ctx, base, out, parts, level, g.table, g.size,
           g.positive_only ? level > 0 : level != 0);
}

// --- Car::Refresh, SBX's car layout ---------------------------------------

int32_t CarWord(uint8_t* base, uint32_t car, uint32_t offset) {
  return int32_t(guest::LoadU32(base, car + offset));
}

// Tire::InchUp (sub_820C44C8, recompiled): r3 = the tire, r4 = the inches.
void InchUp(PPCContext& ctx, uint8_t* base, uint32_t tire, int32_t inches) {
  ctx.r3.u64 = tire;
  ctx.r4.u64 = uint64_t(int64_t(inches));
  sub_820C44C8(ctx, base);
}

// Car::CopySpec (sub_820C4698): the spec's blocks, its engine stage, paints
// and the two tires into the car, the tires inched up, and how much that
// moved each tire's +48 (its width) kept at +3876 and +3880.
void CopySpec(PPCContext& ctx, uint8_t* base, uint32_t car, uint32_t spec) {
  const uint32_t zero = guest::LoadU32(base, kZero);
  guest::StoreU32(base, car + 3876, zero);
  guest::StoreU32(base, car + 3880, zero);
  Memcpy(ctx, base, car + 2240, spec, 640);
  Memcpy(ctx, base, car + 1888, spec + 640, 340);
  Memcpy(ctx, base, car + 2880, guest::LoadU32(base, car + 1652) * 204 + spec + 980, 204);
  Memcpy(ctx, base, car + 3364, spec + 8108, 272);
  Memcpy(ctx, base, car + 3084,
         GetTire(spec, CarWord(base, car, 1708), guest::LoadU32(base, car + 1712), 0), 92);
  Memcpy(ctx, base, car + 3176,
         GetTire(spec, CarWord(base, car, 1716), guest::LoadU32(base, car + 1720), 1), 92);
  const float front = guest::LoadFloat(base, car + 3132);
  const float rear = guest::LoadFloat(base, car + 3224);
  InchUp(ctx, base, car + 3084, CarWord(base, car, 1728));
  InchUp(ctx, base, car + 3176, CarWord(base, car, 1736));
  guest::StoreFloat(base, car + 3876, guest::LoadFloat(base, car + 3132) - front);
  guest::StoreFloat(base, car + 3880, guest::LoadFloat(base, car + 3224) - rear);
}

// Car::CopyAeroParts (sub_820C4378): each part by its level. The original
// builds each in its frame and copies it in; the result in the car is the
// same.
void CopyAeroParts(PPCContext& ctx, uint8_t* base, uint32_t car, const uint32_t parts[9]) {
  auto level = [&](uint32_t offset) { return CarWord(base, car, offset); };
  Get(ctx, base, kGet74, car + 3636, parts, level(1740));
  Get(ctx, base, kGet75, car + 3664, parts, level(1744));
  Get(ctx, base, kGet76, car + 3692, parts, level(1748));
  Get(ctx, base, kGet77, car + 3720, parts, level(1752) == -1 ? 0 : level(1752));
  Get(ctx, base, kGet80, car + 3776, parts, level(1764));
  Get(ctx, base, kGet81, car + 3804, parts, level(1776));
  Get(ctx, base, kGet82, car + 3840, parts, level(1780));
  Get(ctx, base, level(1760) == 0 ? kGet78 : kGet79, car + 3748, parts, level(1756));
}

// A wheel: the spec's stock one for 0, else table 73's.
uint32_t WheelOf(uint8_t* base, uint32_t car, uint32_t offset, uint32_t spec) {
  const int32_t n = CarWord(base, car, offset);
  return n == 0 ? spec + 8380 : GetWheel(base, n);
}

}  // namespace

#define SBX_CAR_SPECS_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                   \
  if (!REXCVAR_GET(native_car_specs)) {  \
    __imp__##name(ctx, base);            \
    return;                              \
  }

// --- The loads' callbacks (through Archive::Load's function pointer): r6 =
// the parameter object. They turn flush-to-zero off, as the originals' lfs
// leave it.

// Table 72: the spec count, then each spec swapped.
extern "C" REX_FUNC(sub_820CBFE8) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBFE8)
  const uint32_t object = ctx.r6.u32;
  uint32_t r3 = Toc(ctx, base, 72);
  ctx.fpscr.disableFlushMode();
  guest::StoreU32(base, object + 56, AssetSize(base, r3, 72) / kSpecBytes);
  // Params::SwapCarSpecs (sub_820CBF90): the count and table read from the
  // global each time.
  for (int32_t i = 0; i < int32_t(guest::LoadU32(base, Params(base) + 56)); i++) {
    r3 = SwapCarSpec(base, guest::LoadU32(base, Params(base) + 12) + kSpecBytes * uint32_t(i));
  }
  CountDown(base, object);
  ctx.r3.u64 = r3;
}

// Table 73: the wheel count, each wheel swapped, each record's model (+40)
// up by one, and each model's first record at +64 + 4 x model.
extern "C" REX_FUNC(sub_820CBE48) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBE48)
  const uint32_t object = ctx.r6.u32;
  uint32_t r3 = Toc(ctx, base, 73);
  ctx.fpscr.disableFlushMode();
  guest::StoreU32(base, object + 60, AssetSize(base, r3, 73) / kWheelBytes);
  // Params::SwapWheels (sub_820CBBB0).
  for (int32_t i = 0; i < int32_t(guest::LoadU32(base, Params(base) + 60)); i++) {
    r3 = guest::LoadU32(base, Params(base) + 8) + kWheelBytes * uint32_t(i);
    if (r3 != 0) {
      r3 = SwapWheel(base, r3);
    }
  }
  uint32_t model = 1;
  guest::StoreU32(base, object + 68, 0);
  for (int32_t i = 0; i < int32_t(guest::LoadU32(base, object + 60)); i++) {
    const uint32_t record = guest::LoadU32(base, object + 8) + kWheelBytes * uint32_t(i);
    const uint32_t m = guest::LoadU32(base, record + 40) + 1;
    guest::StoreU32(base, record + 40, m);
    if (m != model) {
      model = m;
      guest::StoreU32(base, object + 4 * (m + 16), uint32_t(i));
    }
  }
  guest::StoreU32(base, object + 464, model + 1);
  CountDown(base, object);
  ctx.r3.u64 = r3;
}

// Tables 74 to 82: nothing to swap here (their getters swap a copy).
extern "C" REX_FUNC(sub_820C9B90) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9B90)
  CountDown(base, ctx.r6.u32);
}

// Table 83: its 40-byte records swapped.
extern "C" REX_FUNC(sub_820CBB30) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBB30)
  const uint32_t object = ctx.r6.u32;
  uint32_t r3 = Toc(ctx, base, 83);
  ctx.fpscr.disableFlushMode();
  const uint32_t count = AssetSize(base, r3, 83) / 40;
  if (int32_t(count) > 0) {
    for (uint32_t i = 0; i < count; i++) {
      r3 = SwapRecord83(base, i * 40 + guest::LoadU32(base, object + 52));
    }
  }
  CountDown(base, object);
  ctx.r3.u64 = r3;
}

// --- The getters: r3 a word.

// Params::GetCarSpec: r3 = the car id.
extern "C" REX_FUNC(sub_820C9C38) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9C38)
  ctx.r3.u64 = GetCarSpec(base, ctx.r3.u32);
}

// Params::GetWheel: r3 = the wheel number.
extern "C" REX_FUNC(sub_820C9C50) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9C50)
  ctx.r3.u64 = GetWheel(base, ctx.r3.s32);
}

// Params::FirstWheelOfModel: r3 = the model; its first record + 1, or 0.
extern "C" REX_FUNC(sub_820C9C80) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9C80)
  const int32_t m = ctx.r3.s32;
  ctx.r3.u64 = m < 1 ? 0 : uint32_t(guest::LoadU32(base, Params(base) + 4 * (uint32_t(m) + 16)) + 1);
}

// Params::Exists.
extern "C" REX_FUNC(sub_820C9B78) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9B78)
  ctx.r3.u64 = Params(base) != 0 ? 1 : 0;
}

// CarSpec::GetTire: r3 = the spec, r4 = the type, r5 = the grade, r6 =
// rear.
extern "C" REX_FUNC(sub_820C96C8) {
  SBX_CAR_SPECS_FALLBACK(sub_820C96C8)
  ctx.r3.u64 = GetTire(ctx.r3.u32, ctx.r4.s32, ctx.r5.u32, ctx.r6.s32);
}

// Params::GetCarParts: r3 = out (9 words, left in r3), r4 = the car id.
extern "C" REX_FUNC(sub_820C9BA0) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9BA0)
  uint32_t parts[9];
  GetCarParts(base, ctx.r4.u32, parts);
  for (uint32_t i = 0; i < 9; i++) {
    guest::StoreU32(base, ctx.r3.u32 + 4 * i, parts[i]);
  }
}

// The aero part getters: r3 = out (left in r3), r4 = the parts (9
// pointers), r5 = the level.
#define SBX_AERO_GETTER(name, getter)                          \
  extern "C" REX_FUNC(name) {                                  \
    SBX_CAR_SPECS_FALLBACK(name)                               \
    const uint32_t out = ctx.r3.u32;                           \
    uint32_t parts[9];                                         \
    for (uint32_t i = 0; i < 9; i++) {                         \
      parts[i] = guest::LoadU32(base, ctx.r4.u32 + 4 * i);     \
    }                                                          \
    const int32_t level = ctx.r5.s32;                          \
    ctx.fpscr.disableFlushMode();                              \
    Get(ctx, base, getter, out, parts, level);                 \
    ctx.r3.u64 = out;                                          \
  }

SBX_AERO_GETTER(sub_820CB5E8, kGet74)
SBX_AERO_GETTER(sub_820CB670, kGet75)
SBX_AERO_GETTER(sub_820CB6F8, kGet76)
SBX_AERO_GETTER(sub_820CB780, kGet77)
SBX_AERO_GETTER(sub_820CB808, kGet78)
SBX_AERO_GETTER(sub_820CB890, kGet79)
SBX_AERO_GETTER(sub_820CB918, kGet80)
SBX_AERO_GETTER(sub_820CB9A0, kGet81)
SBX_AERO_GETTER(sub_820CBA30, kGet82)

// sub_820CBAC0: r3 = the parts; how many of table 81's first two records
// (the body kits) have a first float other than 0.0. The original copies
// each to its frame and swaps it there; only that float is read.
extern "C" REX_FUNC(sub_820CBAC0) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBAC0)
  ctx.fpscr.disableFlushMode();
  const float zero = guest::LoadFloat(base, kZero);
  const uint32_t table = guest::LoadU32(base, ctx.r3.u32 + 32);
  uint32_t count = 0;
  for (uint32_t offset = 0; offset < 72; offset += 36) {
    const uint32_t word = std::byteswap(guest::LoadU32(base, table + offset));
    if (std::bit_cast<float>(word) != zero) {
      count++;
    }
  }
  ctx.r3.u64 = count;
}

// Car::Refresh: r3 = the car. Its spec (by its id at +12), the parts of
// that car (of car 0 when the spec's byte at +771 says it has none of its
// own), the aero parts, the two wheels (+1724 and +1732). r3 as the
// original leaves it: the last memcpy's.
extern "C" REX_FUNC(sub_820C47A8) {
  SBX_CAR_SPECS_FALLBACK(sub_820C47A8)
  ctx.fpscr.disableFlushMode();
  const uint32_t car = ctx.r3.u32;
  const uint32_t spec = GetCarSpec(base, guest::LoadU32(base, car + 12));
  CopySpec(ctx, base, car, spec);
  uint32_t parts[9];
  GetCarParts(base, guest::LoadU8(base, spec + 771) != 0 ? 0 : guest::LoadU32(base, car + 12),
              parts);
  CopyAeroParts(ctx, base, car, parts);
  Memcpy(ctx, base, car + 3268, WheelOf(base, car, 1724, spec), 48);
  Memcpy(ctx, base, car + 3316, WheelOf(base, car, 1732, spec), 48);
}
