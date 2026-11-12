// The interior accessories as our own C++ (--native_accessories, on by
// default): the set at [0x826B51F0] (create, scale, acquire, release), the
// accessories' constructor and destructor, the skeleton's destructor, and
// the 36-byte AccessoryFile (its load through the archive and its
// callback). The archive (AssetSize, Load, Cancel), VirtualAlloc and
// VirtualFree, the entries' own constructors and destructors, the
// skeleton's construction, the accessory's texture and the matrix code stay
// recompiled.

#include "game/accessories.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_accessories, true, "SBX",
                    "Our own C++ for the interior accessories' set and files (create, scale, "
                    "acquire, release, the file's load and callback) instead of the recompiled "
                    "functions; the archive and the accessories' models stay recompiled");

namespace {

using game::Accessories;
using game::Accessory;
using game::AccessoryEntry;
using game::AccessoryFile;
using game::AccessorySkeleton;
using game::Vector;
using guest::At;
using guest::Guest;
using guest::ptr;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_accessories); }

constexpr uint32_t kSet = 0x826B51F0;  // [it] the set (writable)
constexpr uint32_t kEmptyVtable = 0x82000A40;  // What a destroyed base is left with.
constexpr uint32_t kFileOnArchiveDone = 0x820F0AB0;
constexpr uint32_t kEntryOnLoaded = 0x820F06B0;  // AccessoryEntry::OnLoaded
constexpr int32_t kFirstAsset = 23006;           // entry i's file is asset 23006 + i
constexpr uint32_t kAllocType = 0x00801000;      // VirtualAlloc's type (MEM_COMMIT | 0x800000)
constexpr uint32_t kPageReadWrite = 4;
constexpr uint32_t kMemRelease = 0x8000;

// VirtualAlloc (sub_82292DD8).
uint32_t AllocPages(Guest& g, uint32_t bytes) {
  return g.Call(sub_82292DD8, 0u, bytes, kAllocType, kPageReadWrite);
}

// AccessoryFile::FreeData (sub_820F0760): the pages at `block` freed
// (VirtualFree, sub_82292D70) and the word cleared; nothing for an empty
// word (r3 left as its 0).
void FreeData(Guest& g, u32_be& block) {
  g.ctx.r3.u64 = block;
  if (block == 0) {
    return;
  }
  g.Call(sub_82292D70, block, 0u, kMemRelease);
  block = 0;
}

// The file's defaults.
void ClearFile(AccessoryFile* file) {
  file->state = AccessoryFile::kNone;
  file->asset = -1;
  file->serial = -1;
  file->data = 0;
  file->size = 0;
  file->owns_data = 1;
  file->callback = 0;
  file->context = 0;
}

// AccessoryFile::Load (sub_820F0B20): the asset's size (Archive::AssetSize,
// sub_820A0860), its data (`buffer`, or pages of the file's own), then
// Archive::Load (sub_8209DF80) with OnArchiveDone and the file; its serial
// kept.
void LoadFile(Guest& g, AccessoryFile* file, int32_t asset, uint32_t buffer, uint32_t callback,
              uint32_t context) {
  file->asset = asset;
  file->size = g.Call(sub_820A0860, asset);
  if (buffer != 0) {
    file->data = buffer;
    file->owns_data = 0;
  } else {
    file->data = AllocPages(g, file->size);
    file->owns_data = 1;
  }
  const uint32_t serial = g.Call(sub_8209DF80, file->asset, file->data, kFileOnArchiveDone, file, 0u, 0u);
  file->state = AccessoryFile::kLoading;
  file->callback = callback;
  file->context = context;
  file->serial = int32_t(serial);
}

// An entry's count down by one; at 0 it's destroyed (AccessoryEntry::Destroy,
// sub_820F0818, on its slot).
void ReleaseEntry(Guest& g, ptr<AccessoryEntry>& slot) {
  if (slot == nullptr) {
    return;
  }
  slot->count -= 1;
  if (slot->count == 0) {
    g.Call(sub_820F0818, &slot);
  }
}

// Accessory::SetTexture (sub_820EF3D8), r5 = -1.
// The set's slot for an entry's index (as 32-bit guest arithmetic, as the
// original works it out, whatever the index).
ptr<AccessoryEntry>& EntrySlot(Accessories* set, uint32_t index) {
  return *guest::Element<ptr<AccessoryEntry>>(guest::AddressOf(set->entries), index);
}

void SetTexture(Guest& g, Accessory* accessory, uint32_t texture) {
  g.Call(sub_820EF3D8, accessory, texture, -1);
}

// Accessories::AcquireEntry (sub_820F0870): a new entry
// (AccessoryEntry::New, sub_820F07B0) has its texture set and its file
// loaded (asset 23006 + index, the entry's OnLoaded with the entry); a held
// one has its count up by one and its texture set. The accessory, or 0.
Accessory* AcquireEntry(Guest& g, Accessories* set, uint32_t index, uint32_t texture) {
  if (index == Accessories::kNone) {
    return nullptr;
  }
  ptr<AccessoryEntry>& slot = EntrySlot(set, index);
  if (slot == nullptr) {
    slot.address = g.Call(sub_820F07B0);
    if (slot == nullptr) {
      return nullptr;
    }
    SetTexture(g, &slot->accessory, texture);
    AccessoryEntry* entry = slot;
    LoadFile(g, &entry->file, kFirstAsset + int32_t(index), 0, kEntryOnLoaded,
             guest::AddressOf(entry));
  } else {
    slot->count += 1;
    SetTexture(g, &slot->accessory, texture);
  }
  return &slot->accessory;
}

void ConstructSet(Accessories* set) {
  set->vtable = Accessories::kBaseVtable;
  for (auto& entry : set->entries) {
    entry = nullptr;
  }
  set->vtable = Accessories::kVtable;
  set->scale.x = 1.0f;
  set->scale.y = 1.0f;
  set->scale.z = 1.0f;
  set->scale.w = 1.0f;
}

// Accessories::Free (sub_820F0E20): a set held at `slot` is destroyed (its
// vtable's slot 0 with r4 = 0) and its pages freed.
void FreeSet(Guest& g, u32_be& slot) {
  g.ctx.r3.u64 = slot;
  if (slot == 0) {
    return;
  }
  g.CallVirtual(At<Accessories>(slot), 0, 0u);
  FreeData(g, slot);
}

void SetPoint(Vector& v, float x, float y, float z, float w) {
  v.x = x;
  v.y = y;
  v.z = z;
  v.w = w;
}

}  // namespace

#define SBX_ACCESSORIES_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                 \
  if (!On()) {                         \
    __imp__##name(ctx, base);          \
    return;                            \
  }                                    \
  Guest g(ctx, base);

// --- AccessoryFile ---

// AccessoryFile::AccessoryFile: r3 the file.
extern "C" REX_FUNC(sub_820F0AE0) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0AE0)
  AccessoryFile* file = At<AccessoryFile>(ctx.r3.u32);
  file->vtable = AccessoryFile::kVtable;
  ClearFile(file);
}

// AccessoryFile::~AccessoryFile: r3 the file; reset, then the base's
// empty vtable.
extern "C" REX_FUNC(sub_820F0C48) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0C48)
  AccessoryFile* file = At<AccessoryFile>(ctx.r3.u32);
  file->vtable = AccessoryFile::kVtable;
  g.Call(sub_820F0BC0, file);  // AccessoryFile::Reset
  file->vtable = kEmptyVtable;
}

// AccessoryFile::SetData: r3 the file, r4 the data, r5 whether it's the
// file's own; the file is then loaded.
extern "C" REX_FUNC(sub_820F0A80) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0A80)
  AccessoryFile* file = At<AccessoryFile>(ctx.r3.u32);
  file->data = ctx.r4.u32;
  file->owns_data = ctx.r5.u32;
  file->size = 0;
  file->asset = -1;
  file->serial = -1;
  file->state = AccessoryFile::kLoaded;
  file->callback = 0;
  file->context = 0;
}

// AccessoryFile::Load: r3 the file, r4 the asset, r5 a buffer (0: the
// file allocates its own), r6 the callback, r7 its context.
extern "C" REX_FUNC(sub_820F0B20) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0B20)
  LoadFile(g, At<AccessoryFile>(ctx.r3.u32), int32_t(ctx.r4.u32), ctx.r5.u32, ctx.r6.u32,
           ctx.r7.u32);
}

// AccessoryFile::OnArchiveDone: the archive's callback, r3 the serial, r6
// the file. Unless the file waits on another serial, it's loaded and its
// callback runs with r6 its context (r3 to r5 as given).
extern "C" REX_FUNC(sub_820F0AB0) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0AB0)
  AccessoryFile* file = At<AccessoryFile>(ctx.r6.u32);
  if (int32_t(ctx.r3.u32) != file->serial) {
    return;
  }
  file->state = AccessoryFile::kLoaded;
  if (file->callback == 0) {
    return;
  }
  ctx.r6.u64 = file->context;
  g.CallAddress(file->callback);
}

// AccessoryFile::FreeData: r3 the word holding the pages.
extern "C" REX_FUNC(sub_820F0760) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0760)
  FreeData(g, *At<u32_be>(ctx.r3.u32));
}

// AccessoryFile::Reset: r3 the file. Its own data cancelled
// (Archive::Cancel, sub_8209DC40, while loading) or freed (once loaded),
// then its defaults.
extern "C" REX_FUNC(sub_820F0BC0) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0BC0)
  AccessoryFile* file = At<AccessoryFile>(ctx.r3.u32);
  if (file->owns_data != 0) {
    if (file->state == AccessoryFile::kLoading) {
      g.Call(sub_8209DC40, file->serial);
    } else if (file->state == AccessoryFile::kLoaded) {
      FreeData(g, file->data);
    }
  }
  ClearFile(file);
}

// --- The accessory ---

// Accessory::Accessory: r3 the accessory. Returns it.
extern "C" REX_FUNC(sub_820EF0C8) {
  SBX_ACCESSORIES_FALLBACK(sub_820EF0C8)
  Accessory* accessory = At<Accessory>(ctx.r3.u32);
  g.Call(sub_820CE6E8, &accessory->model);  // PartModel::PartModel
  accessory->model.model.vtable = Accessory::kVtable;
  g.Call(sub_820F0AE0, &accessory->files[0]);  // AccessoryFile::AccessoryFile
  g.Call(sub_820F0AE0, &accessory->files[1]);
  g.Call(sub_820F0270, &accessory->skeleton);  // AccessorySkeleton::AccessorySkeleton
  SetPoint(accessory->position, 0.0f, 0.0f, 0.0f, 1.0f);
  SetPoint(accessory->rotation, 0.0f, 0.0f, 0.0f, 1.0f);
  SetPoint(accessory->scale, 1.0f, 1.0f, 1.0f, 1.0f);
  accessory->unknown_1044 = 1;
  accessory->unknown_1040 = 0;
  accessory->unknown_1048 = 0;
  ctx.r3.u64 = guest::AddressOf(accessory);
}

// Accessory::~Accessory: r3 the accessory; its members destroyed, the
// last first.
extern "C" REX_FUNC(sub_820EF180) {
  SBX_ACCESSORIES_FALLBACK(sub_820EF180)
  Accessory* accessory = At<Accessory>(ctx.r3.u32);
  accessory->model.model.vtable = Accessory::kVtable;
  g.Call(sub_820EFE58, &accessory->skeleton);  // AccessorySkeleton::~AccessorySkeleton
  g.Call(sub_820F0C48, &accessory->files[1]);  // AccessoryFile::~AccessoryFile
  g.Call(sub_820F0C48, &accessory->files[0]);
  g.Call(sub_820CE750, &accessory->model);  // PartModel::~PartModel
}

// AccessorySkeleton::~AccessorySkeleton: r3 the skeleton. Its vtables and
// its members' put back as their destructors leave them, its file
// destroyed, then the skeleton's base (sub_82580088).
extern "C" REX_FUNC(sub_820EFE58) {
  SBX_ACCESSORIES_FALLBACK(sub_820EFE58)
  AccessorySkeleton* skeleton = At<AccessorySkeleton>(ctx.r3.u32);
  skeleton->vtable = AccessorySkeleton::kVtable;
  skeleton->vtable_176 = AccessorySkeleton::kVtable176;
  skeleton->member_240 = 0x82000AC0;
  skeleton->member_220 = 0x82000AB8;
  skeleton->member_216 = 0x82000EB0;
  g.Call(sub_820F0C48, &skeleton->file);  // AccessoryFile::~AccessoryFile
  skeleton->vtable_176 = kEmptyVtable;
  g.Call(sub_82580088, skeleton);
}

// --- The set ---

// AccessoriesBase::AccessoriesBase: r3 the set; the vtable, no entries.
extern "C" REX_FUNC(sub_820F0648) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0648)
  Accessories* set = At<Accessories>(ctx.r3.u32);
  set->vtable = Accessories::kBaseVtable;
  for (auto& entry : set->entries) {
    entry = nullptr;
  }
}

// AccessoriesBase::~AccessoriesBase: r3 the set; every entry released.
extern "C" REX_FUNC(sub_820F09E8) {
  SBX_ACCESSORIES_FALLBACK(sub_820F09E8)
  Accessories* set = At<Accessories>(ctx.r3.u32);
  set->vtable = Accessories::kBaseVtable;
  g.Call(sub_820F0968, set);  // Accessories::ReleaseAll
  set->vtable = kEmptyVtable;
}

// Accessories::Accessories: r3 the set; the base, the vtable, a scale of 1.
extern "C" REX_FUNC(sub_820F0CF0) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0CF0)
  ConstructSet(At<Accessories>(ctx.r3.u32));
}

// Accessories::New: 32 bytes of pages made a set; 0 when there are none.
extern "C" REX_FUNC(sub_820F0DD8) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0DD8)
  const uint32_t set = AllocPages(g, sizeof(Accessories));
  if (set != 0) {
    ConstructSet(At<Accessories>(set));
  }
  ctx.r3.u64 = set;
}

// Accessories::Create: the set made once.
extern "C" REX_FUNC(sub_820F0EC8) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0EC8)
  u32_be& slot = *At<u32_be>(kSet);
  if (slot != 0) {
    return;
  }
  const uint32_t set = AllocPages(g, sizeof(Accessories));
  if (set != 0) {
    ConstructSet(At<Accessories>(set));
  }
  ctx.r3.u64 = set;
  slot = set;
}

// Accessories::SetScale: r3 the set, r4 the scale, to each held entry's
// accessory and the set.
extern "C" REX_FUNC(sub_820F0D90) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0D90)
  Accessories* set = At<Accessories>(ctx.r3.u32);
  const Vector& scale = *At<Vector>(ctx.r4.u32);
  for (auto& entry : set->entries) {
    if (entry != nullptr) {
      entry->accessory.scale = scale;
    }
  }
  set->scale = scale;
}

// Accessories::Free: r3 the word holding the set.
extern "C" REX_FUNC(sub_820F0E20) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0E20)
  FreeSet(g, *At<u32_be>(ctx.r3.u32));
}

// Accessories::Destroy: the set freed, if there is one (r3 left as the
// slot's address, sign-extended as the original's lis/addi leave it).
extern "C" REX_FUNC(sub_820F0F08) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0F08)
  ctx.r3.u64 = uint64_t(int64_t(int32_t(kSet)));
  u32_be& slot = *At<u32_be>(kSet);
  if (slot == 0) {
    return;
  }
  FreeSet(g, slot);
}

// Accessories::ReleaseAll: r3 the set; each held entry released once.
extern "C" REX_FUNC(sub_820F0968) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0968)
  for (auto& entry : At<Accessories>(ctx.r3.u32)->entries) {
    ReleaseEntry(g, entry);
  }
}

// Accessories::Release: r3 the set, r4 the index (3: none); r3 left as the
// entry's slot unless it's destroyed.
extern "C" REX_FUNC(sub_820F0920) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0920)
  if (ctx.r4.u32 == Accessories::kNone) {
    return;
  }
  ptr<AccessoryEntry>& slot = EntrySlot(At<Accessories>(ctx.r3.u32), ctx.r4.u32);
  ctx.r3.u64 = guest::AddressOf(&slot);
  ReleaseEntry(g, slot);
}

// Accessories::AcquireEntry: r3 the set, r4 the index, r5 the texture.
// Returns the accessory, or 0.
extern "C" REX_FUNC(sub_820F0870) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0870)
  Accessory* accessory = AcquireEntry(g, At<Accessories>(ctx.r3.u32), ctx.r4.u32, ctx.r5.u32);
  ctx.r3.u64 = guest::AddressOf(accessory);
}

// Accessories::Acquire: AcquireEntry, then the set's scale to the
// accessory.
extern "C" REX_FUNC(sub_820F0D48) {
  SBX_ACCESSORIES_FALLBACK(sub_820F0D48)
  Accessories* set = At<Accessories>(ctx.r3.u32);
  Accessory* accessory = AcquireEntry(g, set, ctx.r4.u32, ctx.r5.u32);
  if (accessory != nullptr) {
    accessory->scale = set->scale;
  }
  ctx.r3.u64 = guest::AddressOf(accessory);
}
