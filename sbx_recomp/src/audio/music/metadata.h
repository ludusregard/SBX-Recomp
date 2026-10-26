// A custom music folder's metadata.xml (docs/custom_music.md):
//
//   <Music Category="ROAMING" ReplaceChance="0.5">
//     <Track File="drive_01.ogg" Weight="2" LoopIn="12.0" LoopOut="184.5"/>
//     <Track File="drive_02.flac"/>
//   </Music>
//
// A small XML reader for this schema only: elements, attributes, comments,
// the <?xml?> declaration and the five standard entities (and &#...;).
// Element and attribute names are matched ignoring case.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace audio::music {

struct TrackEntry {
  std::string file;  // UTF-8, as written
  double weight = 1;
  // The loop region in seconds; unset: the whole track loops.
  std::optional<double> loop_in, loop_out;
  int line = 0;  // where it is in the file, for messages
};

struct Metadata {
  std::optional<std::string> category;
  std::optional<double> replace_chance;
  std::vector<TrackEntry> tracks;
};

// Parses `text`. A malformed file or a bad value gives nullopt and `error`
// (with its line); things that are only ignored are added to `warnings`.
std::optional<Metadata> ParseMetadata(std::string_view text, std::string* error,
                                      std::vector<std::string>* warnings);

// ASCII case-insensitive equality (names, folder names, file names).
bool SameName(std::string_view a, std::string_view b);

}  // namespace audio::music
