// The file layer's objects as our own C++ (--native_file_layer, on by
// default): the file classes' constructors and destructors (GameFile over
// AsyncFile over File) and the path objects (Path, the game's GamePath),
// which the game builds its file names in. The objects live on the
// recompiled callers' stacks.
//
// The file opens, reads and the archive stay recompiled (and the patch
// loader's and --log_file_opens' hooks on them still apply).

#include "game/std_types.h"
#include "hooks/guest_call.h"

#include <rex/cvar.h>

#include <cstring>

REXCVAR_DEFINE_BOOL(native_file_layer, true, "SBX",
                    "Our own C++ for the file layer's objects (the file classes' constructors "
                    "and destructors, Path::Build, GamePath::SetGame) instead of the recompiled "
                    "functions; opening and reading files stays recompiled");

namespace {

using game::String;
using guest::At;
using guest::Guest;
using guest::i32_be;
using guest::u32_be;

// The classes' vtables, from the game's class down.
constexpr uint32_t kGameFileVtable = 0x82002228;
constexpr uint32_t kAsyncFileVtable = 0x820699EC;
constexpr uint32_t kFileVtable = 0x82068E3C;
constexpr uint32_t kStreamVtable = 0x82068DD8;
constexpr uint32_t kPathVtable = 0x82069A1C;
constexpr uint32_t kGamePathVtable = 0x82002258;

// File, and AsyncFile and GameFile over it (they add no fields here).
struct File {
  u32_be vtable;
  i32_be handle;  // -1 while closed.
  u32_be position;
  u32_be size;

  void Construct(uint32_t class_vtable) {
    position = 0;
    handle = -1;
    size = 0;
    vtable = class_vtable;
  }
  // The destructors only step the vtable down to the stream's; the handle
  // isn't closed.
  void Destroy() { vtable = kStreamVtable; }
};

// Path, and GamePath over it: a file name built in place.
struct Path {
  u32_be vtable;
  char text[540];
};

enum PathKind : uint32_t {
  kPathPlain = 0,
  kPathPlain2 = 1,
  kPathWithFolders = 2,  // The two folder strings below, then the name.
};
constexpr uint32_t kPlainFormat = 0x82069A28;
constexpr uint32_t kPlain2Format = 0x82069A34;
constexpr uint32_t kFoldersFormat = 0x82069A40;
constexpr uint32_t kFolderString1 = 0x8264EBF8;  // std::strings
constexpr uint32_t kFolderString2 = 0x8264EBDC;

// Path::Build: prints the path's text from `name` by the kind (kinds past
// the three print nothing). Returns the text, or nullptr for no name.
char* BuildPath(Guest& g, Path* path, uint32_t kind, const char* name) {
  if (name == nullptr) {
    return nullptr;
  }
  if (kind == kPathWithFolders) {
    guest_call::InFrame(g.ctx, g.base, 96, 0x825792F0, [&](uint32_t) {
      g.Call(sub_8258ED70, path->text, kFoldersFormat, At<String>(kFolderString2)->Text(),
             At<String>(kFolderString1)->Text(), name);  // sprintf
    });
  } else if (kind == kPathPlain || kind == kPathPlain2) {
    guest_call::InFrame(g.ctx, g.base, 96, 0x82579310, [&](uint32_t) {
      g.Call(sub_8258ED70, path->text, kind == kPathPlain ? kPlainFormat : kPlain2Format, name);
    });
  }
  return path->text;
}

}  // namespace

#define SBX_FILE_LAYER_FALLBACK(name)     \
  REX_FUNC_PROLOGUE();                    \
  if (!REXCVAR_GET(native_file_layer)) {  \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// The constructors and destructors: r3 = the object, and r3 left as it.

extern "C" REX_FUNC(sub_82572488) {  // File::File
  SBX_FILE_LAYER_FALLBACK(sub_82572488)
  At<File>(ctx.r3.u32)->Construct(kFileVtable);
}

extern "C" REX_FUNC(sub_82572130) {  // File::~File
  SBX_FILE_LAYER_FALLBACK(sub_82572130)
  At<File>(ctx.r3.u32)->Destroy();
}

extern "C" REX_FUNC(sub_82579070) {  // AsyncFile::AsyncFile
  SBX_FILE_LAYER_FALLBACK(sub_82579070)
  At<File>(ctx.r3.u32)->Construct(kAsyncFileVtable);
}

extern "C" REX_FUNC(sub_825790B0) {  // AsyncFile::~AsyncFile
  SBX_FILE_LAYER_FALLBACK(sub_825790B0)
  At<File>(ctx.r3.u32)->Destroy();
}

extern "C" REX_FUNC(sub_820A69B8) {  // GameFile::GameFile
  SBX_FILE_LAYER_FALLBACK(sub_820A69B8)
  At<File>(ctx.r3.u32)->Construct(kGameFileVtable);
}

extern "C" REX_FUNC(sub_820A69F8) {  // GameFile::~GameFile
  SBX_FILE_LAYER_FALLBACK(sub_820A69F8)
  At<File>(ctx.r3.u32)->Destroy();
}

extern "C" REX_FUNC(sub_825791D8) {  // Path::Path
  SBX_FILE_LAYER_FALLBACK(sub_825791D8)
  Path* path = At<Path>(ctx.r3.u32);
  path->text[0] = 0;
  path->vtable = kPathVtable;
}

extern "C" REX_FUNC(sub_825791F8) {  // Path::~Path
  SBX_FILE_LAYER_FALLBACK(sub_825791F8)
  At<Path>(ctx.r3.u32)->vtable = kPathVtable;
}

extern "C" REX_FUNC(sub_820A6B98) {  // GamePath::GamePath (also its destructor)
  SBX_FILE_LAYER_FALLBACK(sub_820A6B98)
  At<Path>(ctx.r3.u32)->vtable = kGamePathVtable;
}

// Path::Build: r3 = the path, r4 = the kind, r5 = the name. Returns the
// text, or 0 for no name.
extern "C" REX_FUNC(sub_82579250) {
  SBX_FILE_LAYER_FALLBACK(sub_82579250)
  ctx.r3.u64 = guest::AddressOf(
      BuildPath(g, At<Path>(ctx.r3.u32), ctx.r4.u32, At<char>(ctx.r5.u32)));
}

// GamePath::SetGame: r3 = the game's path, r4 = the name: the game path's
// text set to the plain path of the name. Returns the text.
extern "C" REX_FUNC(sub_820A6BA8) {
  SBX_FILE_LAYER_FALLBACK(sub_820A6BA8)
  Path* game_path = At<Path>(ctx.r3.u32);
  const char* name = At<char>(ctx.r4.u32);
  // The original builds a Path on its stack and copies its text over.
  guest_call::InFrame(ctx, base, 624, 0x820A6BDC, [&](uint32_t frame) {
    Path* built = At<Path>(frame + 80);
    built->text[0] = 0;
    built->vtable = kPathVtable;
    if (const char* text = BuildPath(g, built, kPathPlain, name)) {
      std::strcpy(game_path->text, text);
    }
  });
  ctx.r3.u64 = guest::AddressOf(game_path->text);
}
