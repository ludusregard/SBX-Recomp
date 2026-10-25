// The game's CreateFile (sub_82292F48; ITC sub_82295508, exact), the only
// caller of NtCreateFile. Ported from ITC-Recomp (src/hooks/create_file.cpp).
//
// --log_file_opens logs every file the game opens through it. (ITC: failing
// the boot logo movie's open to skip it doesn't work, the game shows the
// dirty disc error; --skip_intro does it instead.)

#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <string>

REXCVAR_DEFINE_BOOL(log_file_opens, false, "SBX", "Log every file the game opens");

// CreateFile: r3 = path; returns the handle, or -1.
extern "C" REX_FUNC(sub_82292F48) {
  REX_FUNC_PROLOGUE();
  if (REXCVAR_GET(log_file_opens)) {
    std::string path;
    for (uint32_t p = ctx.r3.u32; p && path.size() < 260; p++) {
      char c = char(guest::LoadU8(base, p));
      if (!c) {
        break;
      }
      path += c;
    }
    REXLOG_INFO("file open: {} (from {:08X})", path, uint32_t(ctx.lr));
  }
  __imp__sub_82292F48(ctx, base);
}
