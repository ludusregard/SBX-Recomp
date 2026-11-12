// The race set-up's records: each racer's car record (its settings and
// decals), the stages and course record, and the race data's entries.

#pragma once

#include "game/car_spec.h"
#include "game/guest_types.h"

namespace game {

using guest::f32_be;
using guest::i16_be;
using guest::i32_be;
using guest::u16_be;
using guest::u32_be;

// One of a car record's decals. A reset gives it a scale of 1 and no
// image (-1).
struct Decal {
  f32_be scale[2];
  f32_be unknown_8[3];
  u16_be unknown_20;
  uint8_t unknown_22;
  uint8_t unknown_23;
  i32_be image;
  u32_be unknown_28;
};
GUEST_SIZE(Decal, 32);
GUEST_OFFSET(Decal, image, 24);

// A car record's settings (at +16).
struct CarSettings {
  u16_be unknown_0[9];
  uint8_t unknown_18[38];
  uint8_t levels[23];  // +56: 2, 2, 2, 2, 3, 7, then 0s, 10 and 0 after a reset.
  uint8_t unknown_79;
};
GUEST_SIZE(CarSettings, 80);
GUEST_OFFSET(CarSettings, levels, 56);

// Eleven bytes and a word (at +1568).
struct CarOptions {
  uint8_t bytes[11];
  uint8_t unknown_11;
  u32_be unknown_12;
};
GUEST_SIZE(CarOptions, 16);

struct CarSlot {
  u32_be unknown_0;
  u32_be unknown_4;
  f32_be values[3];
};
GUEST_SIZE(CarSlot, 20);

// A car's settings and the working copy of its spec that Car::Refresh
// builds from them (cleared by sub_820CD3B8).
struct CarSetup {
  uint8_t unknown_0[8];
  i32_be engine_stage;
  uint8_t unknown_12[52];
  i32_be front_tire_type;
  u32_be front_tire_grade;
  i32_be rear_tire_type;
  u32_be rear_tire_grade;
  i32_be front_wheel;  // 0: the spec's stock wheel; else a wheel number.
  i32_be front_inch_up;
  i32_be rear_wheel;
  i32_be rear_inch_up;
  i32_be aero_levels[11];
  uint8_t unknown_140[104];
  SpecBlock2 block2;
  uint8_t unknown_584[12];
  SpecBlock1 block1;
  EngineStage engine;
  Tire front_tire;
  Tire rear_tire;
  SpecWheel front_wheel_record;
  SpecWheel rear_wheel_record;
  SpecPaints paints;
  AeroPart aero_74;
  AeroPart aero_75;
  AeroPart aero_76;
  AeroPart aero_77;
  AeroPart aero_78;  // Table 78 or 79, by aero level 5.
  AeroPart aero_80;
  AeroPart36 aero_81;
  AeroPart36 aero_82;
  f32_be front_width_change;  // How far the inch-ups moved each tire's width.
  f32_be rear_width_change;
  uint8_t unknown_2240[4];
};
GUEST_SIZE(CarSetup, 2244);
GUEST_OFFSET(CarSetup, engine_stage, 8);
GUEST_OFFSET(CarSetup, front_tire_type, 64);
GUEST_OFFSET(CarSetup, aero_levels, 96);
GUEST_OFFSET(CarSetup, block2, 244);
GUEST_OFFSET(CarSetup, block1, 596);
GUEST_OFFSET(CarSetup, engine, 1236);
GUEST_OFFSET(CarSetup, front_tire, 1440);
GUEST_OFFSET(CarSetup, front_wheel_record, 1624);
GUEST_OFFSET(CarSetup, paints, 1720);
GUEST_OFFSET(CarSetup, aero_74, 1992);
GUEST_OFFSET(CarSetup, aero_80, 2132);
GUEST_OFFSET(CarSetup, aero_81, 2160);
GUEST_OFFSET(CarSetup, front_width_change, 2232);

// A racer's car: what Vehicle::SetupFromCar builds a vehicle from.
struct Car {
  u16_be unknown_0;
  u16_be unknown_2;
  i16_be unknown_4;
  u16_be unknown_6;
  u16_be unknown_8;
  uint8_t unknown_10;
  uint8_t unknown_11;
  u32_be car_id;  // Its spec's number (Params::GetCarSpec).
  CarSettings settings;
  Decal decals_96[6];
  Decal decals_288[40];
  CarOptions options;
  CarSlot slots[3];
  CarSetup setup;
  u32_be unknown_3888;
  u32_be unknown_3892;
};
GUEST_OFFSET(Car, settings, 16);
GUEST_OFFSET(Car, decals_96, 96);
GUEST_OFFSET(Car, decals_288, 288);
GUEST_OFFSET(Car, options, 1568);
GUEST_OFFSET(Car, slots, 1584);
GUEST_OFFSET(Car, setup, 1644);
GUEST_OFFSET(Car, unknown_3888, 3888);
GUEST_SIZE(Car, 3896);

// A racer's record: its car, then a few ids.
struct RacerRecord {
  Car car;
  uint8_t unknown_3896[8];
  i16_be unknown_3904;
  i16_be unknown_3906;
  uint8_t unknown_3908[4];
  i32_be unknown_3912;
  i32_be unknown_3916;
  f32_be unknown_3920;
  u32_be unknown_3924;
  uint8_t unknown_3928[8];
};
GUEST_SIZE(RacerRecord, 3936);
GUEST_OFFSET(RacerRecord, unknown_3904, 3904);
GUEST_OFFSET(RacerRecord, unknown_3920, 3920);

struct Racers {
  static constexpr int kRecords = 113;

  RacerRecord records[kRecords];
  u32_be unknown_444768[4];
};
GUEST_SIZE(Racers, 444784);

// 76 bytes, cleared by CourseBlock::Clear.
struct CourseBlock {
  uint8_t unknown_0[60];
  u32_be unknown_60;
  u32_be unknown_64;
  u32_be unknown_68;
  u32_be unknown_72;
};
GUEST_SIZE(CourseBlock, 76);

// A list of ten slots: each slot's index and the next free one (-1 after
// the last).
struct CourseSlot {
  i16_be next;
  u16_be index;
};
GUEST_SIZE(CourseSlot, 4);

struct CourseRecord {
  CourseSlot slots[10];
  u16_be unknown_40;
  i16_be unknown_42;
  i16_be unknown_44;
  i16_be unknown_46;
  u16_be unknown_48;
  uint8_t unknown_50[282];
  CourseBlock block;
};
GUEST_SIZE(CourseRecord, 408);
GUEST_OFFSET(CourseRecord, block, 332);

struct StagePair {
  i32_be unknown_0;
  u32_be unknown_4;
};
GUEST_SIZE(StagePair, 8);

struct Stage {
  u32_be unknown_0;
  u32_be unknown_4;
  u32_be unknown_8;
  uint8_t unknown_12[32];
  StagePair pairs[4];
  u32_be unknown_76;
};
GUEST_SIZE(Stage, 80);
GUEST_OFFSET(Stage, pairs, 44);

struct Stages {
  Stage stages[10];
  u32_be unknown_800;
  u32_be unknown_804;
  u32_be unknown_808;
  CourseRecord course;
  u32_be unknown_1220;
};
GUEST_SIZE(Stages, 1224);
GUEST_OFFSET(Stages, course, 812);

// A part of a race entry, made by RaceEntryPart::RaceEntryPart.
struct RaceEntryPart {
  uint8_t unknown_0[12];
  u32_be time_count;  // Counted up by RaceProgress::CountTime.
  u32_be time;        // A time word made from the count.
  uint8_t unknown_20[764];
  u32_be unknown_784;
  u32_be unknown_788;
  f32_be unknown_792;
  u32_be unknown_796;
  uint8_t unknown_800[172];
};
GUEST_SIZE(RaceEntryPart, 972);
GUEST_OFFSET(RaceEntryPart, time_count, 12);
GUEST_OFFSET(RaceEntryPart, unknown_784, 784);

struct RaceEntry {
  u32_be unknown_0;
  RaceEntryPart parts[4];
  uint8_t unknown_3892[16];  // Reset by sub_82101B18.
  u32_be unknown_3908;
  u32_be unknown_3912;
  uint8_t unknown_3916[4];
  u32_be unknown_3920;
  u32_be unknown_3924;
  uint8_t unknown_3928[4];
  u32_be unknown_3932;
};
GUEST_SIZE(RaceEntry, 3936);
GUEST_OFFSET(RaceEntry, unknown_3892, 3892);
GUEST_OFFSET(RaceEntry, unknown_3908, 3908);
GUEST_OFFSET(RaceEntry, unknown_3932, 3932);

struct RaceEntries {
  static constexpr int kEntries = 97;

  RaceEntry entries[kEntries];
  u32_be unknown_381792;
  u32_be unknown_381796;
};
GUEST_SIZE(RaceEntries, 381800);

// The race data (made into [0x826B580C] by RaceData::Create).
struct RaceData {
  uint8_t unknown_0[446056];
  RaceEntries entries;
  uint8_t unknown_827856[48];
};
GUEST_SIZE(RaceData, 827904);
GUEST_OFFSET(RaceData, entries, 446056);

}  // namespace game
