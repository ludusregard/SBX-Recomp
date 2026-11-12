// The save interface's objects: the save slots (one per device, 1,000 bytes
// each), the signed-in users, and the app's save devices and users, called
// through their vtables.

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

struct SaveSlot {
  uint8_t unknown_0[36];
  u32_be device;
  guest::be<uint64_t> free_bytes;
  uint8_t unknown_48[32];
  u32_be saves;  // The address of its saves (308 bytes each).
  uint8_t unknown_84[876];
  u32_be requests[6];  // Pending requests; [0] a device's.
  u32_be save_count;
  u32_be state;
  uint8_t unknown_992[8];
};
GUEST_SIZE(SaveSlot, 1000);
GUEST_OFFSET(SaveSlot, free_bytes, 40);
GUEST_OFFSET(SaveSlot, saves, 80);
GUEST_OFFSET(SaveSlot, requests, 960);
GUEST_OFFSET(SaveSlot, state, 988);

struct SaveSlots {
  uint8_t unknown_0[44];
  u32_be slots;  // The address of the SaveSlot array.

  SaveSlot* Slot(uint32_t index) { return guest::Element<SaveSlot>(slots, index); }
};
GUEST_OFFSET(SaveSlots, slots, 44);

struct UserRecord {
  uint8_t unknown_0[8];
  u32_be signed_in;
  uint8_t unknown_12[28];
};
GUEST_SIZE(UserRecord, 40);

// The signed-in users: a record per controller, each user's controller.
struct Users {
  UserRecord records[5];
  i32_be controllers[4];
  u32_be count;
};
GUEST_OFFSET(Users, controllers, 200);
GUEST_OFFSET(Users, count, 216);

struct SaveDevices {
  uint8_t unknown_0[16];
  char root_name[64];
};

// The app ([0x826AD970]): its vtable's slot 3 the save devices, slot 2 the
// users; +8 the object those come from.
struct App {
  static constexpr uint32_t kSaveDevicesSlot = 3;
  static constexpr uint32_t kUsersSlot = 2;

  u32_be vtable;
  uint8_t unknown_4[4];
  u32_be services;
};
GUEST_OFFSET(App, services, 8);

// The local time (KeQuerySystemTime's fields, sub_82294300): year, month,
// day of week, day, hour, minute...
struct LocalTime {
  guest::u16_be year;
  guest::u16_be month;
  guest::u16_be day_of_week;
  guest::u16_be day;
  guest::u16_be hour;
  guest::u16_be minute;
  guest::u16_be second;
  guest::u16_be millisecond;
};
GUEST_SIZE(LocalTime, 16);

}  // namespace game
