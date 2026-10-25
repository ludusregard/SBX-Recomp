// Hashing for the native renderer: XXH3-64, inlined, from the SDK's xxHash
// (rexglue-sdk/thirdparty/xxHash, on the include path with the renderer).
// From ITC-Recomp, whose copy is its own.

#pragma once

#include <cstddef>
#include <cstdint>

// Inlined, as the SDK's rex/hash.h sets it up. Not XXH_X86DISPATCH: its
// first call writes the dispatch pointers unsynchronized, and we hash on
// several threads.
#ifndef XXH_INLINE_ALL
#define XXH_INLINE_ALL
#endif
#include <xxhash.h>

namespace base {

inline uint64_t Hash64(const void* data, size_t size) { return XXH3_64bits(data, size); }

inline uint64_t Hash64(const void* data, size_t size, uint64_t seed) {
  return XXH3_64bits_withSeed(data, size, seed);
}

}  // namespace base
