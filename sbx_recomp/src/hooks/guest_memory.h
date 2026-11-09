// Small helpers for the native game code (src/hooks/): guest memory filled
// or copied as the C runtime's functions would, and the recompiled C
// runtime called where a copy has to go through it.
//
// Guest memory at 0xE0000000 and up sits 4 KB further on in host memory
// than the rest (REX_PHYS_HOST_OFFSET), so a range that crosses it is done
// in two parts.

#pragma once

#include "guest.h"

#include <cstring>

namespace guest_memory {

constexpr uint32_t kOffsetChange = 0xE0000000u;

// Calls fn(host pointer, bytes) for each part of [address, address + bytes)
// that's contiguous in host memory.
template <typename Fn>
void ForEachPart(uint8_t* base, uint32_t address, uint32_t bytes, Fn fn) {
  if (bytes == 0) {
    return;
  }
  if (address < kOffsetChange && uint64_t(address) + bytes > kOffsetChange) {
    const uint32_t below = kOffsetChange - address;
    fn(REX_RAW_ADDR(address), below);
    fn(REX_RAW_ADDR(kOffsetChange), bytes - below);
    return;
  }
  fn(REX_RAW_ADDR(address), bytes);
}

// memset(address, 0, bytes), as the C runtime's memset (sub_8258C950)
// leaves it.
inline void Clear(uint8_t* base, uint32_t address, uint32_t bytes) {
  ForEachPart(base, address, bytes, [](uint8_t* host, uint32_t n) { std::memset(host, 0, n); });
}

// memset (sub_8258C950): r3 = address, r4 = value, r5 = bytes.
inline void Memset(PPCContext& ctx, uint8_t* base, uint32_t address, uint32_t value,
                   uint32_t bytes) {
  ctx.r3.u64 = address;
  ctx.r4.u64 = value;
  ctx.r5.u64 = bytes;
  sub_8258C950(ctx, base);
}

// memcpy (sub_8258CB90, wrapped by the native renderer, which records
// copies into the physical views): r3 = to, r4 = from, r5 = bytes. Any copy
// that may land in a buffer the renderer reads goes through here.
inline void Memcpy(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t from, uint32_t bytes) {
  ctx.r3.u64 = to;
  ctx.r4.u64 = from;
  ctx.r5.u64 = bytes;
  sub_8258CB90(ctx, base);
}

// XMemCpy (sub_822932D0): the library's copy (vector moves, cache line
// zeroing); r3 = to, r4 = from, r5 = bytes. Returns to.
inline uint32_t XMemCpy(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t from,
                        uint32_t bytes) {
  ctx.r3.u64 = to;
  ctx.r4.u64 = from;
  ctx.r5.u64 = bytes;
  sub_822932D0(ctx, base);
  return ctx.r3.u32;
}

// A store of a table-driven fill (the originals' long runs of stores,
// written as tables in their order). The value: a word, halfword or byte;
// the word at an image address (the originals' lfs/stfs and lwz/stw of the
// image's constants, which copy its bits); an argument register (r4 to
// r10, as the function got it); or the object's own address plus `value`.
// The kAbs kinds store to the absolute address `offset` instead of the
// object's. `count` stores `stride` bytes apart (the originals' loops, or
// repeated stores).
enum FillKind : uint8_t {
  kWord, kHalf, kByte, kImage, kArg, kSelf,
  kAbsWord, kAbsHalf, kAbsByte, kAbsImage, kAbsArg, kAbsSelf,
};
struct Fill {
  uint32_t offset;
  FillKind kind;
  uint32_t value;
  uint32_t count = 1;
  uint32_t stride = 0;
};

// The argument registers r4 to r10 as the function got them, kept before
// any call changes them.
struct Args {
  uint32_t r[7];
  explicit Args(const PPCContext& ctx)
      : r{ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32, ctx.r8.u32, ctx.r9.u32,
          ctx.r10.u32} {}
  uint32_t operator[](uint32_t n) const { return r[n - 4]; }
};

// `args` is needed only by tables with kArg stores.
template <size_t N>
void Apply(uint8_t* base, uint32_t object, const Fill (&fills)[N],
           const Args* args = nullptr) {
  for (const Fill& f : fills) {
    const bool absolute = f.kind >= kAbsWord;
    const FillKind kind = absolute ? FillKind(f.kind - kAbsWord) : f.kind;
    uint32_t value = f.value;
    if (kind == kImage) {
      value = guest::LoadU32(base, f.value);
    } else if (kind == kArg) {
      value = (*args)[f.value];
    } else if (kind == kSelf) {
      value = object + f.value;
    }
    uint32_t at = absolute ? f.offset : object + f.offset;
    for (uint32_t i = 0; i < f.count; i++, at += f.stride) {
      switch (kind) {
        case kHalf: guest::StoreU16(base, at, uint16_t(value)); break;
        case kByte: guest::StoreU8(base, at, uint8_t(value)); break;
        default: guest::StoreU32(base, at, value); break;
      }
    }
  }
}

template <size_t N>
void Apply(uint8_t* base, uint32_t object, const Fill (&fills)[N], const Args& args) {
  Apply(base, object, fills, &args);
}

// A run of words copied from one object to another at the same offsets,
// lowest first (as the originals' field-by-field copies go).
struct Run {
  uint32_t offset;
  uint32_t bytes;
};

inline void CopyRun(uint8_t* base, uint32_t to, uint32_t from, const Run& run) {
  for (uint32_t i = 0; i < run.bytes; i += 4) {
    guest::StoreU32(base, to + run.offset + i, guest::LoadU32(base, from + run.offset + i));
  }
}

// Field-by-field copies, lowest first, as the originals' runs of loads and
// stores go: `bytes` bytes as words, halfwords or bytes.
inline void CopyWords(uint8_t* base, uint32_t to, uint32_t from, uint32_t bytes) {
  for (uint32_t i = 0; i < bytes; i += 4) {
    guest::StoreU32(base, to + i, guest::LoadU32(base, from + i));
  }
}

inline void CopyHalves(uint8_t* base, uint32_t to, uint32_t from, uint32_t bytes) {
  for (uint32_t i = 0; i < bytes; i += 2) {
    guest::StoreU16(base, to + i, guest::LoadU16(base, from + i));
  }
}

inline void CopyBytes(uint8_t* base, uint32_t to, uint32_t from, uint32_t bytes) {
  for (uint32_t i = 0; i < bytes; i++) {
    guest::StoreU8(base, to + i, guest::LoadU8(base, from + i));
  }
}

}  // namespace guest_memory
