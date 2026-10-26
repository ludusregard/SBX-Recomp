#include "audio/music/library.h"

#include "audio/music/metadata.h"
#include "audio/music/track.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <system_error>

namespace audio::music {

namespace {

constexpr const char* kMetadataName = "metadata.xml";

// A path's name as UTF-8 (std::filesystem gives wide strings on Windows).
std::string Utf8Name(const std::filesystem::path& path) {
  const std::u8string name = path.filename().u8string();
  return std::string(name.begin(), name.end());
}

std::string Lower(std::string s) {
  for (char& c : s) {
    c = char(std::tolower(uint8_t(c)));
  }
  return s;
}

std::string Seconds(double s) {
  std::ostringstream out;
  out.precision(2);
  out << std::fixed << s;
  return out.str();
}

std::optional<std::string> ReadText(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

}  // namespace

bool IsMusicFile(const std::filesystem::path& path) {
  const std::string ext = Lower(Utf8Name(path.extension()));
  return ext == ".ogg" || ext == ".oga" || ext == ".opus" || ext == ".flac" || ext == ".mp3" ||
         ext == ".wav";
}

void Library::Scan(const std::filesystem::path& root, const std::vector<std::string>& categories,
                   const Log& log, const ValidGame& valid_game) {
  folders_.clear();
  std::error_code ec;
  if (!std::filesystem::is_directory(root, ec)) {
    log(Level::kInfo, "no CUSTOM_MUSIC folder; the game's music plays");
    return;
  }

  std::vector<std::filesystem::path> dirs;
  for (std::filesystem::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
    // Hidden folders (".git" and the like) aren't categories.
    if (it->is_directory(ec) && !Utf8Name(it->path()).starts_with('.')) {
      dirs.push_back(it->path());
    }
  }
  std::sort(dirs.begin(), dirs.end());

  for (const auto& dir : dirs) {
    const std::string dir_name = Utf8Name(dir);
    auto category = std::find_if(categories.begin(), categories.end(),
                                 [&](const std::string& c) { return SameName(c, dir_name); });
    if (category == categories.end()) {
      std::string known;
      for (const auto& c : categories) {
        known += (known.empty() ? "" : ", ") + c;
      }
      log(Level::kWarning, "CUSTOM_MUSIC/" + dir_name + " isn't a category (" + known +
                               "); ignored");
      continue;
    }
    if (Find(*category)) {
      log(Level::kWarning, "CUSTOM_MUSIC/" + dir_name + ": " + *category +
                               " has another folder already; ignored");
      continue;
    }
    const std::string where = "CUSTOM_MUSIC/" + dir_name;

    // The audio files, by name.
    std::vector<std::filesystem::path> files;
    for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end;
         it.increment(ec)) {
      if (!it->is_regular_file(ec)) {
        continue;
      }
      const std::string name = Utf8Name(it->path());
      if (name.starts_with('.')) {
        continue;  // hidden files (the folder's .gitignore) go unmentioned
      }
      if (IsMusicFile(it->path())) {
        files.push_back(it->path());
      } else if (!SameName(name, kMetadataName)) {
        log(Level::kInfo, where + "/" + name + " isn't a music file (ogg, opus, flac, mp3, " +
                              "wav); skipped");
      }
    }
    std::sort(files.begin(), files.end());

    // metadata.xml, if any.
    Metadata metadata;
    const auto xml = dir / kMetadataName;
    if (std::filesystem::is_regular_file(xml, ec)) {
      auto text = ReadText(xml);
      std::string error;
      std::vector<std::string> warnings;
      std::optional<Metadata> parsed =
          text ? ParseMetadata(*text, &error, &warnings) : std::nullopt;
      if (!text) {
        error = "can't be read";
      }
      for (const auto& w : warnings) {
        log(Level::kWarning, where + "/" + kMetadataName + ", " + w);
      }
      if (!parsed) {
        log(Level::kError, where + "/" + kMetadataName + ", " + error +
                               "; this folder is turned off");
        continue;
      }
      if (parsed->category && !SameName(*parsed->category, *category)) {
        log(Level::kError, where + "/" + kMetadataName + ": Category \"" + *parsed->category +
                               "\" isn't this folder's (" + *category +
                               "); this folder is turned off");
        continue;
      }
      metadata = std::move(*parsed);
    }

    Folder folder;
    folder.category = *category;
    folder.replace_chance = metadata.replace_chance;

    // The game's songs it weighs.
    for (const GameEntry& g : metadata.game) {
      if (!valid_game(*category, g.name)) {
        log(Level::kWarning, where + "/" + kMetadataName + ", line " + std::to_string(g.line) +
                                 ": the game's " + g.name + " can't play in " + *category +
                                 "; ignored");
        continue;
      }
      const bool twice = std::any_of(folder.game.begin(), folder.game.end(),
                                     [&](const GameEntry& e) { return SameName(e.name, g.name); });
      if (twice) {
        log(Level::kWarning, where + "/" + kMetadataName + ", line " + std::to_string(g.line) +
                                 ": " + g.name + " is listed twice; the first counts");
        continue;
      }
      if (g.weight != 1) {
        log(Level::kInfo, where + ": the game's " + g.name + ", weight " + Seconds(g.weight) +
                              (g.weight == 0 ? " (blocked)" : ""));
      }
      folder.game.push_back(g);
    }

    // Tracks named in the XML that aren't in the folder.
    for (const TrackEntry& t : metadata.tracks) {
      const bool found = std::any_of(files.begin(), files.end(), [&](const auto& f) {
        return SameName(Utf8Name(f), t.file);
      });
      if (!found) {
        log(Level::kWarning, where + "/" + kMetadataName + ", line " + std::to_string(t.line) +
                                 ": " + t.file + " isn't a music file in this folder; skipped");
      }
    }

    for (const auto& file : files) {
      const std::string name = Utf8Name(file);
      Entry entry;
      entry.choice.path = file;
      entry.choice.name = name;
      // Its XML entry, if any (the first, if named twice).
      auto t = std::find_if(metadata.tracks.begin(), metadata.tracks.end(),
                            [&](const TrackEntry& e) { return SameName(e.file, name); });
      std::string error;
      auto info = Track::Probe(file, &error);
      if (!info) {
        log(Level::kWarning, where + "/" + name + " " + error + "; skipped");
        continue;
      }
      if (t != metadata.tracks.end()) {
        entry.weight = t->weight;
        if (t->loop_in) {
          if (info->seconds > 0 && *t->loop_out > info->seconds + 0.01) {
            log(Level::kWarning, where + "/" + kMetadataName + ", line " +
                                     std::to_string(t->line) + ": " + name + "'s LoopOut (" +
                                     Seconds(*t->loop_out) + " s) is past its end (" +
                                     Seconds(info->seconds) + " s); the whole track loops");
          } else {
            entry.choice.loop_in = *t->loop_in;
            entry.choice.loop_out = *t->loop_out;
          }
        }
      }
      std::string loop = entry.choice.loop_out > 0
                             ? ", loops " + Seconds(entry.choice.loop_in) + "-" +
                                   Seconds(entry.choice.loop_out) + " s"
                             : "";
      log(Level::kInfo, where + "/" + name + ": " + info->codec + ", " +
                            std::to_string(info->sample_rate) + " Hz, " +
                            std::to_string(info->channels) + " ch, " + Seconds(info->seconds) +
                            " s, weight " + Seconds(entry.weight) + loop);
      folder.entries.push_back(std::move(entry));
    }

    const size_t count = std::count_if(folder.entries.begin(), folder.entries.end(),
                                       [](const Entry& e) { return e.weight > 0; });
    std::string chance =
        folder.replace_chance ? "ReplaceChance " + Seconds(*folder.replace_chance)
                              : "the new_music_roll chance";
    log(Level::kInfo, *category + ": " + std::to_string(count) + " track(s) to pick from, " +
                          chance);
    folders_.push_back(std::move(folder));
  }
}

const Library::Folder* Library::Find(const std::string& category) const {
  for (const auto& f : folders_) {
    if (SameName(f.category, category)) {
      return &f;
    }
  }
  return nullptr;
}

double Library::Chance(const std::string& category, double fallback) const {
  const Folder* f = Find(category);
  if (!f || Count(category) == 0) {
    return 0;
  }
  return std::clamp(f->replace_chance.value_or(fallback), 0.0, 1.0);
}

double Library::GameWeight(const std::string& category, const std::string& name) const {
  const Folder* f = Find(category);
  if (!f) {
    return 1;
  }
  for (const auto& g : f->game) {
    if (SameName(g.name, name)) {
      return g.weight;
    }
  }
  return 1;
}

std::optional<Substitute> Library::PickSubstitute(const std::string& category,
                                                  const std::string& blocked,
                                                  std::mt19937& rng) const {
  const Folder* f = Find(category);
  if (!f) {
    return std::nullopt;
  }
  // The folder's tracks, then the game songs it lists (but the blocked one).
  std::vector<double> weights;
  double total = 0;
  for (const auto& e : f->entries) {
    weights.push_back(e.weight);
    total += e.weight;
  }
  for (const auto& g : f->game) {
    const double w = SameName(g.name, blocked) ? 0 : g.weight;
    weights.push_back(w);
    total += w;
  }
  if (total <= 0) {
    return std::nullopt;
  }
  std::discrete_distribution<size_t> pick(weights.begin(), weights.end());
  const size_t i = pick(rng);
  Substitute s;
  if (i < f->entries.size()) {
    s.custom = f->entries[i].choice;
  } else {
    s.game = f->game[i - f->entries.size()].name;
  }
  return s;
}

size_t Library::Count(const std::string& category) const {
  const Folder* f = Find(category);
  if (!f) {
    return 0;
  }
  return std::count_if(f->entries.begin(), f->entries.end(),
                       [](const Entry& e) { return e.weight > 0; });
}

std::optional<Choice> Library::Pick(const std::string& category, std::mt19937& rng,
                                    const std::filesystem::path* not_path) const {
  const Folder* f = Find(category);
  if (!f || Count(category) == 0) {
    return std::nullopt;
  }
  std::vector<double> weights;
  double others = 0;
  for (const auto& e : f->entries) {
    weights.push_back(e.weight);
    if (!not_path || e.choice.path != *not_path) {
      others += e.weight;
    }
  }
  if (not_path && others > 0) {
    for (size_t i = 0; i < weights.size(); i++) {
      if (f->entries[i].choice.path == *not_path) {
        weights[i] = 0;
      }
    }
  }
  std::discrete_distribution<size_t> pick(weights.begin(), weights.end());
  return f->entries[pick(rng)].choice;
}

}  // namespace audio::music
