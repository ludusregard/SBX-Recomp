// The settings file: sbx.toml next to sbx.exe (src/base/settings_file.cpp).
//
// The SDK reads it at every start, before the command line and REX_*
// environment variables (which win over it). It holds one setting per line,
// `name = value`, flat (no [sections]: the SDK would read `[Input]` and
// `mnk_mode` as a flag named `Input_mnk_mode`). The F4 menu's Save button
// writes it through Save here.

#pragma once

#include <rex/cvar.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace settings_file {

// The file the SDK reads (PathConfig::config_path). Set by SbxApp before the
// SDK loads it.
void SetPath(const std::filesystem::path& path);
const std::filesystem::path& Path();

// A flag's value as TOML text: true/false, a number, or a quoted string.
std::string TOMLValue(rex::cvar::FlagType type, std::string_view value);

// One setting to write.
struct Entry {
  std::string name;
  std::string value;     // TOMLValue's text
  bool is_default = false;
  std::string section;   // a comment over new lines, e.g. "Graphics / Window"
};

// Writes `entries` into the file, keeping everything else in it:
// - a setting already in the file has its value replaced in its line (its
//   comment kept);
// - a new setting is added after the file's other settings, under its
//   section's comment, unless it's at its default (nothing to keep);
// - other lines (comments, settings not given, [log.levels] and other
//   tables) are kept as they are.
// The file is written to a temporary file first and then moved over the old
// one. Returns false with `error` set if it couldn't be written.
bool Save(const std::vector<Entry>& entries, std::string& error);

}  // namespace settings_file
