// The save interface's queries as our own C++ (--native_save_interface, on
// by default): the save slots' getters (device, free bytes, saves, state,
// pending work), a device's request, the signed-in users, the save
// interface's calls into the app's save devices and users, a slot by its
// controller, the play time's update, the devices' root name, and
// SaveGame::Write (for autosave). The content calls (enumerate, read,
// write), the interface's set-up and load stay recompiled.

#include "game/save_interface.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_save_interface, true, "SBX",
                    "Our own C++ for the save interface's queries (the slots' getters, the "
                    "users, a slot by its controller, the play time) instead of the recompiled "
                    "functions; the content calls stay recompiled");

namespace {

using game::App;
using game::LocalTime;
using game::SaveDevices;
using game::SaveSlot;
using game::SaveSlots;
using game::Users;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_save_interface); }

constexpr uint32_t kApp = 0x826AD970;           // [it] the app (writable)
constexpr uint32_t kSaveController = 0x826AA884;  // [it] the controller saving (writable)
constexpr uint32_t kSaveNameFormat = 0x82011BAC;  // Read-only strings.
constexpr uint32_t kSaveNameText = 0x82011BDC;
constexpr uint32_t kSaveFileName = 0x82011B98;
constexpr uint32_t kSaveContent = 0x82628040;     // Passed on (writable).
constexpr uint32_t kSaveBytes = 129200;

App* TheApp() { return At<App>(*At<u32_be>(kApp)); }

// The app's save devices (vtable slot 3) or users (slot 2).
uint32_t AppPart(Guest& g, uint32_t slot) { return g.CallVirtual(TheApp(), slot); }

uint32_t UserCount(Guest& g) {
  const uint32_t users = AppPart(g, App::kUsersSlot);
  return g.CallVirtual(At<u32_be>(users), 1);
}

uint32_t UserAt(Guest& g, uint32_t i) {
  const uint32_t users = AppPart(g, App::kUsersSlot);
  return g.CallVirtual(At<u32_be>(users), 2, i);
}

SaveSlot* Slot(PPCContext& ctx) { return At<SaveSlots>(ctx.r3.u32)->Slot(ctx.r4.u32); }

}  // namespace

#define SBX_SAVE_INTERFACE_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                    \
  if (!On()) {                            \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// App::SaveDevices and App::Users: r3 the app; its services' (+8) slots 10
// and 9.
extern "C" REX_FUNC(sub_820949C8) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_820949C8)
  g.CallVirtual(At<u32_be>(At<App>(ctx.r3.u32)->services), 10);
}

extern "C" REX_FUNC(sub_820949E0) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_820949E0)
  g.CallVirtual(At<u32_be>(At<App>(ctx.r3.u32)->services), 9);
}

// Users::SignedIn: r3 the users, r4 the user (below their count): its
// controller when that controller's record is signed in, else -1.
extern "C" REX_FUNC(sub_82099E00) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_82099E00)
  Users* users = At<Users>(ctx.r3.u32);
  const uint32_t user = ctx.r4.u32;
  if (user < users->count) {
    const int32_t controller = *guest::Element<guest::i32_be>(guest::AddressOf(users->controllers), user);
    if (guest::Element<game::UserRecord>(guest::AddressOf(users->records), uint32_t(controller))->signed_in != 0) {
      ctx.r3.u64 = uint32_t(controller);
      return;
    }
  }
  ctx.r3.s64 = -1;
}

// The save slots' getters: r3 the slots, r4 the slot.
extern "C" REX_FUNC(sub_820A5970) {  // SaveSlot::RequestDevice
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A5970)
  Slot(ctx)->requests[0] = 1;
}

extern "C" REX_FUNC(sub_820A5988) {  // SaveSlot::Device
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A5988)
  ctx.r3.u64 = uint32_t(Slot(ctx)->device);
}

extern "C" REX_FUNC(sub_820A59B8) {  // SaveSlot::SaveCount
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A59B8)
  ctx.r3.u64 = uint32_t(Slot(ctx)->save_count);
}

extern "C" REX_FUNC(sub_820A5A10) {  // SaveSlot::State
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A5A10)
  ctx.r3.u64 = uint32_t(Slot(ctx)->state);
}

extern "C" REX_FUNC(sub_820A59A0) {  // SaveSlot::FreeBytes (a doubleword)
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A59A0)
  ctx.r3.u64 = uint64_t(Slot(ctx)->free_bytes);
}

// SaveSlot::SaveAt: r5's save (308 bytes each).
extern "C" REX_FUNC(sub_820A59D0) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A59D0)
  ctx.r3.u64 = uint32_t(Slot(ctx)->saves + 308 * ctx.r5.u32);
}

// SaveSlot::Pending: the six requests' sum.
extern "C" REX_FUNC(sub_820A5A28) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A5A28)
  uint32_t sum = 0;
  for (const u32_be& request : Slot(ctx)->requests) {
    sum += request;
  }
  ctx.r3.u64 = sum;
}

// SaveDevices::SetRootName: r3 the devices, r4 the name copied in (with its
// terminator).
extern "C" REX_FUNC(sub_820A57A0) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A57A0)
  char* to = At<SaveDevices>(ctx.r3.u32)->root_name;
  for (const char* from = At<char>(ctx.r4.u32); (*to++ = *from++) != 0;) {
  }
}

// PlayTime::Update: r3 the save; sub_82110928 with a local of the
// original's 96-byte frame and the save's play time (+48).
extern "C" REX_FUNC(sub_82112988) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_82112988)
  const uint32_t save = ctx.r3.u32;
  guest::StackFrame frame(ctx, 96);
  g.Call(sub_82110928, frame.Local(80), save + 48);
}

// SaveInterface's calls into the save devices: r3 (and r4) passed on as r4
// (and r5) to the devices' slot; r5 otherwise as the first call leaves it.
#define SBX_DEVICES_CALL(address, slot)                                         \
  extern "C" REX_FUNC(sub_##address) {                                          \
    SBX_SAVE_INTERFACE_FALLBACK(sub_##address)                                  \
    const uint32_t a = ctx.r3.u32;                                              \
    const uint32_t devices = AppPart(g, App::kSaveDevicesSlot);                 \
    g.CallVirtual(At<u32_be>(devices), slot, a);                                \
  }

SBX_DEVICES_CALL(820A03F0, 1)  // SaveInterface::RequestDevice
SBX_DEVICES_CALL(820A04B0, 4)  // SaveInterface::SaveCount
SBX_DEVICES_CALL(820A0508, 3)  // SaveInterface::FreeBytes
SBX_DEVICES_CALL(820A0560, 8)  // SaveInterface::Pending
SBX_DEVICES_CALL(820A05B8, 2)  // SaveInterface::Device

#undef SBX_DEVICES_CALL

extern "C" REX_FUNC(sub_820A0610) {  // SaveInterface::SaveAt
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A0610)
  const uint32_t a = ctx.r3.u32, b = ctx.r4.u32;
  const uint32_t devices = AppPart(g, App::kSaveDevicesSlot);
  g.CallVirtual(At<u32_be>(devices), 5, a, b);
}

// SaveInterface::UserCount and UserAt: the users' slots 1 and 2.
extern "C" REX_FUNC(sub_820A07C0) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A07C0)
  UserCount(g);
}

extern "C" REX_FUNC(sub_820A0808) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_820A0808)
  UserAt(g, ctx.r3.u32);
}

// SaveSlots::OfController: r3 the controller: the first user it is, or the
// user count (also when that's 0 or below).
extern "C" REX_FUNC(sub_8228CB68) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_8228CB68)
  const uint32_t controller = ctx.r3.u32;
  const int32_t count = int32_t(UserCount(g));
  int32_t i = 0;
  while (i < count && UserAt(g, uint32_t(i)) != controller) {
    i++;
  }
  ctx.r3.s64 = count <= 0 ? count : i;
}

// Two plain getters: the word at +92 and the word at +216 (the users'
// count, for a Users).
extern "C" REX_FUNC(sub_8236FD60) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_8236FD60)
  struct Object92 {
    uint8_t unknown_0[92];
    u32_be word_92;
  };
  ctx.r3.u64 = uint32_t(At<Object92>(ctx.r3.u32)->word_92);
}

extern "C" REX_FUNC(sub_82473DE8) {
  SBX_SAVE_INTERFACE_FALLBACK(sub_82473DE8)
  ctx.r3.u64 = uint32_t(At<Users>(ctx.r3.u32)->count);
}

// SaveGame::Write: no arguments. The save's display name formatted
// (swprintf, sub_8210A5C0, into 256 cleared bytes of the original's
// 368-byte frame, with the local time, sub_82294300), then the write asked
// of the save interface (sub_820A0678) for the saving controller's slot.
// Called from autosave's override (autosave.cpp), which logs it.
void hooks::NativeSaveInterface_Write(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_8228CEE8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  guest::StackFrame frame(ctx, 368);
  LocalTime* time = At<LocalTime>(frame.Local(80));
  const uint32_t name = frame.Local(96);
  g.Call(sub_8258C950, name, 0u, 256u);  // memset
  const uint32_t save = g.Call(sub_82105790, 0u);
  g.Call(sub_82112988, save);  // PlayTime::Update
  g.Call(sub_82294300, time);  // the local time
  g.Call(sub_8210A5C0, name, kSaveNameFormat, kSaveNameText, uint32_t(time->year),
         uint32_t(time->month), uint32_t(time->day), uint32_t(time->hour),
         uint32_t(time->minute));  // swprintf
  const uint32_t save_again = g.Call(sub_82105790, 0u);
  const uint32_t slot = g.Call(sub_8228CB68, *At<u32_be>(kSaveController));  // SaveSlots::OfController
  g.Call(sub_820A0678, slot, name, kSaveContent, kSaveFileName, save_again, kSaveBytes, 0u, 0u);
}
