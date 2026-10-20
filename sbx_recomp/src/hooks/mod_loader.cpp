// Loads archive entries from loose files in MOD_DATA instead of the .DAT
// archives, so game files can be replaced without rebuilding BUILD.DAT.
//
//   assets/MOD_DATA/BUILD/TXR/BUILD/<id>.<any extension>       BUILD ids
//   assets/MOD_DATA/BUILD/TXR/BUILD_CRS/<id>.<any extension>   BUILD_CRS ids
//
// A file is named by its archive id, as GUT Archive Tools names its dumps
// (00000072.bin is the car specs); the extension doesn't matter. In
// BUILD_CRS/, the id is either the game's (23009-24367) or the one within
// that archive (0-1358). Files are uncompressed, like the dumps.
//
// It uses the game's own loose-file support (docs/game_data.md): an entry
// whose data is a file on disc has a start sector of 0, and
// Archive::FindLooseFile (sub_8209D9F8) gives the file's path. After
// Archive::Init (sub_8209E348) has read the TOCs, each override's TOC entry is
// changed in memory to start sector 0, stored size = the file's size, and
// decompressed size 0 (stored uncompressed); FindLooseFile then returns the
// override's path for that id. Archive::Load (sub_8209DF80) reads the sizes
// from the TOC on every load, and so do the callers that size their buffers
// from it (sub_820C9700 for the parameter tables), so a file may be larger or
// smaller than the original. Its format is still up to the modder.
//
// The .TOC and .DAT files on disk aren't changed. Only existing ids can be
// replaced: adding ids needs the entry counts built into the XEX patched.
//
// --mod_loader=false turns it off.

#include "hooks/mod_loader.h"

#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

REXCVAR_DEFINE_BOOL(mod_loader, true, "SBX",
                    "Load archive entries from MOD_DATA/BUILD/TXR/ (experimental)");

namespace {

constexpr uint32_t kBuildCount = 23009;    // BUILD ids are 0-23008.
constexpr uint32_t kBuildCrsCount = 1359;  // BUILD_CRS ids are 23009-24367.
constexpr uint32_t kBuildTocPointer = 0x82694E00;
constexpr uint32_t kBuildCrsTocPointer = 0x82694E08;

struct Override {
  uint32_t id;
  uint32_t size;
  std::string path;  // Relative to game:\, as the loose-file table's.
  uint32_t guest_path = 0;
};

std::filesystem::path g_game_data_root;
// Archive id -> override. Filled once in Archive::Init, read-only after.
std::unordered_map<uint32_t, Override> g_overrides;

// The file's name if it's printable ASCII (the guest path is a C string).
std::optional<std::string> AsciiName(const std::filesystem::path& file) {
  std::string name;
  for (auto c : file.filename().native()) {
    if (c < 0x20 || c > 0x7E) {
      return std::nullopt;
    }
    name.push_back(char(c));
  }
  return name;
}

// The archive id a file name gives, or nullopt.
std::optional<uint32_t> ParseId(const std::string& name, bool crs) {
  std::string stem = name.substr(0, name.find('.'));
  uint32_t n = 0;
  auto [end, ec] = std::from_chars(stem.data(), stem.data() + stem.size(), n);
  if (stem.empty() || ec != std::errc() || end != stem.data() + stem.size()) {
    return std::nullopt;
  }
  if (!crs) {
    return n < kBuildCount ? std::optional(n) : std::nullopt;
  }
  if (n < kBuildCrsCount) {
    return kBuildCount + n;
  }
  if (n >= kBuildCount && n < kBuildCount + kBuildCrsCount) {
    return n;
  }
  return std::nullopt;
}

void Scan(std::vector<Override>& found, bool crs) {
  const std::string folder = crs ? "BUILD_CRS" : "BUILD";
  auto dir = g_game_data_root / "MOD_DATA" / "BUILD" / "TXR" / folder;
  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec)) {
    return;
  }
  std::vector<std::filesystem::path> files;
  std::filesystem::directory_iterator it(dir, ec), end;
  for (; !ec && it != end; it.increment(ec)) {
    if (it->is_regular_file(ec)) {
      files.push_back(it->path());
    }
  }
  std::sort(files.begin(), files.end());

  for (const auto& file : files) {
    auto name = AsciiName(file);
    if (!name) {
      REXLOG_WARN("mod_loader: skipped a file in MOD_DATA/BUILD/TXR/{}: not an ASCII name",
                  folder);
      continue;
    }
    auto id = ParseId(*name, crs);
    if (!id) {
      REXLOG_WARN("mod_loader: skipped {}/{}: not an archive id", folder, *name);
      continue;
    }
    uint64_t size = std::filesystem::file_size(file, ec);
    if (ec || size > UINT32_MAX) {
      REXLOG_WARN("mod_loader: skipped {}/{}: can't read its size", folder, *name);
      continue;
    }
    auto same = std::find_if(found.begin(), found.end(),
                             [&](const Override& o) { return o.id == *id; });
    if (same != found.end()) {
      REXLOG_WARN("mod_loader: skipped {}/{}: id {} is already replaced by {}", folder,
                  *name, *id, same->path);
      continue;
    }
    found.push_back({*id, uint32_t(size), "MOD_DATA\\BUILD\\TXR\\" + folder + "\\" + *name});
  }
}

}  // namespace

namespace mod_loader {

void SetGameDataRoot(const std::filesystem::path& root) { g_game_data_root = root; }

}  // namespace mod_loader

// Archive::Init: reads both TOCs, then the overrides are applied to them.
extern "C" REX_FUNC(sub_8209E348) {
  __imp__sub_8209E348(ctx, base);
  if (!REXCVAR_GET(mod_loader) || g_game_data_root.empty()) {
    return;
  }

  std::vector<Override> found;
  Scan(found, false);
  Scan(found, true);
  if (found.empty()) {
    return;
  }

  // Guest copies of the paths for FindLooseFile to return, from the game's
  // operator new (sub_8258C000). Never freed: queued loads keep the pointer.
  uint32_t total = 0;
  for (const auto& o : found) {
    total += uint32_t(o.path.size()) + 1;
  }
  uint64_t result = ctx.r3.u64;
  ctx.r3.u64 = total;
  sub_8258C000(ctx, base);
  uint32_t strings = ctx.r3.u32;
  ctx.r3.u64 = result;
  if (strings == 0) {
    REXLOG_ERROR("mod_loader: couldn't allocate {} bytes for the paths; no overrides", total);
    return;
  }

  for (auto& o : found) {
    o.guest_path = strings;
    for (char c : o.path) {
      guest::StoreU8(base, strings++, uint8_t(c));
    }
    guest::StoreU8(base, strings++, 0);

    bool crs = o.id >= kBuildCount;
    uint32_t toc = guest::LoadU32(base, crs ? kBuildCrsTocPointer : kBuildTocPointer);
    uint32_t entry = toc + 16 * ((crs ? o.id - kBuildCount : o.id) + 1);
    uint32_t stored = guest::LoadU32(base, entry + 4);
    uint32_t unpacked = guest::LoadU32(base, entry + 8);
    guest::StoreU32(base, entry + 0, 0);       // Start sector: a loose file.
    guest::StoreU32(base, entry + 4, o.size);  // Stored size.
    guest::StoreU32(base, entry + 8, 0);       // Decompressed size: stored as is.

    REXLOG_INFO("mod_loader: id {} from {} ({} bytes; was {} stored, {} unpacked)", o.id,
                o.path, o.size, stored, unpacked);
    g_overrides.emplace(o.id, o);
  }
}

// Archive::FindLooseFile: r3 = an archive id; returns its loose file's path.
extern "C" REX_FUNC(sub_8209D9F8) {
  auto it = g_overrides.find(ctx.r3.u32);
  if (it != g_overrides.end()) {
    REXLOG_INFO("mod_loader: loading id {} from {}", it->first, it->second.path);
    ctx.r3.u64 = it->second.guest_path;
    return;
  }
  __imp__sub_8209D9F8(ctx, base);
}
