// DataFile::GetEntry (sub_82292170) as our own C++ (--native_data_files,
// on by default): an entry of one of the game's container files. A
// container is a little-endian count at +0 and one little-endian offset an
// entry at +4 (from the container's start). The light sets, the course and
// car data and the font read their entries through it.
//
// Written from SBX's generated code; ITC-Recomp's game_data/data_file.cpp
// (graded A here: the same instructions) was the reference.

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_data_files, true, "SBX",
                    "Our own C++ for DataFile::GetEntry (an entry of the game's container files) "
                    "instead of the recompiled function");

// r3 = the container, r4 = the index, r5's low byte = whether an entry whose
// first word is 0 counts as none; r3 = the entry, or 0 when the index isn't
// below the count (unsigned). No check for a null container, as the
// original. Its scratch registers (r8 to r11) aren't read by its callers.
extern "C" REX_FUNC(sub_82292170) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_data_files)) {
    __imp__sub_82292170(ctx, base);
    return;
  }
  const uint32_t container = ctx.r3.u32;
  const uint32_t index = ctx.r4.u32;
  uint32_t entry = 0;
  if (index < __builtin_bswap32(guest::LoadU32(base, container))) {
    entry = container + __builtin_bswap32(guest::LoadU32(base, container + 4 + 4 * index));
    if ((ctx.r5.u32 & 0xFF) && guest::LoadU32(base, entry) == 0) {
      entry = 0;
    }
  }
  ctx.r3.u64 = entry;
}
