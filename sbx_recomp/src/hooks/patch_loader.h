// The patch loader: archive overrides from PATCH_DATA
// (src/hooks/patch_loader.cpp).

#pragma once

#include <filesystem>

namespace patch_loader {

// The game data root (assets/), where PATCH_DATA is looked for. Set by SbxApp
// before the game starts.
void SetGameDataRoot(const std::filesystem::path& root);

}  // namespace patch_loader
