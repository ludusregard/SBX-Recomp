// The save interface's queries as our own C++ (--native_save_interface, on
// by default): the save slots' getters (device, free bytes, saves, state,
// pending work), a device's request, the signed-in users, the save
// interface's calls into the app's save devices and users, a slot by its
// controller, the play time's update and the devices' root name.
//
// Written from SBX's generated code. A save slot: [+44] + 1000 x slot,
// with +36 its device, +40 its free bytes (a doubleword), +80 its saves
// (308 bytes each), +960 to +980 its pending requests, +984 the save
// count, +988 its state. The app ([0x826AD970]): its vtable's slot 3 the
// save devices, slot 2 the users. The content calls (enumerate, read,
// write), the interface's set-up and load stay recompiled.

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_save_interface, true, "SBX",
                    "Our own C++ for the save interface's queries (the slots' getters, the "
                    "users, a slot by its controller, the play time) instead of the recompiled "
                    "functions; the content calls stay recompiled");

namespace {

using guest::LoadU32;
using guest::LoadU8;
using guest::StoreU32;
using guest::StoreU8;

constexpr uint32_t kApp = 0x826AD970;  // [it] the app

uint32_t Slot(uint8_t* base, uint32_t slots, uint32_t slot) {
  return LoadU32(base, slots + 44) + 1000 * slot;
}

// An object's vtable slot `at` (bytes), r3 the object; r4 and r5 as set.
void CallSlot(PPCContext& ctx, uint8_t* base, uint32_t object, uint32_t at) {
  ctx.r3.u64 = object;
  REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, object) + at));
}

// The app's slot 3 (the save devices) or 2 (the users), then that one's
// slot `at` on the result with r4 (and r5 when `with_r5`; else r5 as the
// first call left it).
void AppCall(PPCContext& ctx, uint8_t* base, uint32_t app_slot, uint32_t at, uint32_t r4,
             uint32_t r5, bool with_r5) {
  CallSlot(ctx, base, LoadU32(base, kApp), app_slot);
  ctx.r4.u64 = r4;
  if (with_r5) {
    ctx.r5.u64 = r5;
  }
  CallSlot(ctx, base, ctx.r3.u32, at);
}

uint32_t UserCount(PPCContext& ctx, uint8_t* base) {
  CallSlot(ctx, base, LoadU32(base, kApp), 8);
  CallSlot(ctx, base, ctx.r3.u32, 4);
  return ctx.r3.u32;
}

uint32_t UserAt(PPCContext& ctx, uint8_t* base, uint32_t i) {
  AppCall(ctx, base, 8, 8, i, 0, false);
  return ctx.r3.u32;
}

bool On() { return REXCVAR_GET(native_save_interface); }

}  // namespace

// App::SaveDevices and App::Users: [+8]'s slots 10 (+40) and 9 (+36), tail
// calls.
extern "C" REX_FUNC(sub_820949C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820949C8(ctx, base);
    return;
  }
  CallSlot(ctx, base, LoadU32(base, ctx.r3.u32 + 8), 40);
}

extern "C" REX_FUNC(sub_820949E0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820949E0(ctx, base);
    return;
  }
  CallSlot(ctx, base, LoadU32(base, ctx.r3.u32 + 8), 36);
}

// Users::SignedIn: r4 the user (below +216's count): its controller ([+200
// + 4 x r4]) when that one's record (+8 of 40 bytes each) is set, else -1.
extern "C" REX_FUNC(sub_82099E00) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82099E00(ctx, base);
    return;
  }
  const uint32_t users = ctx.r3.u32, user = ctx.r4.u32;
  if (user < LoadU32(base, users + 216)) {
    const uint32_t controller = LoadU32(base, users + ((user + 50) << 2));
    if (int32_t(LoadU32(base, users + 40 * controller + 8)) != 0) {
      ctx.r3.u64 = controller;
      return;
    }
  }
  ctx.r3.u64 = 0xFFFFFFFFFFFFFFFFull;
}

// SaveSlot::RequestDevice: the slot's +960 = 1.
extern "C" REX_FUNC(sub_820A5970) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A5970(ctx, base);
    return;
  }
  StoreU32(base, Slot(base, ctx.r3.u32, ctx.r4.u32) + 960, 1);
}

// SaveSlot::Device (+36), SaveCount (+984), State (+988).
#define SBX_SLOT_WORD(address, at)                                           \
  extern "C" REX_FUNC(sub_##address) {                                       \
    REX_FUNC_PROLOGUE();                                                     \
    if (!On()) {                                                             \
      __imp__sub_##address(ctx, base);                                       \
      return;                                                                \
    }                                                                        \
    ctx.r3.u64 = LoadU32(base, Slot(base, ctx.r3.u32, ctx.r4.u32) + at);     \
  }

SBX_SLOT_WORD(820A5988, 36)
SBX_SLOT_WORD(820A59B8, 984)
SBX_SLOT_WORD(820A5A10, 988)

#undef SBX_SLOT_WORD

// SaveSlot::FreeBytes: the doubleword at +40.
extern "C" REX_FUNC(sub_820A59A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A59A0(ctx, base);
    return;
  }
  ctx.r3.u64 = REX_LOAD_U64(Slot(base, ctx.r3.u32, ctx.r4.u32) + 40);
}

// SaveSlot::SaveAt: r5's save (+80's, 308 bytes each).
extern "C" REX_FUNC(sub_820A59D0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A59D0(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, Slot(base, ctx.r3.u32, ctx.r4.u32) + 80) + 308 * ctx.r5.u32;
}

// SaveSlot::Pending: the six requests' sum (+960 to +980).
extern "C" REX_FUNC(sub_820A5A28) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A5A28(ctx, base);
    return;
  }
  const uint32_t slot = Slot(base, ctx.r3.u32, ctx.r4.u32);
  uint32_t sum = 0;
  for (uint32_t at = 960; at <= 980; at += 4) {
    sum += LoadU32(base, slot + at);
  }
  ctx.r3.u64 = sum;
}

// SaveDevices::SetRootName: r4's name to +16 (strcpy).
extern "C" REX_FUNC(sub_820A57A0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A57A0(ctx, base);
    return;
  }
  const uint32_t to = ctx.r3.u32 + 16, from = ctx.r4.u32;
  for (uint32_t i = 0;; i++) {
    const uint8_t c = LoadU8(base, from + i);
    StoreU8(base, to + i, c);
    if (c == 0) {
      break;
    }
  }
}

// PlayTime::Update: sub_82110928 with the original's 96-byte frame's +80
// and r3 + 48.
extern "C" REX_FUNC(sub_82112988) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_82112988(ctx, base);
    return;
  }
  const uint32_t time = ctx.r3.u32;
  guest::StackFrame frame(ctx, 96);
  ctx.r3.u64 = frame.Local(80);
  ctx.r4.u64 = time + 48;
  sub_82110928(ctx, base);
}

// SaveInterface's calls into the save devices (the app's slot 3): r3 (and
// r4) passed on as r4 (and r5) to the devices' slot.
#define SBX_DEVICES_CALL(address, at, with_r5)                        \
  extern "C" REX_FUNC(sub_##address) {                                \
    REX_FUNC_PROLOGUE();                                              \
    if (!On()) {                                                      \
      __imp__sub_##address(ctx, base);                                \
      return;                                                         \
    }                                                                 \
    AppCall(ctx, base, 12, at, ctx.r3.u32, ctx.r4.u32, with_r5);      \
  }

SBX_DEVICES_CALL(820A03F0, 4, false)   // SaveInterface::RequestDevice
SBX_DEVICES_CALL(820A04B0, 16, false)  // SaveInterface::SaveCount
SBX_DEVICES_CALL(820A0508, 12, false)  // SaveInterface::FreeBytes
SBX_DEVICES_CALL(820A0560, 32, false)  // SaveInterface::Pending
SBX_DEVICES_CALL(820A05B8, 8, false)   // SaveInterface::Device
SBX_DEVICES_CALL(820A0610, 20, true)   // SaveInterface::SaveAt

#undef SBX_DEVICES_CALL

// SaveInterface::UserCount and UserAt: the users' (the app's slot 2) slots
// 1 and 2.
extern "C" REX_FUNC(sub_820A07C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A07C0(ctx, base);
    return;
  }
  UserCount(ctx, base);
}

extern "C" REX_FUNC(sub_820A0808) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820A0808(ctx, base);
    return;
  }
  UserAt(ctx, base, ctx.r3.u32);
}

// SaveSlots::OfController: r3 the controller: the first user it is, or the
// user count (also when that's 0 or below).
extern "C" REX_FUNC(sub_8228CB68) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8228CB68(ctx, base);
    return;
  }
  const uint32_t controller = ctx.r3.u32;
  const int32_t count = int32_t(UserCount(ctx, base));
  if (count <= 0) {
    ctx.r3.u64 = uint64_t(int64_t(count));
    return;
  }
  int32_t i = 0;
  for (; i < count; i++) {
    if (UserAt(ctx, base, uint32_t(i)) == controller) {
      break;
    }
  }
  ctx.r3.u64 = uint64_t(int64_t(i));
}
