// The file layer's objects as our own C++ (--native_file_layer, on by
// default): the file classes' constructors and destructors (GameFile over
// AsyncFile over File) and the path objects (Path, the game's GamePath),
// which the game builds its file names in. The objects live on the
// recompiled callers' stacks, so these work on guest memory.
//
// Written from SBX's generated code. The file opens, reads and the archive
// stay recompiled (and the patch loader's and --log_file_opens' hooks on
// them still apply).

#include "guest.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_file_layer, true, "SBX",
                    "Our own C++ for the file layer's objects (the file classes' constructors "
                    "and destructors, Path::Build, GamePath::SetGame) instead of the recompiled "
                    "functions; opening and reading files stays recompiled");

namespace {

// The vtables, from the game's class down.
constexpr uint32_t kGameFileVtable = 0x82002228;
constexpr uint32_t kAsyncFileVtable = 0x820699EC;
constexpr uint32_t kFileVtable = 0x82068E3C;
constexpr uint32_t kStreamVtable = 0x82068DD8;
constexpr uint32_t kPathVtable = 0x82069A1C;
constexpr uint32_t kGamePathVtable = 0x82002258;

// Path::Build's formats (sprintf), by the kind, and the two strings the
// third one puts before the name (std::strings: the text inline at +4 while
// the capacity, +24, is below 16, else a pointer at +4).
constexpr uint32_t kFormat0 = 0x82069A28;
constexpr uint32_t kFormat1 = 0x82069A34;
constexpr uint32_t kFormat2 = 0x82069A40;
constexpr uint32_t kString1 = 0x8264EBF8;
constexpr uint32_t kString2 = 0x8264EBDC;

// File::File: +4 = -1 (closed), +8 the position and +12 the size = 0.
void FileConstruct(uint8_t* base, uint32_t file) {
  guest::StoreU32(base, file + 8, 0);
  guest::StoreU32(base, file + 0, kFileVtable);
  guest::StoreU32(base, file + 4, 0xFFFFFFFF);
  guest::StoreU32(base, file + 12, 0);
}

void AsyncFileConstruct(uint8_t* base, uint32_t file) {
  FileConstruct(base, file);
  guest::StoreU32(base, file, kAsyncFileVtable);
}

void GameFileConstruct(uint8_t* base, uint32_t file) {
  AsyncFileConstruct(base, file);
  guest::StoreU32(base, file, kGameFileVtable);
}

// The destructors only put the vtables back, down to the stream's; the
// handle isn't closed.
void FileDestruct(uint8_t* base, uint32_t file) { guest::StoreU32(base, file, kStreamVtable); }

void AsyncFileDestruct(uint8_t* base, uint32_t file) {
  guest::StoreU32(base, file, kAsyncFileVtable);
  FileDestruct(base, file);
}

void GameFileDestruct(uint8_t* base, uint32_t file) {
  guest::StoreU32(base, file, kGameFileVtable);
  AsyncFileDestruct(base, file);
}

// Path::Path: the vtable, the text (+4) empty.
void PathConstruct(uint8_t* base, uint32_t path) {
  guest::StoreU8(base, path + 4, 0);
  guest::StoreU32(base, path, kPathVtable);
}

uint32_t StringText(uint8_t* base, uint32_t string) {
  return guest::LoadU32(base, string + 24) >= 16 ? guest::LoadU32(base, string + 4) : string + 4;
}

// Path::Build: r3 = the path, r4 = the kind, r5 = the name. The text (+4)
// printed from the kind's format: 0 and 1 the name alone, 2 the two strings
// and the name; 3 and above nothing. r3 = the text, or 0 when the name is 0
// (nothing printed).
uint32_t PathBuild(PPCContext& ctx, uint8_t* base, uint32_t path, uint32_t kind,
                   uint32_t name) {
  if (name == 0) {
    return 0;
  }
  if (kind < 3) {
    ctx.r3.u64 = path + 4;
    if (kind == 2) {
      const uint32_t first = StringText(base, kString1);
      const uint32_t second = StringText(base, kString2);
      ctx.r4.u64 = kFormat2;
      ctx.r5.u64 = second;
      ctx.r6.u64 = first;
      ctx.r7.u64 = name;
    } else {
      ctx.r4.u64 = kind == 0 ? kFormat0 : kFormat1;
      ctx.r5.u64 = name;
    }
    guest_call::InFrame(ctx, base, 96, kind == 2 ? 0x825792F0 : 0x82579310,
                        [&](uint32_t) { sub_8258ED70(ctx, base); });  // sprintf
  }
  return path + 4;
}

}  // namespace

#define SBX_FILE_LAYER_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                    \
  if (!REXCVAR_GET(native_file_layer)) {  \
    __imp__##name(ctx, base);             \
    return;                               \
  }

// The constructors and destructors: r3 = the object, and r3 left as it.

extern "C" REX_FUNC(sub_82572488) {  // File::File
  SBX_FILE_LAYER_FALLBACK(sub_82572488)
  FileConstruct(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_82572130) {  // File::~File
  SBX_FILE_LAYER_FALLBACK(sub_82572130)
  FileDestruct(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_82579070) {  // AsyncFile::AsyncFile
  SBX_FILE_LAYER_FALLBACK(sub_82579070)
  AsyncFileConstruct(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_825790B0) {  // AsyncFile::~AsyncFile
  SBX_FILE_LAYER_FALLBACK(sub_825790B0)
  AsyncFileDestruct(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820A69B8) {  // GameFile::GameFile
  SBX_FILE_LAYER_FALLBACK(sub_820A69B8)
  GameFileConstruct(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_820A69F8) {  // GameFile::~GameFile
  SBX_FILE_LAYER_FALLBACK(sub_820A69F8)
  GameFileDestruct(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_825791D8) {  // Path::Path
  SBX_FILE_LAYER_FALLBACK(sub_825791D8)
  PathConstruct(base, ctx.r3.u32);
}

extern "C" REX_FUNC(sub_825791F8) {  // Path::~Path: the vtable written again
  SBX_FILE_LAYER_FALLBACK(sub_825791F8)
  guest::StoreU32(base, ctx.r3.u32, kPathVtable);
}

extern "C" REX_FUNC(sub_820A6B98) {  // GamePath::GamePath (also its destructor)
  SBX_FILE_LAYER_FALLBACK(sub_820A6B98)
  guest::StoreU32(base, ctx.r3.u32, kGamePathVtable);
}

// Path::Build: r3 = the path, r4 = the kind, r5 = the name.
extern "C" REX_FUNC(sub_82579250) {
  SBX_FILE_LAYER_FALLBACK(sub_82579250)
  ctx.r3.u64 = PathBuild(ctx, base, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);
}

// GamePath::SetGame: r3 = the game's path, r4 = the name. A Path built in
// the original's frame (+80 of 624 bytes) with kind 0, its text copied to
// the game's (+4, up to and with the terminator), the Path destroyed. r3 =
// the game's text. As the original, a name of 0 isn't checked for (the
// copy reads from address 0).
extern "C" REX_FUNC(sub_820A6BA8) {
  SBX_FILE_LAYER_FALLBACK(sub_820A6BA8)
  const uint32_t game = ctx.r3.u32;
  const uint32_t name = ctx.r4.u32;
  guest_call::InFrame(ctx, base, 624, 0x820A6BDC, [&](uint32_t frame) {
    const uint32_t path = frame + 80;
    PathConstruct(base, path);
    uint32_t from = PathBuild(ctx, base, path, 0, name);
    uint32_t to = game + 4;
    for (;;) {
      const uint8_t c = guest::LoadU8(base, from++);
      guest::StoreU8(base, to++, c);
      if (c == 0) {
        break;
      }
    }
    guest::StoreU32(base, path, kPathVtable);
  });
  ctx.r3.u64 = game + 4;
}
