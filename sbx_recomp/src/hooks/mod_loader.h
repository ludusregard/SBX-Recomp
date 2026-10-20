// Archive overrides from MOD_DATA (src/hooks/mod_loader.cpp).

#pragma once

#include <filesystem>

namespace mod_loader {

// The game data root (assets/), where MOD_DATA is looked for. Set by SbxApp
// before the game starts.
void SetGameDataRoot(const std::filesystem::path& root);

}  // namespace mod_loader
