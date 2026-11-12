// The race set-up's records as our own C++ (--native_race_setup, on by
// default): the racers' car records cleared (settings, decals, options),
// the racers' and the race data's entries reset, the stages and course
// record reset, and the race data's entries made. The race set-up itself
// stays recompiled.

#include "game/race_setup.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_race_setup, true, "SBX",
                    "Our own C++ for the race setup' set-ups, resets, small setters and getters "
                    "instead of the recompiled functions");

namespace {

using game::Car;
using game::CourseRecord;
using game::Decal;
using game::RaceData;
using game::RaceEntries;
using game::RaceEntry;
using game::RaceEntryPart;
using game::RacerRecord;
using game::Racers;
using game::Stages;
using guest::At;
using guest::Guest;

bool On() { return REXCVAR_GET(native_race_setup); }

// Writable: the race data, made at start up.
constexpr uint32_t kRaceData = 0x826B580C;
// RaceEntryPart::RaceEntryPart (sub_82101EA8, ours below).
constexpr uint32_t kRaceEntryPartConstructor = 0x82101EA8;

void Memset(Guest& g, void* to, uint32_t value, uint32_t size) {
  g.Call(sub_8258C950, to, value, size);  // memset
}

void ResetDecal(Decal& decal) {
  decal.scale[0] = 1.0f;
  decal.scale[1] = 1.0f;
  for (auto& v : decal.unknown_8) {
    v = 0.0f;
  }
  decal.unknown_20 = 0;
  decal.unknown_22 = 0;
  decal.unknown_23 = 0;
  decal.image = -1;
  decal.unknown_28 = 0;
}

}  // namespace

#define SBX_RACE_SETUP_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                \
  if (!On()) {                        \
    __imp__##name(ctx, base);         \
    return;                           \
  }                                   \
  Guest g(ctx, base);

// Car::ResetDecals40: r3 the car's 40 decals at +288.
extern "C" REX_FUNC(sub_820CC248) {
  SBX_RACE_SETUP_FALLBACK(sub_820CC248)
  Decal* decals = At<Decal>(ctx.r3.u32);
  for (int i = 0; i < 40; i++) {
    ResetDecal(decals[i]);
  }
}

// Car::ResetDecals6: r3 the car's 6 decals at +96.
extern "C" REX_FUNC(sub_820CC998) {
  SBX_RACE_SETUP_FALLBACK(sub_820CC998)
  Decal* decals = At<Decal>(ctx.r3.u32);
  for (int i = 0; i < 6; i++) {
    ResetDecal(decals[i]);
  }
}

// Car::Reset1568: r3 the car's options (+1568).
extern "C" REX_FUNC(sub_820CCA18) {
  SBX_RACE_SETUP_FALLBACK(sub_820CCA18)
  game::CarOptions* options = At<game::CarOptions>(ctx.r3.u32);
  static constexpr uint8_t kDefaults[11] = {0, 0, 5, 0, 0, 0, 1, 2, 1, 2, 0};
  for (int i = 0; i < 11; i++) {
    options->bytes[i] = kDefaults[i];
  }
  options->unknown_12 = 0;
}

// Car::ResetSettings: r3 the car's settings (+16).
extern "C" REX_FUNC(sub_820CD500) {
  SBX_RACE_SETUP_FALLBACK(sub_820CD500)
  game::CarSettings* settings = At<game::CarSettings>(ctx.r3.u32);
  static constexpr uint8_t kLevels[23] = {2, 2, 2, 2, 3, 7, 0, 0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0};
  for (int i = 0; i < 23; i++) {
    settings->levels[i] = kLevels[i];
  }
  for (auto& v : settings->unknown_0) {
    v = 0;
  }
}

// Car::SetupVehicle: r3 the car, r4 a vehicle; Vehicle::SetupFromCar
// with the two swapped.
extern "C" REX_FUNC(sub_820CD368) {
  SBX_RACE_SETUP_FALLBACK(sub_820CD368)
  const uint32_t car = ctx.r3.u32;
  const uint32_t vehicle = ctx.r4.u32;
  g.Call(sub_820C2608, vehicle, car);  // Vehicle::SetupFromCar
}

// Car::Clear: r3 the car.
extern "C" REX_FUNC(sub_820CD590) {
  SBX_RACE_SETUP_FALLBACK(sub_820CD590)
  Car* car = At<Car>(ctx.r3.u32);
  car->car_id = 0;
  g.Call(sub_820CD3B8, &car->setup, 0u);  // its setup cleared
  car->unknown_2 = 0;
  car->unknown_0 = 0;
  car->unknown_6 = 0;
  car->unknown_8 = 0;
  car->unknown_10 = 0;
  car->unknown_4 = -1;
  car->unknown_11 = 0;
  g.Call(sub_820CD500, &car->settings);    // Car::ResetSettings
  g.Call(sub_820CC248, car->decals_288);  // Car::ResetDecals40
  g.Call(sub_820CC998, car->decals_96);   // Car::ResetDecals6
  g.Call(sub_820CCA18, &car->options);     // Car::Reset1568
  for (game::CarSlot& slot : car->slots) {
    slot.unknown_0 = 0;
    slot.unknown_4 = 0;
    for (auto& v : slot.values) {
      v = 0.0f;
    }
  }
}

// Car::Init: r3 the car.
extern "C" REX_FUNC(sub_820CD378) {
  SBX_RACE_SETUP_FALLBACK(sub_820CD378)
  Car* car = At<Car>(ctx.r3.u32);
  g.Call(sub_820CD590, car);  // Car::Clear
  car->unknown_3888 = 0;
  car->unknown_3892 = 1;
}

// RacerRecord::Init: r3 the record.
extern "C" REX_FUNC(sub_820FF958) {
  SBX_RACE_SETUP_FALLBACK(sub_820FF958)
  RacerRecord* record = At<RacerRecord>(ctx.r3.u32);
  g.Call(sub_820CD378, &record->car);  // Car::Init
  record->unknown_3920 = 0.0f;
  record->unknown_3912 = -1;
  record->unknown_3916 = -1;
  record->unknown_3904 = -1;
  record->unknown_3906 = -1;
  record->unknown_3924 = 0;
}

// Racers::Reset: r3 the racers; each record's RacerRecord::Init.
extern "C" REX_FUNC(sub_820FFAC8) {
  SBX_RACE_SETUP_FALLBACK(sub_820FFAC8)
  Racers* racers = At<Racers>(ctx.r3.u32);
  for (auto& v : racers->unknown_444768) {
    v = 0;
  }
  for (RacerRecord& record : racers->records) {
    g.Call(sub_820FF958, &record);  // RacerRecord::Init
  }
}

// CourseBlock::Clear: r3 the block.
extern "C" REX_FUNC(sub_820FF6A8) {
  SBX_RACE_SETUP_FALLBACK(sub_820FF6A8)
  game::CourseBlock* block = At<game::CourseBlock>(ctx.r3.u32);
  Memset(g, block, 0, 64);
  block->unknown_60 = 0;
  block->unknown_64 = 0;
  block->unknown_72 = 0;
  block->unknown_68 = 0;
}

// CourseRecord::Reset: r3 the record. Its ten slots made a free list.
extern "C" REX_FUNC(sub_82105E20) {
  SBX_RACE_SETUP_FALLBACK(sub_82105E20)
  CourseRecord* course = At<CourseRecord>(ctx.r3.u32);
  g.Call(sub_820FF6A8, &course->block);  // CourseBlock::Clear
  for (int i = 0; i < 10; i++) {
    course->slots[i].index = uint16_t(i);
    course->slots[i].next = int16_t(i + 1);
  }
  course->unknown_40 = 0;
  course->unknown_48 = 0;
  course->slots[9].next = -1;
  course->unknown_44 = -1;
  course->unknown_46 = -1;
  course->unknown_42 = -1;
}

// Stages::Reset: r3 the stages.
extern "C" REX_FUNC(sub_821019B0) {
  SBX_RACE_SETUP_FALLBACK(sub_821019B0)
  Stages* stages = At<Stages>(ctx.r3.u32);
  Memset(g, stages, 0, sizeof(Stages));
  for (game::Stage& stage : stages->stages) {
    stage.unknown_4 = 0;
    stage.unknown_0 = 4;
    stage.unknown_8 = 0;
    for (game::StagePair& pair : stage.pairs) {
      pair.unknown_0 = -1;
      pair.unknown_4 = 2;
    }
    stage.unknown_76 = 0;
  }
  stages->unknown_800 = 0;
  stages->unknown_804 = 0;
  stages->unknown_1220 = 0;
  stages->unknown_808 = 0;
  g.Call(sub_82105E20, &stages->course);  // CourseRecord::Reset
}

// RaceEntryPart::RaceEntryPart: r3 the part.
extern "C" REX_FUNC(sub_82101EA8) {
  SBX_RACE_SETUP_FALLBACK(sub_82101EA8)
  RaceEntryPart* part = At<RaceEntryPart>(ctx.r3.u32);
  part->unknown_792 = 0.0f;
  part->unknown_796 = 0;
}

// RaceEntry::Reset: r3 the entry.
extern "C" REX_FUNC(sub_821020F0) {
  SBX_RACE_SETUP_FALLBACK(sub_821020F0)
  RaceEntry* entry = At<RaceEntry>(ctx.r3.u32);
  entry->unknown_3912 = 0;
  entry->unknown_3908 = 1;
  entry->unknown_0 = 0;
  entry->unknown_3924 = 0;
  entry->unknown_3920 = 0;
  for (RaceEntryPart& part : entry->parts) {
    g.Call(sub_82102038, &part);
    part.unknown_784 = 12000;
    part.unknown_788 = 12000;
  }
  entry->unknown_3932 = 0;
  g.Call(sub_82101B18, entry->unknown_3892);
}

// RaceEntries::Reset: r3 the entries; cleared, then each RaceEntry::Reset.
extern "C" REX_FUNC(sub_82102158) {
  SBX_RACE_SETUP_FALLBACK(sub_82102158)
  RaceEntries* entries = At<RaceEntries>(ctx.r3.u32);
  Memset(g, entries, 0, sizeof(RaceEntries));
  entries->unknown_381792 = 0;
  entries->unknown_381796 = 0;
  for (RaceEntry& entry : entries->entries) {
    g.Call(sub_821020F0, &entry);  // RaceEntry::Reset
  }
}

// RaceData::ResetGlobal: sub_82102278 on the race data.
extern "C" REX_FUNC(sub_82102330) {
  SBX_RACE_SETUP_FALLBACK(sub_82102330)
  g.Call(sub_82102278, *At<guest::u32_be>(kRaceData));
}

// RaceData::RaceData: r3 the race data; each entry's parts made. Returns it.
extern "C" REX_FUNC(sub_82102340) {
  SBX_RACE_SETUP_FALLBACK(sub_82102340)
  RaceData* data = At<RaceData>(ctx.r3.u32);
  for (RaceEntry& entry : data->entries.entries) {
    g.Call(sub_82091278, entry.parts, uint32_t(sizeof(RaceEntryPart)), 4u,
           kRaceEntryPartConstructor);  // The vector constructor iterator.
  }
  ctx.r3.u64 = guest::AddressOf(data);
}
