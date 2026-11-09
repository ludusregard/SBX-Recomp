// The interior accessories' set and files as our own C++
// (--native_accessories, on by default): the set at [0x826B51F0] (three
// entries of 1,104 bytes, each a count, an AccessoryFile at +4 and the
// accessory at +48), its create, scale, acquire and release, and the
// 36-byte AccessoryFile (its load through the archive and its callback).
//
// Written from SBX's generated code. The archive (AssetSize, Load,
// Cancel), VirtualAlloc and VirtualFree, the entries' and accessories'
// own constructors and destructors (with their skeletons), the accessory's
// texture and the matrix code stay recompiled.
//
// An AccessoryFile: +0 the vtable, +4 the asset, +8 the archive's serial,
// +12 its state (0 none, 1 loading, 2 loaded), +16 the data, +20 its size,
// +24 whether the data is the file's own (VirtualAlloc'd), +28 a callback
// and +32 its context.

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_accessories, true, "SBX",
                    "Our own C++ for the interior accessories' set and files (create, scale, "
                    "acquire, release, the file's load and callback) instead of the recompiled "
                    "functions; the archive and the accessories' models stay recompiled");

namespace {

using guest::LoadU32;
using guest::StoreU32;

constexpr uint32_t kSet = 0x826B51F0;            // [it] the set
constexpr uint32_t kSetBaseVtable = 0x82006704;
constexpr uint32_t kSetVtable = 0x82006718;
constexpr uint32_t kFileVtable = 0x82006710;
constexpr uint32_t kOne = 0x82000A20;            // 1.0f
constexpr uint32_t kFileOnArchiveDone = 0x820F0AB0;
constexpr uint32_t kEntryOnLoaded = 0x820F06B0;  // AccessoryEntry::OnLoaded
constexpr uint32_t kFirstAsset = 23006;          // entry i's file is asset 23006 + i
constexpr uint32_t kAllocType = 0x00801000;      // VirtualAlloc's type (MEM_COMMIT | 0x800000)
constexpr uint32_t kMemRelease = 0x8000;

// VirtualAlloc (sub_82292DD8): 0, bytes, kAllocType, PAGE_READWRITE.
uint32_t AllocPages(PPCContext& ctx, uint8_t* base, uint32_t bytes) {
  ctx.r3.u64 = 0;
  ctx.r4.u64 = bytes;
  ctx.r5.u64 = kAllocType;
  ctx.r6.u64 = 4;
  sub_82292DD8(ctx, base);
  return ctx.r3.u32;
}

// AccessoryFile::FreeData (sub_820F0760): the block at [slot] VirtualFree'd
// (sub_82292D70) and the word cleared; nothing for an empty word (r3 left
// as the word's 0).
void FreeData(PPCContext& ctx, uint8_t* base, uint32_t slot) {
  const uint32_t block = LoadU32(base, slot);
  ctx.r3.u64 = block;
  if (block == 0) {
    return;
  }
  ctx.r4.u64 = 0;
  ctx.r5.u64 = kMemRelease;
  sub_82292D70(ctx, base);
  StoreU32(base, slot, 0);
}

// AccessoryFile::Load (sub_820F0B20): r3 the file, r4 the asset, r5 a
// buffer (0: the file allocates its own), r6 the callback, r7 its context.
// Archive::AssetSize (sub_820A0860) into +20, then Archive::Load
// (sub_8209DF80: the asset, the data, OnArchiveDone, the file, 0, 0); the
// serial into +8 and returned.
void FileLoad(PPCContext& ctx, uint8_t* base, uint32_t file, uint32_t asset, uint32_t buffer,
              uint32_t callback, uint32_t context) {
  StoreU32(base, file + 4, asset);
  ctx.r3.u64 = asset;
  sub_820A0860(ctx, base);
  const uint32_t size = ctx.r3.u32;
  StoreU32(base, file + 20, size);
  if (buffer != 0) {
    StoreU32(base, file + 16, buffer);
    StoreU32(base, file + 24, 0);
  } else {
    StoreU32(base, file + 16, AllocPages(ctx, base, size));
    StoreU32(base, file + 24, 1);
  }
  ctx.r3.u64 = LoadU32(base, file + 4);
  ctx.r4.u64 = LoadU32(base, file + 16);
  ctx.r5.u64 = kFileOnArchiveDone;
  ctx.r6.u64 = file;
  ctx.r7.u64 = 0;
  ctx.r8.u64 = 0;
  sub_8209DF80(ctx, base);
  StoreU32(base, file + 12, 1);
  StoreU32(base, file + 28, callback);
  StoreU32(base, file + 32, context);
  StoreU32(base, file + 8, ctx.r3.u32);
}

// AccessoryEntry::Destroy (sub_820F0818) on a slot, recompiled.
void DestroyEntry(PPCContext& ctx, uint8_t* base, uint32_t slot) {
  ctx.r3.u64 = slot;
  sub_820F0818(ctx, base);
}

// One entry's count down by one; at 0 it's destroyed. False when the slot
// is empty.
bool ReleaseSlot(PPCContext& ctx, uint8_t* base, uint32_t slot) {
  const uint32_t entry = LoadU32(base, slot);
  if (entry == 0) {
    return false;
  }
  StoreU32(base, entry, LoadU32(base, entry) - 1);
  if (LoadU32(base, LoadU32(base, slot)) == 0) {
    DestroyEntry(ctx, base, slot);
  }
  return true;
}

// Accessory::SetTexture (sub_820EF3D8): r3 the accessory, r4 the texture,
// r5 = -1.
void SetTexture(PPCContext& ctx, uint8_t* base, uint32_t accessory, uint32_t texture) {
  ctx.r3.u64 = accessory;
  ctx.r4.u64 = texture;
  ctx.r5.u64 = 0xFFFFFFFFFFFFFFFFull;
  sub_820EF3D8(ctx, base);
}

// Accessories::AcquireEntry (sub_820F0870): r3 the set, r4 the index (3:
// none), r5 the texture. A new entry (AccessoryEntry::New, sub_820F07B0)
// has its texture set and its file loaded (asset 23006 + index, the
// entry's OnLoaded with the entry); a held one has its count up by one and
// its texture set. Returns the accessory (the entry + 48), or 0.
uint32_t AcquireEntry(PPCContext& ctx, uint8_t* base, uint32_t set, uint32_t index,
                      uint32_t texture) {
  if (int32_t(index) == 3) {
    return 0;
  }
  const uint32_t slot = set + 4 * (index + 1);
  const uint32_t held = LoadU32(base, slot);
  if (held == 0) {
    sub_820F07B0(ctx, base);
    const uint32_t entry = ctx.r3.u32;
    StoreU32(base, slot, entry);
    if (entry == 0) {
      return 0;
    }
    SetTexture(ctx, base, entry + 48, texture);
    const uint32_t made = LoadU32(base, slot);
    FileLoad(ctx, base, made + 4, index + kFirstAsset, 0, kEntryOnLoaded, made);
  } else {
    StoreU32(base, held, LoadU32(base, held) + 1);
    SetTexture(ctx, base, LoadU32(base, slot) + 48, texture);
  }
  return LoadU32(base, slot) + 48;
}

// Accessories::Free (sub_820F0E20): r3 the set's slot; a set held there is
// destroyed (its vtable's slot 0 with r4 = 0) and its pages freed.
void FreeSet(PPCContext& ctx, uint8_t* base, uint32_t slot) {
  const uint32_t set = LoadU32(base, slot);
  ctx.r3.u64 = set;
  if (set == 0) {
    return;
  }
  ctx.r4.u64 = 0;
  REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, set)));
  FreeData(ctx, base, slot);
}

bool On() { return REXCVAR_GET(native_accessories); }

}  // namespace

// AccessoryFile::AccessoryFile.
extern "C" REX_FUNC(sub_820F0AE0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0AE0(ctx, base);
    return;
  }
  const uint32_t file = ctx.r3.u32;
  StoreU32(base, file + 4, 0xFFFFFFFFu);
  StoreU32(base, file + 0, kFileVtable);
  StoreU32(base, file + 8, 0xFFFFFFFFu);
  StoreU32(base, file + 12, 0);
  StoreU32(base, file + 16, 0);
  StoreU32(base, file + 20, 0);
  StoreU32(base, file + 24, 1);
  StoreU32(base, file + 28, 0);
  StoreU32(base, file + 32, 0);
}

// AccessoryFile::SetData: r4 the data, r5 whether it's the file's own.
extern "C" REX_FUNC(sub_820F0A80) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0A80(ctx, base);
    return;
  }
  const uint32_t file = ctx.r3.u32;
  StoreU32(base, file + 16, ctx.r4.u32);
  StoreU32(base, file + 24, ctx.r5.u32);
  StoreU32(base, file + 20, 0);
  StoreU32(base, file + 4, 0xFFFFFFFFu);
  StoreU32(base, file + 8, 0xFFFFFFFFu);
  StoreU32(base, file + 12, 2);
  StoreU32(base, file + 28, 0);
  StoreU32(base, file + 32, 0);
}

extern "C" REX_FUNC(sub_820F0B20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0B20(ctx, base);
    return;
  }
  FileLoad(ctx, base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32);
}

// AccessoryFile::OnArchiveDone: the archive's callback, r3 the serial, r6
// the file. Unless the file's serial (+8) is another, +12 = 2 and its
// callback (+28) runs with r6 = +32 (the original's tail call; r3 to r5 as
// given).
extern "C" REX_FUNC(sub_820F0AB0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0AB0(ctx, base);
    return;
  }
  const uint32_t file = ctx.r6.u32;
  if (ctx.r3.u32 != LoadU32(base, file + 8)) {
    return;
  }
  StoreU32(base, file + 12, 2);
  const uint32_t callback = LoadU32(base, file + 28);
  if (callback == 0) {
    return;
  }
  ctx.r6.u64 = LoadU32(base, file + 32);
  REX_CALL_INDIRECT_FUNC(callback);
}

extern "C" REX_FUNC(sub_820F0760) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0760(ctx, base);
    return;
  }
  FreeData(ctx, base, ctx.r3.u32);
}

// AccessoryFile::Reset: the file's own data cancelled (Archive::Cancel,
// sub_8209DC40, while loading) or freed (once loaded), then the file's
// words back to their defaults.
extern "C" REX_FUNC(sub_820F0BC0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0BC0(ctx, base);
    return;
  }
  const uint32_t file = ctx.r3.u32;
  if (LoadU32(base, file + 24) != 0) {
    const int32_t state = int32_t(LoadU32(base, file + 12));
    if (state == 1) {
      ctx.r3.u64 = LoadU32(base, file + 8);
      sub_8209DC40(ctx, base);
    } else if (state == 2) {
      FreeData(ctx, base, file + 16);
    }
  }
  StoreU32(base, file + 12, 0);
  StoreU32(base, file + 4, 0xFFFFFFFFu);
  StoreU32(base, file + 8, 0xFFFFFFFFu);
  StoreU32(base, file + 16, 0);
  StoreU32(base, file + 20, 0);
  StoreU32(base, file + 24, 1);
  StoreU32(base, file + 28, 0);
  StoreU32(base, file + 32, 0);
}

// AccessoriesBase::AccessoriesBase: the vtable, the three entries 0.
extern "C" REX_FUNC(sub_820F0648) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0648(ctx, base);
    return;
  }
  const uint32_t set = ctx.r3.u32;
  StoreU32(base, set + 0, kSetBaseVtable);
  StoreU32(base, set + 4, 0);
  StoreU32(base, set + 8, 0);
  StoreU32(base, set + 12, 0);
}

// Accessories::Accessories: the base, the vtable, the scale 1.0 (+28
// written first and last, as the original); returns the set.
void ConstructSet(uint8_t* base, uint32_t set) {
  StoreU32(base, set + 0, kSetBaseVtable);
  StoreU32(base, set + 4, 0);
  StoreU32(base, set + 8, 0);
  StoreU32(base, set + 12, 0);
  const uint32_t one = LoadU32(base, kOne);
  StoreU32(base, set + 0, kSetVtable);
  StoreU32(base, set + 28, one);
  StoreU32(base, set + 16, one);
  StoreU32(base, set + 20, one);
  StoreU32(base, set + 24, one);
  StoreU32(base, set + 28, one);
}

extern "C" REX_FUNC(sub_820F0CF0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0CF0(ctx, base);
    return;
  }
  ConstructSet(base, ctx.r3.u32);
}

// Accessories::New: 32 bytes of pages made a set; 0 when there are none.
extern "C" REX_FUNC(sub_820F0DD8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0DD8(ctx, base);
    return;
  }
  const uint32_t set = AllocPages(ctx, base, 32);
  if (set != 0) {
    ConstructSet(base, set);
  }
  ctx.r3.u64 = set;
}

// Accessories::Create: the set made once.
extern "C" REX_FUNC(sub_820F0EC8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0EC8(ctx, base);
    return;
  }
  if (LoadU32(base, kSet) != 0) {
    return;
  }
  const uint32_t set = AllocPages(ctx, base, 32);
  if (set != 0) {
    ConstructSet(base, set);
  }
  ctx.r3.u64 = set;
  StoreU32(base, kSet, set);
}

// Accessories::SetScale: r4 the scale (16 bytes) to each held entry's
// +1072 and the set's +16.
extern "C" REX_FUNC(sub_820F0D90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0D90(ctx, base);
    return;
  }
  const uint32_t set = ctx.r3.u32, scale = ctx.r4.u32;
  for (uint32_t i = 0; i < 3; i++) {
    const uint32_t entry = LoadU32(base, set + 4 + 4 * i);
    if (entry == 0) {
      continue;
    }
    for (uint32_t w = 0; w < 16; w += 4) {
      StoreU32(base, entry + 1072 + w, LoadU32(base, scale + w));
    }
  }
  for (uint32_t w = 0; w < 16; w += 4) {
    StoreU32(base, set + 16 + w, LoadU32(base, scale + w));
  }
}

extern "C" REX_FUNC(sub_820F0E20) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0E20(ctx, base);
    return;
  }
  FreeSet(ctx, base, ctx.r3.u32);
}

// Accessories::Destroy: the set freed, if there is one (r3 the slot,
// sign-extended as the original's lis/addi leave it, otherwise).
extern "C" REX_FUNC(sub_820F0F08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0F08(ctx, base);
    return;
  }
  ctx.r3.u64 = uint64_t(int64_t(int32_t(kSet)));
  if (LoadU32(base, kSet) == 0) {
    return;
  }
  FreeSet(ctx, base, kSet);
}

// Accessories::ReleaseAll: each held entry released once.
extern "C" REX_FUNC(sub_820F0968) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0968(ctx, base);
    return;
  }
  const uint32_t set = ctx.r3.u32;
  for (uint32_t i = 0; i < 3; i++) {
    ReleaseSlot(ctx, base, set + 4 + 4 * i);
  }
}

// Accessories::Release: r4 the index (3: none); r3 left as the slot unless
// the entry is destroyed.
extern "C" REX_FUNC(sub_820F0920) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0920(ctx, base);
    return;
  }
  if (int32_t(ctx.r4.u32) == 3) {
    return;
  }
  const uint32_t slot = ctx.r3.u32 + 4 * (ctx.r4.u32 + 1);
  ctx.r3.u64 = slot;
  ReleaseSlot(ctx, base, slot);
}

extern "C" REX_FUNC(sub_820F0870) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0870(ctx, base);
    return;
  }
  const uint32_t accessory = AcquireEntry(ctx, base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);
  ctx.r3.u64 = accessory;
}

// Accessories::Acquire: AcquireEntry, then the set's scale (+16) to the
// accessory's +1024.
extern "C" REX_FUNC(sub_820F0D48) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820F0D48(ctx, base);
    return;
  }
  const uint32_t set = ctx.r3.u32;
  const uint32_t accessory = AcquireEntry(ctx, base, set, ctx.r4.u32, ctx.r5.u32);
  if (accessory != 0) {
    for (uint32_t w = 0; w < 16; w += 4) {
      StoreU32(base, accessory + 1024 + w, LoadU32(base, set + 16 + w));
    }
  }
  ctx.r3.u64 = accessory;
}
