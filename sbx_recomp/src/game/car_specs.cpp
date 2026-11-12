// The car parameter tables as our own C++ (--native_car_specs, on by
// default): assets 72 to 83, loaded once at boot into one block and
// byte-swapped by the loads' callbacks, then read by the getters all
// through the game; and Car::Refresh, which builds a car's working copy of
// its spec, tires, aero parts and wheels from the tables and its settings.
// Tire::InchUp (the tire's size and grip for the inch-ups, a long float
// sequence with SBX's own constants), the archive's TOC and memcpy stay
// recompiled, called from here.
//
// SBX's car record has its settings 4 bytes earlier than ITC-Recomp's from
// +1652 on, so Car::Refresh and its helpers are written from SBX's code.

#include "game/archive.h"
#include "game/car_spec.h"
#include "game/race_setup.h"

#include <rex/cvar.h>

#include <bit>

REXCVAR_DEFINE_BOOL(native_car_specs, true, "SBX",
                    "Our own C++ for the car parameter tables (their loads' byte swaps, the "
                    "getters, the aero parts) and Car::Refresh instead of the recompiled "
                    "functions; Tire::InchUp and memcpy stay recompiled");

namespace {

using game::AeroPart;
using game::AssetSize;
using game::AeroPart36;
using game::Car;
using game::CarParams;
using game::CarSetup;
using game::CarSpec;
using game::Paint;
using game::SpecBlock1;
using game::SpecBlock2;
using game::SpecVector;
using game::SwapBytes;
using game::Tire;
using game::SpecWheel;
using game::WheelColor;
using guest::At;
using guest::AddressOf;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kParams = 0x826B2B28;  // [it] the parameter object (writable)

CarParams* Params() { return At<CarParams>(*At<u32_be>(kParams)); }

// --- The swaps -------------------------------------------------------------
//
// The originals swap each word through a float register (lfs, stfs to the
// stack, the bytes stored reversed), which on the 360 keeps every word's
// bits as they are. The bytes are swapped here, which does the same. The
// recompiled round trip doesn't quite: a word that reads as a signaling NaN
// comes out quieted, so with the flag off such a word differs from the
// 360's. Each returns what the original leaves in r3 (a guest address).

uint32_t SwapVector(SpecVector& v) {  // sub_820C94F0
  SwapBytes(v.xyz);
  v.w = 1.0f;
  return AddressOf(&v);
}

// Wheel::SwapColor (sub_820CB2D0).
void SwapColor(WheelColor& color) {
  SwapBytes(color.unknown_0);
  SwapBytes(color.unknown_4);
  SwapBytes(color.unknown_8);
}

// Wheel::Swap (sub_820CB328): its two colors and four words; r3 = the
// second color.
uint32_t SwapWheel(SpecWheel& wheel) {
  SwapColor(wheel.colors[0]);
  SwapColor(wheel.colors[1]);
  SwapBytes(wheel.unknown_32);
  SwapBytes(wheel.model);
  SwapBytes(wheel.unknown_44);
  return AddressOf(&wheel.colors[1]);
}

// Paint::Swap (sub_820CB1F8): each paint's five halves, and its second
// shade by its type; then the word after.
void SwapPaints(game::SpecPaints& paints) {
  for (Paint& paint : paints.paints) {
    SwapBytes(paint.type);
    SwapBytes(paint.hue);
    SwapBytes(paint.unknown_8);
    SwapBytes(paint.shade2_hue);
    SwapBytes(paint.unknown_16);
    if (paint.type == Paint::kSolid) {
      paint.shade2_hue = 0;
      paint.shade2_saturation = 0;
      paint.shade2_value = 25;
    } else if (paint.type == Paint::kTwoShade) {
      paint.shade2_hue = uint16_t(paint.hue);
      paint.shade2_saturation = uint8_t(int32_t(paint.saturation) * 60 / 100);
      paint.shade2_value = 204;
    }
  }
  SwapBytes(paints.unknown_268);
}

// sub_820CA2A0: r3 = its words at +368.
uint32_t SwapBlock1(SpecBlock1& block) {
  for (SpecVector& v : block.vectors) {
    SwapVector(v);
  }
  SwapBytes(block.matrices[0]);  // sub_820C9CB0
  SwapBytes(block.matrices[1]);
  SwapBytes(block.words_224);
  SwapBytes(block.words_244);
  SwapBytes(block.block_264.words_0);  // sub_820CA098
  SwapBytes(block.block_264.words_16);
  SwapBytes(block.block_264.words_32);
  SwapBytes(block.words_356);
  SwapBytes(block.words_368);  // sub_820C9F38
  SwapBytes(block.words_528);
  return AddressOf(block.words_368);
}

// sub_820CBC10: r3 = its words at +300.
uint32_t SwapBlock2(SpecBlock2& block) {
  SwapBytes(block.block_8.words_0);  // sub_820CACC0
  for (auto& v : block.block_8.zeros) {
    v = 0.0f;
  }
  SwapBytes(block.block_8.word_56);
  SwapBytes(block.words_72);
  SwapBytes(block.block_132.words_0);  // sub_820CAE60
  for (auto& group : block.block_132.groups) {
    SwapBytes(group);
  }
  SwapBytes(block.block_132.words_100);
  SwapBytes(block.words_300);
  return AddressOf(block.words_300);
}

// CarSpec::Swap (sub_820CBF08): the two blocks, 16 engine stages, 21 front
// and 21 rear tires, the paints, the stock wheel.
uint32_t SwapCarSpec(CarSpec& spec) {
  SwapBlock1(spec.block1);
  SwapBlock2(spec.block2);
  for (game::EngineStage& stage : spec.stages) {
    SwapBytes(stage.words);  // EngineStage::Swap
  }
  for (int i = 0; i < 21; i++) {
    for (Tire* tire : {&spec.front_tires[i], &spec.rear_tires[i]}) {  // Tire::Swap
      SwapBytes(tire->words_8);
      SwapBytes(tire->width);
      SwapBytes(tire->words_52);
    }
  }
  SwapPaints(spec.paints);
  return SwapWheel(spec.stock_wheel);
}

// The archive's TOC (sub_8209D988), for asset `id`.
uint32_t Toc(Guest& g, uint32_t id) { return g.Call(sub_8209D988, id); }

// --- The getters -----------------------------------------------------------

CarSpec* GetCarSpec(uint32_t id) { return guest::Element<CarSpec>(Params()->specs, id); }

// Wheel number n (1 up), or none.
SpecWheel* GetWheel(int32_t n) {
  if (n < 1) {
    return nullptr;
  }
  return guest::Element<SpecWheel>(Params()->wheels, uint32_t(n) - 1);
}

// The spec's tire: type 0 is record 0, else type x 5 + grade - 5.
Tire* GetTire(CarSpec* spec, int32_t type, uint32_t grade, bool rear) {
  const uint32_t index = type == 0 ? 0 : uint32_t(type) * 5 + grade - 5;
  return guest::Element<Tire>(AddressOf(rear ? spec->rear_tires : spec->front_tires), index);
}

// The car's parts tables: tables 74 to 82, at car `car`'s records.
void GetCarParts(uint32_t car, uint32_t parts[CarParams::kPartTables]) {
  static constexpr uint32_t kRecordBytes[CarParams::kPartTables] = {280, 280, 280, 280, 168,
                                                                    168, 84,  108, 72};
  CarParams* params = Params();
  for (int i = 0; i < CarParams::kPartTables; i++) {
    parts[i] = params->parts[i] + kRecordBytes[i] * car;
  }
}

// --- The aero parts --------------------------------------------------------

void Memcpy(Guest& g, void* to, uint32_t from, uint32_t size) {
  g.Call(sub_8258CB90, to, from, size);
}

// AeroParts::Get74 to Get82: the record for `level` (1 up) of table
// `table` copied to `out` and swapped (AeroPart::Swap, sub_820CB468, and
// Swap36, sub_820CB588); with no level, the defaults.
struct AeroGetter {
  int table;
  bool wide;           // The 36-byte parts (tables 81 and 82).
  bool positive_only;  // Get77: a level above 0 (ble), the others not 0.
};
constexpr AeroGetter kGet74{0, false, false}, kGet75{1, false, false}, kGet76{2, false, false},
    kGet77{3, false, true}, kGet78{4, false, false}, kGet79{5, false, false},
    kGet80{6, false, false}, kGet81{7, true, false}, kGet82{8, true, false};

void GetAeroPart(Guest& g, const AeroGetter& getter, uint32_t out,
                 const uint32_t parts[CarParams::kPartTables], int32_t level) {
  const uint32_t size = getter.wide ? sizeof(AeroPart36) : sizeof(AeroPart);
  AeroPart36* part = At<AeroPart36>(out);  // Its last two words only for the wide ones.
  const bool use = getter.positive_only ? level > 0 : level != 0;
  if (use) {
    Memcpy(g, part, parts[getter.table] + size * uint32_t(level) - size, size);
    SwapBytes(part->part.values);
    SwapBytes(part->part.scales);
    SwapBytes(part->part.unknown_24);
    if (getter.wide) {
      SwapBytes(part->unknown_28);
      SwapBytes(part->unknown_32);
    }
    return;
  }
  for (auto& v : part->part.values) {
    v = 0.0f;
  }
  part->part.scales[0] = 1.0f;
  part->part.scales[1] = 1.0f;
  part->part.unknown_24 = 0;
  if (getter.wide) {
    part->unknown_28 = 0;
    part->unknown_32 = 0;
  }
}

// --- Car::Refresh ------------------------------------------------------------

// Tire::InchUp (sub_820C44C8): the tire, the inches.
void InchUp(Guest& g, Tire* tire, int32_t inches) { g.Call(sub_820C44C8, tire, inches); }

// Car::CopySpec (sub_820C4698): the spec's blocks, its engine stage, paints
// and the two tires into the car, the tires inched up, and how far that
// moved each tire's width.
void CopySpec(Guest& g, CarSetup& car, CarSpec* spec) {
  car.front_width_change = 0.0f;
  car.rear_width_change = 0.0f;
  Memcpy(g, &car.block1, AddressOf(&spec->block1), sizeof(SpecBlock1));
  Memcpy(g, &car.block2, AddressOf(&spec->block2), sizeof(SpecBlock2));
  Memcpy(g, &car.engine,
         AddressOf(spec->stages) + uint32_t(sizeof(game::EngineStage)) * uint32_t(car.engine_stage),
         sizeof(game::EngineStage));
  Memcpy(g, &car.paints, AddressOf(&spec->paints), sizeof(game::SpecPaints));
  Memcpy(g, &car.front_tire,
         AddressOf(GetTire(spec, car.front_tire_type, car.front_tire_grade, false)), sizeof(Tire));
  Memcpy(g, &car.rear_tire,
         AddressOf(GetTire(spec, car.rear_tire_type, car.rear_tire_grade, true)), sizeof(Tire));
  const float front = car.front_tire.width;
  const float rear = car.rear_tire.width;
  InchUp(g, &car.front_tire, car.front_inch_up);
  InchUp(g, &car.rear_tire, car.rear_inch_up);
  car.front_width_change = car.front_tire.width - front;
  car.rear_width_change = car.rear_tire.width - rear;
}

// Car::CopyAeroParts (sub_820C4378): each part by its level. The original
// builds each in its frame and copies it in; the result in the car is the
// same.
void CopyAeroParts(Guest& g, CarSetup& car, const uint32_t parts[CarParams::kPartTables]) {
  const auto& level = car.aero_levels;
  const int32_t level_77 = level[3] == -1 ? 0 : int32_t(level[3]);
  GetAeroPart(g, kGet74, AddressOf(&car.aero_74), parts, level[0]);
  GetAeroPart(g, kGet75, AddressOf(&car.aero_75), parts, level[1]);
  GetAeroPart(g, kGet76, AddressOf(&car.aero_76), parts, level[2]);
  GetAeroPart(g, kGet77, AddressOf(&car.aero_77), parts, level_77);
  GetAeroPart(g, kGet80, AddressOf(&car.aero_80), parts, level[6]);
  GetAeroPart(g, kGet81, AddressOf(&car.aero_81), parts, level[9]);
  GetAeroPart(g, kGet82, AddressOf(&car.aero_82), parts, level[10]);
  GetAeroPart(g, level[5] == 0 ? kGet78 : kGet79, AddressOf(&car.aero_78), parts, level[4]);
}

// A wheel: the spec's stock one for 0, else table 73's.
uint32_t WheelOf(int32_t n, CarSpec* spec) {
  return n == 0 ? AddressOf(&spec->stock_wheel) : AddressOf(GetWheel(n));
}

}  // namespace

#define SBX_CAR_SPECS_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                   \
  if (!REXCVAR_GET(native_car_specs)) {  \
    __imp__##name(ctx, base);            \
    return;                              \
  }                                      \
  Guest g(ctx, base);

// --- The loads' callbacks (through Archive::Load's function pointer): r6 =
// the parameter object. They turn flush-to-zero off, as the originals' lfs
// leave it.

// Table 72: the spec count, then each spec swapped (Params::SwapCarSpecs,
// sub_820CBF90: the count and table read from the global each time).
extern "C" REX_FUNC(sub_820CBFE8) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBFE8)
  CarParams* object = At<CarParams>(ctx.r6.u32);
  uint32_t r3 = Toc(g, 72);
  ctx.fpscr.disableFlushMode();
  object->spec_count = AssetSize(r3, 72) / uint32_t(sizeof(CarSpec));
  for (int32_t i = 0; i < int32_t(Params()->spec_count); i++) {
    r3 = SwapCarSpec(*GetCarSpec(uint32_t(i)));
  }
  object->loads_pending -= 1;
  ctx.r3.u64 = r3;
}

// Table 73: the wheel count, each wheel swapped (Params::SwapWheels,
// sub_820CBBB0), each record's model up by one, and each model's first
// record kept.
extern "C" REX_FUNC(sub_820CBE48) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBE48)
  CarParams* object = At<CarParams>(ctx.r6.u32);
  uint32_t r3 = Toc(g, 73);
  ctx.fpscr.disableFlushMode();
  object->wheel_count = AssetSize(r3, 73) / uint32_t(sizeof(SpecWheel));
  for (int32_t i = 0; i < int32_t(Params()->wheel_count); i++) {
    SpecWheel* wheel = guest::Element<SpecWheel>(Params()->wheels, uint32_t(i));
    r3 = AddressOf(wheel);
    if (wheel != nullptr) {
      r3 = SwapWheel(*wheel);
    }
  }
  int32_t model = 1;
  object->first_wheel_of_model[1] = 0;
  for (int32_t i = 0; i < int32_t(object->wheel_count); i++) {
    SpecWheel* wheel = guest::Element<SpecWheel>(object->wheels, uint32_t(i));
    wheel->model += 1;
    if (wheel->model != model) {
      model = wheel->model;
      *guest::Element<u32_be>(AddressOf(object->first_wheel_of_model), uint32_t(model)) = uint32_t(i);
    }
  }
  object->model_count = uint32_t(model + 1);
  object->loads_pending -= 1;
  ctx.r3.u64 = r3;
}

// Tables 74 to 82: nothing to swap here (their getters swap a copy).
extern "C" REX_FUNC(sub_820C9B90) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9B90)
  At<CarParams>(ctx.r6.u32)->loads_pending -= 1;
}

// Table 83: its 40-byte records swapped (sub_820CB3F8).
extern "C" REX_FUNC(sub_820CBB30) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBB30)
  CarParams* object = At<CarParams>(ctx.r6.u32);
  struct Record83 {
    u32_be words[10];
  };
  uint32_t r3 = Toc(g, 83);
  ctx.fpscr.disableFlushMode();
  const uint32_t count = AssetSize(r3, 83) / uint32_t(sizeof(Record83));
  for (uint32_t i = 0; int32_t(count) > 0 && i < count; i++) {
    Record83* record = guest::Element<Record83>(object->table_83, i);
    SwapBytes(record->words);
    r3 = AddressOf(record);
  }
  object->loads_pending -= 1;
  ctx.r3.u64 = r3;
}

// --- The getters

// Params::GetCarSpec: r3 the car id.
extern "C" REX_FUNC(sub_820C9C38) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9C38)
  ctx.r3.u64 = AddressOf(GetCarSpec(ctx.r3.u32));
}

// Params::GetWheel: r3 the wheel number; 0 below 1.
extern "C" REX_FUNC(sub_820C9C50) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9C50)
  ctx.r3.u64 = AddressOf(GetWheel(ctx.r3.s32));
}

// Params::FirstWheelOfModel: r3 the model; its first record + 1, or 0.
extern "C" REX_FUNC(sub_820C9C80) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9C80)
  const int32_t model = ctx.r3.s32;
  if (model < 1) {
    ctx.r3.u64 = 0;
    return;
  }
  const u32_be* first = guest::Element<u32_be>(AddressOf(Params()->first_wheel_of_model), uint32_t(model));
  ctx.r3.u64 = uint32_t(*first + 1);
}

// Params::Exists.
extern "C" REX_FUNC(sub_820C9B78) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9B78)
  ctx.r3.u64 = Params() != nullptr ? 1 : 0;
}

// CarSpec::GetTire: r3 the spec, r4 the type, r5 the grade, r6 rear.
extern "C" REX_FUNC(sub_820C96C8) {
  SBX_CAR_SPECS_FALLBACK(sub_820C96C8)
  // Worked out as guest addresses (the spec isn't read).
  const uint32_t spec = ctx.r3.u32;
  const int32_t type = ctx.r4.s32;
  const uint32_t index = type == 0 ? 0 : uint32_t(type) * 5 + ctx.r5.u32 - 5;
  const uint32_t tires = ctx.r6.s32 == 0 ? offsetof(CarSpec, front_tires) : offsetof(CarSpec, rear_tires);
  ctx.r3.u64 = uint32_t(index * sizeof(Tire) + spec + tires);
}

// Params::GetCarParts: r3 the 9 words out (left in r3), r4 the car id.
extern "C" REX_FUNC(sub_820C9BA0) {
  SBX_CAR_SPECS_FALLBACK(sub_820C9BA0)
  uint32_t parts[CarParams::kPartTables];
  GetCarParts(ctx.r4.u32, parts);
  u32_be* out = At<u32_be>(ctx.r3.u32);
  for (int i = 0; i < CarParams::kPartTables; i++) {
    out[i] = parts[i];
  }
}

// The aero part getters: r3 the part out (left in r3), r4 the parts (9
// pointers), r5 the level.
#define SBX_AERO_GETTER(name, getter)                     \
  extern "C" REX_FUNC(name) {                             \
    SBX_CAR_SPECS_FALLBACK(name)                          \
    const uint32_t out = ctx.r3.u32;                      \
    const u32_be* words = At<u32_be>(ctx.r4.u32);         \
    uint32_t parts[CarParams::kPartTables];               \
    for (int i = 0; i < CarParams::kPartTables; i++) {    \
      parts[i] = words[i];                                \
    }                                                     \
    const int32_t level = ctx.r5.s32;                     \
    ctx.fpscr.disableFlushMode();                         \
    GetAeroPart(g, getter, out, parts, level);            \
    ctx.r3.u64 = out;                                     \
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

// r3 the parts: how many of the last parts table's (+32) first two
// records (the body kits) have a first float other than 0.0. The original copies each to its frame
// and swaps it there; only that float is read.
extern "C" REX_FUNC(sub_820CBAC0) {
  SBX_CAR_SPECS_FALLBACK(sub_820CBAC0)
  ctx.fpscr.disableFlushMode();
  const u32_be* parts = At<u32_be>(ctx.r3.u32);
  const AeroPart36* kits = At<AeroPart36>(parts[kGet82.table]);
  uint32_t count = 0;
  for (int i = 0; i < 2; i++) {
    const uint32_t raw = kits[i].part.values[0].raw;  // Little-endian in the table.
    if (std::bit_cast<float>(raw) != 0.0f) {
      count++;
    }
  }
  ctx.r3.u64 = count;
}

// Car::Refresh: r3 the car. Its spec (by its id at +12), the parts of that
// car (of car 0 when the spec says it has none of its own), the aero parts,
// the two wheels. r3 as the original leaves it: the last memcpy's.
extern "C" REX_FUNC(sub_820C47A8) {
  SBX_CAR_SPECS_FALLBACK(sub_820C47A8)
  ctx.fpscr.disableFlushMode();
  Car* car = At<Car>(ctx.r3.u32);
  CarSetup& setup = car->setup;
  CarSpec* spec = GetCarSpec(car->car_id);
  CopySpec(g, setup, spec);
  uint32_t parts[CarParams::kPartTables];
  GetCarParts(spec->block2.uses_car0_parts != 0 ? 0 : uint32_t(car->car_id), parts);
  CopyAeroParts(g, setup, parts);
  Memcpy(g, &setup.front_wheel_record, WheelOf(setup.front_wheel, spec), sizeof(SpecWheel));
  Memcpy(g, &setup.rear_wheel_record, WheelOf(setup.rear_wheel, spec), sizeof(SpecWheel));
}
