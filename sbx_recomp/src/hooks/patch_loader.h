// The patch loader: archive overrides from PATCH_DATA, and the chosen
// language's files from ADDON_DATA/LANGUAGES (src/hooks/patch_loader.cpp).

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace patch_loader {

// The game data root (assets/), where PATCH_DATA is looked for. Set by SbxApp
// before the game starts.
void SetGameDataRoot(const std::filesystem::path& root);

// A language folder in ADDON_DATA/LANGUAGES/ (its language.xml).
struct Language {
  std::string code;    // the folder's name: EN, JP, ESP...
  std::string name;    // what the menu shows (language.xml's Name)
  std::string script;  // the font folder it uses (language.xml's Font): LATIN...
};

// Any thread, after Archive::Init: the languages found (empty if none), and
// the one in use (empty if languages are off).
std::vector<Language> Languages();
std::string ActiveLanguage();

}  // namespace patch_loader
