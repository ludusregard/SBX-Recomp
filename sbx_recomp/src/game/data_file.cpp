// DataFile::GetEntry as our own C++ (--native_data_files, on by default):
// an entry of one of the game's container files. The light sets, the course
// and car data and the font read their entries through it.

#include "game/data_file.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_data_files, true, "SBX",
                    "Our own C++ for DataFile::GetEntry (an entry of the game's container files) "
                    "instead of the recompiled function");

using game::DataFile;

// r3 = the container, r4 = the index, r5's low byte = whether to skip an
// empty entry; r3 = the entry or 0.
extern "C" REX_FUNC(sub_82292170) {
  REX_FUNC_PROLOGUE();
  if (!REXCVAR_GET(native_data_files)) {
    __imp__sub_82292170(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  DataFile* file = guest::At<DataFile>(ctx.r3.u32);
  ctx.r3.u64 = guest::AddressOf(file->Entry(ctx.r4.u32, (ctx.r5.u32 & 0xFF) != 0));
}
