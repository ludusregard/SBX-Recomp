// The custom music folders: CUSTOM_MUSIC/<CATEGORY>/ with audio files and an
// optional metadata.xml (docs/custom_music.md), which can also weigh the
// game's own songs (<Game>; weight 0 blocks one). Scanned once at start-up,
// read-only after. Which category the game is playing music for is decided
// by the caller (src/hooks/custom_music.cpp); this only holds the files and
// picks one.

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "audio/music/metadata.h"

namespace audio::music {

struct Choice {
  std::filesystem::path path;
  std::string name;  // the file's name, UTF-8, for messages
  // The loop region in seconds; 0 and 0: the whole track.
  double loop_in = 0, loop_out = 0;
  // The factor the track plays at: its Volume, times with adaptive volume
  // the gain that brings its loudness up or down to the game's (1: as it is).
  double gain = 1;
};

// Who plays in the place of a blocked game song: one of the folder's tracks,
// or another game song (its bank's name).
struct Substitute {
  std::optional<Choice> custom;
  std::string game;
};

class Library {
 public:
  enum class Level { kInfo, kWarning, kError };
  using Log = std::function<void(Level, const std::string&)>;
  // Whether game song `name` can play in `category` (a <Game> entry's Name).
  using ValidGame = std::function<bool(const std::string& category, const std::string& name)>;

  // Scans `root` (the CUSTOM_MUSIC folder) for the folders named in
  // `categories` (matched ignoring case). Missing folders are fine; other
  // folders are reported. Each file is opened once to check it (Track::Probe).
  // With `adaptive_volume`, each is also decoded once for its loudness
  // (Track::Measure), which takes a moment per file at start-up; the gain is
  // then fixed.
  void Scan(const std::filesystem::path& root, const std::vector<std::string>& categories,
            const Log& log, const ValidGame& valid_game, bool adaptive_volume);

  // The chance a track start in `category` plays a custom track: the
  // folder's ReplaceChance, else `fallback`; 0 when it has nothing to pick.
  double Chance(const std::string& category, double fallback) const;

  // A track of `category` by weight, or nullopt if none can be picked. With
  // `not_path`, any other track if there is one (the same only when it is
  // the only one).
  std::optional<Choice> Pick(const std::string& category, std::mt19937& rng,
                             const std::filesystem::path* not_path = nullptr) const;

  // Tracks that can be picked (weight above 0).
  size_t Count(const std::string& category) const;

  // The weight of game song `name` in `category`: its <Game> entry's, else 1.
  double GameWeight(const std::string& category, const std::string& name) const;

  // For a blocked game song (`blocked`): another song of the folder by
  // weight, from its tracks and the game songs it lists; nullopt if none.
  std::optional<Substitute> PickSubstitute(const std::string& category, const std::string& blocked,
                                           std::mt19937& rng) const;

 private:
  struct Entry {
    Choice choice;
    double weight = 1;
  };
  struct Folder {
    std::string category;
    std::optional<double> replace_chance;
    std::vector<Entry> entries;
    std::vector<GameEntry> game;
  };
  const Folder* Find(const std::string& category) const;

  std::vector<Folder> folders_;
};

// The file extensions read as music (lower case, with the dot).
bool IsMusicFile(const std::filesystem::path& path);

}  // namespace audio::music
