// The patch loader: loads archive entries from loose files in PATCH_DATA
// instead of the .DAT archives, so game files can be replaced without
// rebuilding BUILD.DAT.
//
//   assets/PATCH_DATA/BUILD/TXR/BUILD/<id>.<any extension>       BUILD ids
//   assets/PATCH_DATA/BUILD/TXR/BUILD_CRS/<id>.<any extension>   BUILD_CRS ids
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
// Languages (docs/modding-plan.md, "Languages"): the same overlay also serves
// a language's files, chosen by --language, from
//
//   assets/ADDON_DATA/LANGUAGES/<LANG>/<id>.<any extension>       text, screens
//   assets/ADDON_DATA/LANGUAGES/FONTS/<HD|SD>/<SCRIPT>/<id>.<ext>  fonts
//
// Every id found in any language folder or font folder is a "language id".
// For those ids a PATCH_DATA file is ignored, so text left in PATCH_DATA
// can't show up in another language; the chosen language's file is served,
// else the disc's. A font id is looked for in --font_quality's folder for the
// language's script, then the other quality's, then the disc's. A language's
// script comes from its language.xml (<Language Name="English" Font="LATIN"/>);
// without one, JP uses JAPANESE and every other language LATIN. Until
// LANGUAGES/ holds files nothing changes. Phase 1 of the language plan: read
// once at Archive::Init, so a change applies on the next start.
//
// The .TOC and .DAT files on disk aren't changed. Only existing ids can be
// replaced: adding ids needs the entry counts built into the XEX patched.
// New content goes in ADDON_DATA, for the addon loader (docs/modding-plan.md),
// which isn't built yet.
//
// --patch_loader=false turns it all off.

#include "hooks/patch_loader.h"

#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

REXCVAR_DEFINE_BOOL(patch_loader, true, "SBX",
                    "Load archive entries from PATCH_DATA/BUILD/TXR/ (experimental)");
REXCVAR_DEFINE_STRING(language, "", "SBX",
                      "The game's language: a folder in ADDON_DATA/LANGUAGES/ (EN, JP, ...). "
                      "Empty: EN if that folder exists, else JP (the disc's text). Read at "
                      "startup");
REXCVAR_DEFINE_STRING(font_quality, "hd", "SBX",
                      "Which font folder in ADDON_DATA/LANGUAGES/FONTS/ is looked in first: "
                      "hd or sd (the other is the fallback). Read at startup");

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

// A loose file found in a folder: its id, size and game:\ path.
struct Found {
  uint32_t id;
  uint32_t size;
  std::string path;
};

std::filesystem::path g_game_data_root;
// Archive id -> override. Filled once in Archive::Init, read-only after.
std::unordered_map<uint32_t, Override> g_overrides;

// For the F4 menu: the languages found, and which one is in use.
std::mutex g_languages_mutex;
std::vector<patch_loader::Language> g_languages;
std::string g_active_language;

std::string Upper(std::string text) {
  for (char& c : text) {
    c = char(std::toupper(static_cast<unsigned char>(c)));
  }
  return text;
}

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

// The archive id a file name gives, or nullopt. crs: a BUILD_CRS folder,
// where the id within that archive is also taken. any: a flat folder (the
// languages'), where 0-23008 are BUILD ids and 23009-24367 BUILD_CRS ids.
std::optional<uint32_t> ParseId(const std::string& name, bool crs, bool any = false) {
  std::string stem = name.substr(0, name.find('.'));
  uint32_t n = 0;
  auto [end, ec] = std::from_chars(stem.data(), stem.data() + stem.size(), n);
  if (stem.empty() || ec != std::errc() || end != stem.data() + stem.size()) {
    return std::nullopt;
  }
  if (any) {
    return n < kBuildCount + kBuildCrsCount ? std::optional(n) : std::nullopt;
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

// The regular files in `dir`, sorted; empty if it isn't a folder.
std::vector<std::filesystem::path> FilesIn(const std::filesystem::path& dir) {
  std::vector<std::filesystem::path> files;
  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec)) {
    return files;
  }
  std::filesystem::directory_iterator it(dir, ec), end;
  for (; !ec && it != end; it.increment(ec)) {
    if (it->is_regular_file(ec)) {
      files.push_back(it->path());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

// The sub-folders of `dir` (names), sorted.
std::vector<std::string> FoldersIn(const std::filesystem::path& dir) {
  std::vector<std::string> folders;
  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec)) {
    return folders;
  }
  std::filesystem::directory_iterator it(dir, ec), end;
  for (; !ec && it != end; it.increment(ec)) {
    if (it->is_directory(ec)) {
      if (auto name = AsciiName(it->path())) {
        folders.push_back(*name);
      }
    }
  }
  std::sort(folders.begin(), folders.end());
  return folders;
}

// The archive files in `dir` (on disk), given their game:\ folder `guest_dir`
// (backslashes, no trailing one) and how their names are read. `label` names
// the folder in the log.
std::vector<Found> ScanFolder(const std::filesystem::path& dir, const std::string& guest_dir,
                              const std::string& label, bool crs, bool any) {
  std::vector<Found> found;
  for (const auto& file : FilesIn(dir)) {
    auto name = AsciiName(file);
    if (!name) {
      REXLOG_WARN("patch_loader: skipped a file in {}: not an ASCII name", label);
      continue;
    }
    if (Upper(*name) == "LANGUAGE.XML") {
      continue;  // a language's settings, not an archive file
    }
    auto id = ParseId(*name, crs, any);
    if (!id) {
      REXLOG_WARN("patch_loader: skipped {}/{}: not an archive id", label, *name);
      continue;
    }
    std::error_code ec;
    uint64_t size = std::filesystem::file_size(file, ec);
    if (ec || size > UINT32_MAX) {
      REXLOG_WARN("patch_loader: skipped {}/{}: can't read its size", label, *name);
      continue;
    }
    auto same = std::find_if(found.begin(), found.end(),
                             [&](const Found& f) { return f.id == *id; });
    if (same != found.end()) {
      REXLOG_WARN("patch_loader: skipped {}/{}: id {} is already given by {}", label, *name, *id,
                  same->path);
      continue;
    }
    found.push_back({*id, uint32_t(size), guest_dir + "\\" + *name});
  }
  return found;
}

void ScanPatchData(std::vector<Found>& found, bool crs) {
  const std::string folder = crs ? "BUILD_CRS" : "BUILD";
  auto dir = g_game_data_root / "PATCH_DATA" / "BUILD" / "TXR" / folder;
  for (Found& f : ScanFolder(dir, "PATCH_DATA\\BUILD\\TXR\\" + folder,
                             "PATCH_DATA/BUILD/TXR/" + folder, crs, false)) {
    found.push_back(std::move(f));
  }
}

// An attribute's value in a small XML element's text, or "" (enough for
// language.xml's one element; a malformed file just gives no values).
std::string XmlAttribute(const std::string& text, const std::string& name) {
  size_t at = 0;
  while ((at = text.find(name, at)) != std::string::npos) {
    bool starts_word = at == 0 || std::isspace(static_cast<unsigned char>(text[at - 1]));
    size_t eq = text.find_first_not_of(" \t\r\n", at + name.size());
    if (starts_word && eq != std::string::npos && text[eq] == '=') {
      size_t quote = text.find_first_not_of(" \t\r\n", eq + 1);
      if (quote != std::string::npos && (text[quote] == '"' || text[quote] == '\'')) {
        size_t close = text.find(text[quote], quote + 1);
        if (close != std::string::npos) {
          return text.substr(quote + 1, close - quote - 1);
        }
      }
    }
    at += name.size();
  }
  return {};
}

// A language folder's language.xml: its display name and font script.
patch_loader::Language ReadLanguage(const std::filesystem::path& dir, const std::string& code) {
  patch_loader::Language language;
  language.code = code;
  language.name = code;
  language.script = Upper(code) == "JP" ? "JAPANESE" : "LATIN";
  std::filesystem::path xml;
  for (const auto& file : FilesIn(dir)) {
    if (auto name = AsciiName(file); name && Upper(*name) == "LANGUAGE.XML") {
      xml = file;
    }
  }
  if (xml.empty()) {
    return language;
  }
  std::ifstream in(xml, std::ios::binary);
  std::stringstream text;
  text << in.rdbuf();
  std::string body = text.str();
  size_t element = body.find("<Language");
  if (!in || element == std::string::npos) {
    REXLOG_WARN("patch_loader: LANGUAGES/{}/language.xml has no <Language> element; using the "
                "defaults (font {})",
                code, language.script);
    return language;
  }
  size_t close = body.find('>', element);
  std::string tag = body.substr(element, close == std::string::npos ? std::string::npos
                                                                      : close - element);
  if (std::string name = XmlAttribute(tag, "Name"); !name.empty()) {
    language.name = name;
  }
  if (std::string font = XmlAttribute(tag, "Font"); !font.empty()) {
    language.script = Upper(font);
  }
  return language;
}

// The languages and fonts in ADDON_DATA/LANGUAGES/, applied to `found` (the
// PATCH_DATA files, changed in place): language ids lose their PATCH_DATA
// file, and get the chosen language's (and its fonts') instead.
void ApplyLanguages(std::vector<Found>& found) {
  const auto root = g_game_data_root / "ADDON_DATA" / "LANGUAGES";
  std::error_code ec;
  if (!std::filesystem::is_directory(root, ec)) {
    return;
  }

  // Each language folder's files, and each font folder's (quality, script).
  std::map<std::string, std::vector<Found>> language_files;  // by upper-case code
  std::vector<patch_loader::Language> languages;
  std::string fonts_folder;
  for (const std::string& folder : FoldersIn(root)) {
    if (Upper(folder) == "FONTS") {
      fonts_folder = folder;
      continue;
    }
    language_files[Upper(folder)] =
        ScanFolder(root / folder, "ADDON_DATA\\LANGUAGES\\" + folder, "LANGUAGES/" + folder,
                   false, true);
    languages.push_back(ReadLanguage(root / folder, folder));
  }
  // Fonts: (quality, script), both upper case, -> files.
  std::map<std::pair<std::string, std::string>, std::vector<Found>> font_files;
  if (!fonts_folder.empty()) {
    for (const std::string& quality : FoldersIn(root / fonts_folder)) {
      for (const std::string& script : FoldersIn(root / fonts_folder / quality)) {
        const std::string guest =
            "ADDON_DATA\\LANGUAGES\\" + fonts_folder + "\\" + quality + "\\" + script;
        font_files[{Upper(quality), Upper(script)}] =
            ScanFolder(root / fonts_folder / quality / script, guest,
                       "LANGUAGES/FONTS/" + quality + "/" + script, false, true);
      }
    }
  }

  std::set<uint32_t> language_ids, font_ids;
  for (const auto& [code, files] : language_files) {
    for (const Found& f : files) {
      language_ids.insert(f.id);
    }
  }
  for (const auto& [key, files] : font_files) {
    for (const Found& f : files) {
      font_ids.insert(f.id);
    }
  }
  {
    std::lock_guard lock(g_languages_mutex);
    g_languages = languages;
  }
  if (language_ids.empty() && font_ids.empty()) {
    REXLOG_INFO("patch_loader: ADDON_DATA/LANGUAGES/ holds no archive files; languages off");
    return;
  }

  // The language: --language, else EN if it exists, else JP (the disc's).
  auto has = [&](const std::string& code) { return language_files.count(Upper(code)) != 0; };
  std::string wanted = Upper(REXCVAR_GET(language));
  if (!wanted.empty() && !has(wanted) && wanted != "JP") {
    REXLOG_WARN("patch_loader: --language={}: no such folder in ADDON_DATA/LANGUAGES/; "
                "choosing one",
                wanted);
    wanted.clear();
  }
  if (wanted.empty()) {
    wanted = has("EN") ? "EN" : "JP";
  }
  patch_loader::Language chosen;
  chosen.code = wanted;
  chosen.name = wanted;
  chosen.script = wanted == "JP" ? "JAPANESE" : "LATIN";
  for (const auto& language : languages) {
    if (Upper(language.code) == wanted) {
      chosen = language;
    }
  }
  {
    std::lock_guard lock(g_languages_mutex);
    g_active_language = chosen.code;
  }

  // Language ids served from PATCH_DATA: dropped, logged once each.
  std::set<uint32_t> all_ids = language_ids;
  all_ids.insert(font_ids.begin(), font_ids.end());
  found.erase(std::remove_if(found.begin(), found.end(),
                             [&](const Found& f) {
                               if (!all_ids.count(f.id)) {
                                 return false;
                               }
                               REXLOG_INFO("patch_loader: id {}: {} ignored (a language id; "
                                           "the language folders serve it)",
                                           f.id, f.path);
                               return true;
                             }),
              found.end());

  // The chosen language's own files.
  size_t text_count = 0;
  if (auto it = language_files.find(Upper(chosen.code)); it != language_files.end()) {
    for (const Found& f : it->second) {
      found.push_back(f);
      text_count++;
    }
  }

  // Its fonts: the preferred quality's folder for its script, then the other.
  std::string first = Upper(REXCVAR_GET(font_quality)) == "SD" ? "SD" : "HD";
  std::string second = first == "HD" ? "SD" : "HD";
  size_t font_count = 0;
  for (uint32_t id : font_ids) {
    const Found* pick = nullptr;
    for (const std::string& quality : {first, second}) {
      auto it = font_files.find({quality, chosen.script});
      if (it == font_files.end()) {
        continue;
      }
      for (const Found& f : it->second) {
        if (f.id == id) {
          pick = &f;
          break;
        }
      }
      if (pick) {
        break;
      }
    }
    if (pick) {
      found.push_back(*pick);
      font_count++;
    } else {
      REXLOG_INFO("patch_loader: font id {}: none for script {}; the disc's", id, chosen.script);
    }
  }
  REXLOG_INFO("patch_loader: language {} ({}), font {} ({} first): {} text files, {} fonts; "
              "{} language ids in all, the rest from the disc",
              chosen.code, chosen.name, chosen.script, first, text_count, font_count,
              all_ids.size());
}

}  // namespace

namespace patch_loader {

void SetGameDataRoot(const std::filesystem::path& root) { g_game_data_root = root; }

std::vector<Language> Languages() {
  std::lock_guard lock(g_languages_mutex);
  return g_languages;
}

std::string ActiveLanguage() {
  std::lock_guard lock(g_languages_mutex);
  return g_active_language;
}

}  // namespace patch_loader

// Archive::Init: reads both TOCs, then the overrides are applied to them.
extern "C" REX_FUNC(sub_8209E348) {
  __imp__sub_8209E348(ctx, base);
  if (!REXCVAR_GET(patch_loader) || g_game_data_root.empty()) {
    return;
  }

  std::vector<Found> files;
  ScanPatchData(files, false);
  ScanPatchData(files, true);
  ApplyLanguages(files);
  if (files.empty()) {
    return;
  }
  std::vector<Override> found;
  for (Found& f : files) {
    found.push_back({f.id, f.size, std::move(f.path)});
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
    REXLOG_ERROR("patch_loader: couldn't allocate {} bytes for the paths; no overrides", total);
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

    REXLOG_INFO("patch_loader: id {} from {} ({} bytes; was {} stored, {} unpacked)", o.id,
                o.path, o.size, stored, unpacked);
    g_overrides.emplace(o.id, o);
  }
}

// Archive::FindLooseFile: r3 = an archive id; returns its loose file's path.
extern "C" REX_FUNC(sub_8209D9F8) {
  auto it = g_overrides.find(ctx.r3.u32);
  if (it != g_overrides.end()) {
    REXLOG_INFO("patch_loader: loading id {} from {}", it->first, it->second.path);
    ctx.r3.u64 = it->second.guest_path;
    return;
  }
  __imp__sub_8209D9F8(ctx, base);
}
