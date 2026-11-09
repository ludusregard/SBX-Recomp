// The data tables as our own C++ (--native_data_tables, on by default):
// twelve assets loaded once at boot into one block, their entries
// byte-swapped in place by the loads' callbacks, then read by index all
// through the game (the item records, asset 94, are what the shop, the
// parts lists and the battle music read).
//
// Written from SBX's generated code. The item records are 3,004 bytes, and
// the table object is at [0x826B5980].
//
// The table object (64 bytes): +0 the last load's serial, +4 the loads
// pending, +8 the item count, +12 the block, +16 the item records (asset
// 94), +20 to +48 eight 56-byte tables (their asset ids at 0x82606BEC), +52
// asset 95's 20-byte table, +56 asset 96's 40-byte table, +60 asset 84.
//
// The swaps: the originals swap each word through a float register (lfs,
// stfs to the stack, the bytes stored reversed). Here the integer is
// swapped, as the 360 does it: the recompiled round trip through a double
// sets the quiet bit of a word that reads as a signaling NaN. SwapWord
// itself (sub_820C9578) is shared with other loaders and stays recompiled.

#include "guest.h"

#include <rex/cvar.h>

#include <bit>

REXCVAR_DEFINE_BOOL(native_data_tables, true, "SBX",
                    "Our own C++ for the data tables (the item records and the tables loaded "
                    "with them at boot: their loads, byte swaps and lookups) instead of the "
                    "recompiled functions; the archive's loads stay recompiled");

namespace {

constexpr uint32_t kTables = 0x826B5980;  // [it] the table object
constexpr uint32_t kTableIds = 0x82606BEC;  // the eight 56-byte tables' asset ids
constexpr uint32_t kItemBytes = 3004;
constexpr uint32_t kItemCount = 400;

// The loads' callbacks (DataTables::On...Loaded).
constexpr uint32_t kOnItemsLoaded = 0x82117240;
constexpr uint32_t kOnLoaded84 = 0x82117228;
constexpr uint32_t kOn56Loaded = 0x82117258;
constexpr uint32_t kOn20Loaded = 0x82117158;
constexpr uint32_t kOn40Loaded = 0x821171C0;

uint32_t Tables(uint8_t* base) { return guest::LoadU32(base, kTables); }

void SwapWords(uint8_t* base, uint32_t address, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    const uint32_t a = address + 4 * i;
    guest::StoreU32(base, a, std::byteswap(guest::LoadU32(base, a)));
  }
}

// The entries' swaps: every word. Each returns the address of the last word
// swapped (what the original leaves in r3).
uint32_t Swap20(uint8_t* base, uint32_t entry) { SwapWords(base, entry, 5); return entry + 16; }
uint32_t Swap40(uint8_t* base, uint32_t entry) { SwapWords(base, entry, 10); return entry + 36; }
uint32_t Swap56(uint8_t* base, uint32_t entry) { SwapWords(base, entry, 14); return entry + 52; }

void CountDown(uint8_t* base, uint32_t object) {
  guest::StoreU32(base, object + 4, guest::LoadU32(base, object + 4) - 1);
}

// A table's entries swapped up to and including the one whose word at
// `end_at` reads -1 once swapped, then the loads pending counted down.
uint32_t SwapTable(uint8_t* base, uint32_t table, uint32_t bytes, uint32_t end_at,
                   uint32_t (*swap)(uint8_t*, uint32_t)) {
  uint32_t r3;
  for (uint32_t entry = table;; entry += bytes) {
    r3 = swap(base, entry);
    if (int32_t(guest::LoadU32(base, entry + end_at)) == -1) {
      break;
    }
  }
  CountDown(base, Tables(base));
  return r3;
}

uint32_t Toc(PPCContext& ctx, uint8_t* base, uint32_t id) {
  ctx.r3.u64 = id;
  sub_8209D988(ctx, base);
  return ctx.r3.u32;
}

// An asset's size from its TOC entry (+16 x id): the unpacked size at +24,
// or the stored one at +20 when that's 0.
uint32_t TocSize(uint8_t* base, uint32_t toc, uint32_t id) {
  const uint32_t unpacked = guest::LoadU32(base, toc + 16 * id + 24);
  return unpacked != 0 ? unpacked : guest::LoadU32(base, toc + 16 * id + 20);
}

// Archive::Load (sub_8209DF80): asset `id` into `to`, `callback` called
// with r6 = `context` once it's loaded; its serial.
uint32_t Load(PPCContext& ctx, uint8_t* base, uint32_t id, uint32_t to, uint32_t callback,
              uint32_t context) {
  ctx.r3.u64 = id;
  ctx.r4.u64 = to;
  ctx.r5.u64 = callback;
  ctx.r6.u64 = context;
  ctx.r7.u64 = 0;
  ctx.r8.u64 = 0;
  sub_8209DF80(ctx, base);
  return ctx.r3.u32;
}

}  // namespace

#define SBX_DATA_TABLES_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                     \
  if (!REXCVAR_GET(native_data_tables)) {  \
    __imp__##name(ctx, base);              \
    return;                                \
  }

// --- The set-up and the loads (once, at boot) ---

// DataTables::Init: the object made (operator new) into [kTables], then two
// passes over the twelve assets, each table's pointer the block (+12) plus
// the sizes before it (0 while there's no block), +4 counted up from 0 for
// each; between the passes the block (operator new[] of the total). r3 =
// the last TOC lookup's, as the original leaves it.
extern "C" REX_FUNC(sub_82116F18) {
  SBX_DATA_TABLES_FALLBACK(sub_82116F18)
  ctx.r3.u64 = 64;
  sub_8258C000(ctx, base);
  const uint32_t object = ctx.r3.u32;
  guest::StoreU32(base, kTables, object);
  guest::StoreU32(base, object + 12, 0);
  guest::StoreU32(base, Tables(base) + 4, 0);
  guest::StoreU32(base, Tables(base) + 8, 0);
  if (guest::LoadU32(base, Tables(base) + 12) != 0) {
    return;
  }
  auto count_up = [&] {
    const uint32_t t = Tables(base);
    guest::StoreU32(base, t + 4, guest::LoadU32(base, t + 4) + 1);
  };
  // The table after `size` bytes of the block, or 0 while there's no block
  // (`at` the table before it, 0 then too).
  auto after = [&](uint32_t at, uint32_t size) {
    return at != 0 ? guest::LoadU32(base, Tables(base) + 12) + size : 0;
  };
  uint32_t total = 0;
  uint32_t toc = 0;
  for (;;) {
    if (total != 0) {
      ctx.r3.u64 = total;
      sub_8258C0C0(ctx, base);
      guest::StoreU32(base, Tables(base) + 12, ctx.r3.u32);
    }
    uint32_t table = guest::LoadU32(base, Tables(base) + 12);
    guest::StoreU32(base, Tables(base) + 4, 0);
    count_up();
    guest::StoreU32(base, Tables(base) + 16, table);
    toc = Toc(ctx, base, 94);
    uint32_t size = TocSize(base, toc, 94);
    table = after(table, size);

    count_up();
    guest::StoreU32(base, Tables(base) + 60, table);
    toc = Toc(ctx, base, 84);
    size += TocSize(base, toc, 84);
    table = after(table, size);

    for (uint32_t k = 0; k < 8; k++) {
      count_up();
      guest::StoreU32(base, Tables(base) + 20 + 4 * k, table);
      ctx.r3.u64 = guest::LoadU32(base, kTableIds + 4 * k);
      sub_820A0860(ctx, base);  // Archive::AssetSize
      size += ctx.r3.u32;
      table = after(table, size);
    }

    count_up();
    guest::StoreU32(base, Tables(base) + 52, table);
    toc = Toc(ctx, base, 95);
    size += TocSize(base, toc, 95);
    table = after(table, size);

    count_up();
    guest::StoreU32(base, Tables(base) + 56, table);
    toc = Toc(ctx, base, 96);
    total = TocSize(base, toc, 96) + size;
    if (guest::LoadU32(base, Tables(base) + 12) != 0) {
      break;
    }
  }
  ctx.r3.u64 = toc;
}

// DataTables::Load: the twelve loads into the tables Init set, each serial
// stored at +0. r3 = the last serial.
extern "C" REX_FUNC(sub_821173A8) {
  SBX_DATA_TABLES_FALLBACK(sub_821173A8)
  auto store = [&](uint32_t serial) { guest::StoreU32(base, Tables(base), serial); };
  uint32_t t = Tables(base);
  store(Load(ctx, base, 94, guest::LoadU32(base, t + 16), kOnItemsLoaded, t));
  t = Tables(base);
  store(Load(ctx, base, 84, guest::LoadU32(base, t + 60), kOnLoaded84, t));
  for (uint32_t k = 0; k < 8; k++) {
    const uint32_t table = guest::LoadU32(base, Tables(base) + 20 + 4 * k);
    store(Load(ctx, base, guest::LoadU32(base, kTableIds + 4 * k), table, kOn56Loaded, table));
  }
  uint32_t table = guest::LoadU32(base, Tables(base) + 52);
  store(Load(ctx, base, 95, table, kOn20Loaded, table));
  table = guest::LoadU32(base, Tables(base) + 56);
  const uint32_t serial = Load(ctx, base, 96, table, kOn40Loaded, table);
  store(serial);
  ctx.r3.u64 = serial;
}

// --- The loads' callbacks (r6 = the context) ---

// Asset 94: the item count (400) and the loads pending. r3 left.
extern "C" REX_FUNC(sub_82117240) {
  SBX_DATA_TABLES_FALLBACK(sub_82117240)
  guest::StoreU32(base, ctx.r6.u32 + 8, kItemCount);
  CountDown(base, ctx.r6.u32);
}

// Asset 84: nothing to swap. r3 left.
extern "C" REX_FUNC(sub_82117228) {
  SBX_DATA_TABLES_FALLBACK(sub_82117228)
  CountDown(base, Tables(base));
}

// The eight 56-byte tables: up to the entry whose +0 is -1.
extern "C" REX_FUNC(sub_82117258) {
  SBX_DATA_TABLES_FALLBACK(sub_82117258)
  ctx.r3.u64 = SwapTable(base, ctx.r6.u32, 56, 0, Swap56);
}

// Asset 95's 20-byte table: up to the entry whose +0 is -1.
extern "C" REX_FUNC(sub_82117158) {
  SBX_DATA_TABLES_FALLBACK(sub_82117158)
  ctx.r3.u64 = SwapTable(base, ctx.r6.u32, 20, 0, Swap20);
}

// Asset 96's 40-byte table: up to the entry whose +4 is -1.
extern "C" REX_FUNC(sub_821171C0) {
  SBX_DATA_TABLES_FALLBACK(sub_821171C0)
  ctx.r3.u64 = SwapTable(base, ctx.r6.u32, 40, 4, Swap40);
}

// The entries' swaps on their own (r3 = the entry).
extern "C" REX_FUNC(sub_82111A70) {
  SBX_DATA_TABLES_FALLBACK(sub_82111A70)
  ctx.r3.u64 = Swap20(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_82111AC0) {
  SBX_DATA_TABLES_FALLBACK(sub_82111AC0)
  ctx.r3.u64 = Swap56(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_82112A30) {
  SBX_DATA_TABLES_FALLBACK(sub_82112A30)
  ctx.r3.u64 = Swap40(base, ctx.r3.u32);
}

// --- The lookups (no bounds checks, as the originals) ---

// DataTables::IsLoading: 1 while loads are pending.
extern "C" REX_FUNC(sub_821172C0) {
  SBX_DATA_TABLES_FALLBACK(sub_821172C0)
  ctx.r3.u64 = guest::LoadU32(base, Tables(base) + 4) != 0 ? 1 : 0;
}

// DataTables::Item: r3 = the item; its record.
extern "C" REX_FUNC(sub_821172E0) {
  SBX_DATA_TABLES_FALLBACK(sub_821172E0)
  ctx.r3.u64 = uint32_t(guest::LoadU32(base, Tables(base) + 16) + ctx.r3.u32 * kItemBytes);
}

// ItemRecords::Item: r4 = the item (r3 not read).
extern "C" REX_FUNC(sub_82116F10) {
  SBX_DATA_TABLES_FALLBACK(sub_82116F10)
  ctx.r3.u64 = uint32_t(guest::LoadU32(base, Tables(base) + 16) + ctx.r4.u32 * kItemBytes);
}

// DataTables::Entry56: r3 = the table (0 to 7), r4 = the entry.
extern "C" REX_FUNC(sub_821172F8) {
  SBX_DATA_TABLES_FALLBACK(sub_821172F8)
  const uint32_t table = guest::LoadU32(base, Tables(base) + (ctx.r3.u32 + 5) * 4);
  ctx.r3.u64 = uint32_t(table + ctx.r4.u32 * 56);
}

// DataTables::Find20: r3 = the +8 to find, r4 = the +0; the first such
// entry's index of asset 95's table, or -1.
extern "C" REX_FUNC(sub_82117318) {
  SBX_DATA_TABLES_FALLBACK(sub_82117318)
  const int32_t key8 = ctx.r3.s32;
  const int32_t key0 = ctx.r4.s32;
  int64_t found = -1;
  uint32_t entry = guest::LoadU32(base, Tables(base) + 52);
  for (int32_t i = 0;; i++, entry += 20) {
    const int32_t first = int32_t(guest::LoadU32(base, entry));
    if (first == -1) {
      break;
    }
    if (int32_t(guest::LoadU32(base, entry + 8)) == key8 && first == key0) {
      found = i;
      break;
    }
  }
  ctx.r3.s64 = found;
}

// DataTables::Entry20 and Entry40: r3 = the entry.
extern "C" REX_FUNC(sub_82117378) {
  SBX_DATA_TABLES_FALLBACK(sub_82117378)
  ctx.r3.u64 = uint32_t(guest::LoadU32(base, Tables(base) + 52) + ctx.r3.u32 * 20);
}

extern "C" REX_FUNC(sub_82117390) {
  SBX_DATA_TABLES_FALLBACK(sub_82117390)
  ctx.r3.u64 = uint32_t(guest::LoadU32(base, Tables(base) + 56) + ctx.r3.u32 * 40);
}
