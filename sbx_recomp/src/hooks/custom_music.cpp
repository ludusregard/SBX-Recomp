// Custom music (docs/custom_music.md): when the game picks its music, a roll
// decides whether a track from ADDON_DATA/CUSTOM_MUSIC/<CATEGORY>/ plays in
// its place. The game's own choice, its start, stop and fades are untouched:
// the swap happens underneath, where the music's XMA sound is decoded
// (src/hooks/xma.cpp), so the game's voice plays our samples instead.
//
// Every music change goes through Bgm::Load (sub_82106540, r4 the bank's
// name); its caller (the return address) says what the music is for. The
// call sites (docs/audio.md, "Music"):
//
//   (menu)    0x82107B38  sub_82107A38: the front end's BGM_SYSTEM, which
//                         holds four songs (below) and is loaded once
//   ROAMING   0x82173D80  Music::PlayRoaming (sub_82173C70): bgm_tmp_06, or
//                         one of bgm_tmp_07, 01, 15, 05, 10 at random
//   BATTLE    0x8217424C  Music::PlayRequested (sub_821741B8), a battle with
//                         no special music: bgm_tmp_07, 01 or 15
//             0x8217427C  rival class 1: bgm_tmp_10
//             0x821742AC  rival class 2: bgm_tmp_05
//   BOSS      0x821742DC  rival class 3 (other ids): bgm_tmp_03
//             0x8217430C  rival class 3 (383-390) or 4: bgm_tmp_09
//             0x8217433C  rival class 3 (392-397) or 4 (391): bgm_tmp_17
//             0x8217436C  rival 398: bgm_tmp_11
//             0x8217439C  rival class 5: bgm_tmp_16
//   RACE_LOSE 0x82174C84  sub_82174AD0: bgm_tmp_08
//   RACE_WIN  0x82262678  sub_822624F0: bgm_tmp_14
//   RACE_LOSE 0x822626B0  the same: bgm_tmp_08
//
// BGM_SYSTEM has four songs, one wave each (the bank's own order; the game
// starts them by cue without loading the bank again, so a roll at the load
// would catch only the first): 0 the course select, 1 the main menu, 2 the
// other menus (dialogs, garage, car select), 3 the dialog after some story
// events. Each time one of its sounds starts it rolls for itself; the wave is
// told by its first 2048 bytes (kMenuSongs, from the disc's BGM_SYSTEM.xwb).
//
// Never replaced: the music select (0x821B8788, sub_821B8580: the player's
// own pick) and SB_SYSTEM_SE at boot (0x82110280, sub_82110180: sound
// effects). The category names are from reading the code; every Bgm::Load
// is logged with its caller so they can be checked in play.
//
// The roll is once per Bgm::Load, with our own random numbers (the game's
// stay as they were, for replays). A hit picks a track by weight and opens it
// here, on the game thread; the next stereo XMA sound to start (the music is
// stereo XMA, the menu sounds mono) takes it. Until the next Bgm::Load, that
// sound starting again (the same XMA input) reopens the same track; any other
// stereo sound plays the game's.
//
// When a custom track ends and the game's sound would loop, another track of
// the folder plays next (--custom_music_shuffle, the default) or, with
// --custom_music_shuffle=false, the track loops. The next track is picked by
// weight, never the same one twice in a row unless it is the only one, from
// the folder only, with no roll against the game's music.
//
// The generator behind the picks and rolls is seeded from the system time as
// well as the system's random source.
//
// --custom_music=false turns it off (read at start-up for the scan, and at
// each Bgm::Load); --new_music_roll is the chance when a folder's
// metadata.xml gives no ReplaceChance; --custom_music_log logs the details.

#include "hooks/custom_music.h"

#include "audio/music/library.h"
#include "audio/music/track.h"
#include "guest.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <vector>

REXCVAR_DEFINE_BOOL(custom_music, true, "SBX",
                    "Play tracks from ADDON_DATA/CUSTOM_MUSIC in the place of the game's music "
                    "(experimental)");
REXCVAR_DEFINE_DOUBLE(new_music_roll, 0.0, "SBX",
                      "Custom music: the chance (0 to 1) each music start plays a custom track, "
                      "for folders whose metadata.xml has no ReplaceChance");
REXCVAR_DEFINE_BOOL(custom_music_shuffle, true, "SBX",
                    "Custom music: when a custom track ends and would loop, play another track "
                    "of the folder instead");
REXCVAR_DEFINE_BOOL(custom_music_log, false, "SBX",
                    "Custom music: log each roll and each time a track starts");

namespace {

// The "category" of the Bgm::Load call that loads BGM_SYSTEM: not a category
// itself (see MenuSong).
constexpr const char* kMenuBank = "MENU";

struct CallSite {
  uint32_t return_address;
  const char* category;
};

constexpr CallSite kCallSites[] = {
    {0x82107B38, kMenuBank},   {0x82173D80, "ROAMING"},   {0x8217424C, "BATTLE"},
    {0x8217427C, "BATTLE"},    {0x821742AC, "BATTLE"},    {0x821742DC, "BOSS"},
    {0x8217430C, "BOSS"},      {0x8217433C, "BOSS"},      {0x8217436C, "BOSS"},
    {0x8217439C, "BOSS"},      {0x82174C84, "RACE_LOSE"}, {0x82262678, "RACE_WIN"},
    {0x822626B0, "RACE_LOSE"},
};

// BGM_SYSTEM's four songs: the first 2048 bytes' CRC-32, the wave's length
// (a second check) and the category. The sizes and data are from the
// disc's BGM_SYSTEM.xwb.
struct MenuSong {
  uint32_t crc;
  uint32_t size;
  const char* category;
};
constexpr MenuSong kMenuSongs[] = {
    {0xE3A47B25, 0x1BD000, "MENU_COURSE"},   // wave 0
    {0xE66C2C17, 0x0BA800, "MENU_MAIN"},     // wave 1
    {0xC448EE59, 0x18F800, "MENU_GENERAL"},  // wave 2
    {0x4FEB9978, 0x12B000, "MENU_STORY"},    // wave 3
};

const std::vector<std::string> kCategories = {"MENU_COURSE", "MENU_MAIN", "MENU_GENERAL",
                                              "MENU_STORY",  "ROAMING",   "BATTLE",
                                              "BOSS",        "RACE_WIN",  "RACE_LOSE"};

uint32_t Crc32(const uint8_t* data, size_t size) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc >> 1) ^ (0xEDB88320 & (0 - (crc & 1)));
    }
  }
  return ~crc;
}

// Which of the four songs a BGM_SYSTEM sound is (an index into kMenuSongs), or
// -1. By its first bytes, else by its length.
int MenuSong(const uint8_t* data, uint32_t size, uint32_t* crc_out) {
  const uint32_t crc = data ? Crc32(data, std::min<uint32_t>(size, 2048)) : 0;
  *crc_out = crc;
  for (int i = 0; i < 4; i++) {
    if (kMenuSongs[i].crc == crc) {
      return i;
    }
  }
  for (int i = 0; i < 4; i++) {
    if (kMenuSongs[i].size == size) {
      return i;
    }
  }
  return -1;
}

const char* CategoryOf(uint32_t return_address) {
  for (const auto& site : kCallSites) {
    if (site.return_address == return_address) {
      return site.category;
    }
  }
  return nullptr;
}

// "0.25": two decimals, for messages.
std::string Fixed2(double v) {
  char text[32];
  std::snprintf(text, sizeof(text), "%.2f", v);
  return text;
}

std::mutex g_mutex;
audio::music::Library g_library;
// Seeded from the system's random source, the system time (both clocks) and
// an address, so no two runs start the same even where the random source is
// weak.
std::mt19937 MakeRandom() {
  const auto wall = std::chrono::system_clock::now().time_since_epoch().count();
  const auto steady = std::chrono::steady_clock::now().time_since_epoch().count();
  int local = 0;
  std::random_device device;
  std::seed_seq seed{device(),
                     device(),
                     uint32_t(wall),
                     uint32_t(uint64_t(wall) >> 32),
                     uint32_t(steady),
                     uint32_t(uint64_t(steady) >> 32),
                     uint32_t(reinterpret_cast<uintptr_t>(&local))};
  return std::mt19937(seed);
}
std::mt19937 g_random = MakeRandom();
// The category of the last Bgm::Load that picked a track, for the shuffle.
std::string g_category;
// The track chosen at the last Bgm::Load (none: the game's music), and the
// one opened for it, not yet taken by a starting sound.
std::optional<audio::music::Choice> g_current;
std::unique_ptr<audio::music::Track> g_pending;
uint32_t g_input = 0;  // the XMA input of the sound that took it, 0 none yet
bool g_menu_bank = false;  // BGM_SYSTEM is the loaded bank: each sound rolls as it starts

std::string GuestString(uint8_t* base, uint32_t address, uint32_t max) {
  std::string s;
  for (uint32_t i = 0; address && i < max; i++) {
    const char c = char(guest::LoadU8(base, address + i));
    if (!c) {
      break;
    }
    s += c;
  }
  return s;
}

// Opens a chosen track (the slow part: the file's headers, the decoder).
std::unique_ptr<audio::music::Track> Open(const audio::music::Choice& choice) {
  std::string error;
  auto track = audio::music::Track::Open(choice.path, &error);
  if (!track) {
    REXLOG_ERROR("custom_music: {} {}; the game's music plays", choice.name, error);
    return nullptr;
  }
  track->SetRegion(choice.loop_in, choice.loop_out);
  return track;
}

// At each Bgm::Load: the roll, and the track opened if it hits. The open is
// under the lock on purpose: a music sound starting meanwhile (TakeTrack, on
// the audio thread) waits the few milliseconds for it rather than playing the
// game's track after a hit.
void Choose(uint32_t from, const std::string& bank) {
  std::lock_guard lock(g_mutex);
  g_current.reset();
  g_pending.reset();
  g_input = 0;
  g_category.clear();
  g_menu_bank = false;
  const char* category = CategoryOf(from);
  if (!category) {
    REXLOG_INFO("custom_music: {} (from {:08X}): not a category; the game's", bank, from);
    return;
  }
  if (!REXCVAR_GET(custom_music)) {
    REXLOG_INFO("custom_music: {} (from {:08X}, {}): off; the game's", bank, from, category);
    return;
  }
  if (std::strcmp(category, kMenuBank) == 0) {
    g_menu_bank = true;
    REXLOG_INFO("custom_music: {} (from {:08X}): the menu bank; each of its songs rolls as it "
                "starts",
                bank, from);
    return;
  }
  const double chance = g_library.Chance(category, REXCVAR_GET(new_music_roll));
  const double roll = std::uniform_real_distribution<double>(0.0, 1.0)(g_random);
  if (chance <= 0 || roll >= chance) {
    REXLOG_INFO("custom_music: {} (from {:08X}, {}): the game's{}", bank, from, category,
                chance > 0 ? " (rolled " + Fixed2(roll) + ", chance " + Fixed2(chance) + ")"
                           : std::string(" (nothing to pick, or no chance set)"));
    return;
  }
  auto choice = g_library.Pick(category, g_random);
  if (!choice) {
    return;
  }
  g_pending = Open(*choice);
  if (!g_pending) {
    return;
  }
  REXLOG_INFO("custom_music: {} (from {:08X}, {}): {} instead (rolled {}, chance {})", bank,
              from, category, choice->name, Fixed2(roll), Fixed2(chance));
  g_current = std::move(choice);
  g_category = category;
}

// A BGM_SYSTEM song starting (the lock is held): the roll and pick for its
// category, and the track opened on a hit. The game starts these without
// loading the bank, so each start rolls on its own.
std::unique_ptr<audio::music::Track> RollMenuSong(uint32_t input, const uint8_t* data,
                                                  uint32_t size) {
  g_current.reset();
  g_pending.reset();
  g_category.clear();
  g_input = 0;
  uint32_t crc = 0;
  const int song = MenuSong(data, size, &crc);
  if (song < 0) {
    REXLOG_INFO("custom_music: a stereo sound in the menu bank ({:08X}, {} bytes, CRC {:08X}) "
                "isn't one of its four songs; the game's",
                input, size, crc);
    return nullptr;
  }
  const char* category = kMenuSongs[song].category;
  const double chance = g_library.Chance(category, REXCVAR_GET(new_music_roll));
  const double roll = std::uniform_real_distribution<double>(0.0, 1.0)(g_random);
  if (chance <= 0 || roll >= chance) {
    REXLOG_INFO("custom_music: menu song {} ({}): the game's{}", song, category,
                chance > 0 ? " (rolled " + Fixed2(roll) + ", chance " + Fixed2(chance) + ")"
                           : std::string(" (nothing to pick, or no chance set)"));
    return nullptr;
  }
  auto choice = g_library.Pick(category, g_random);
  if (!choice) {
    return nullptr;
  }
  auto track = Open(*choice);
  if (!track) {
    return nullptr;
  }
  REXLOG_INFO("custom_music: menu song {} ({}): {} instead (rolled {}, chance {})", song,
              category, choice->name, Fixed2(roll), Fixed2(chance));
  g_current = std::move(choice);
  g_category = category;
  g_input = input;
  return track;
}

}  // namespace

namespace custom_music {

void SetGameDataRoot(const std::filesystem::path& root) {
  if (!REXCVAR_GET(custom_music)) {
    REXLOG_INFO("custom_music: off (--custom_music=false)");
    return;
  }
  std::lock_guard lock(g_mutex);
  g_library.Scan(root / "ADDON_DATA" / "CUSTOM_MUSIC", kCategories,
                 [](audio::music::Library::Level level, const std::string& message) {
                   switch (level) {
                     case audio::music::Library::Level::kInfo:
                       REXLOG_INFO("custom_music: {}", message);
                       break;
                     case audio::music::Library::Level::kWarning:
                       REXLOG_WARN("custom_music: {}", message);
                       break;
                     case audio::music::Library::Level::kError:
                       REXLOG_ERROR("custom_music: {}", message);
                       break;
                   }
                 });
}

std::unique_ptr<audio::music::Track> TakeTrack(uint32_t input, const uint8_t* data, uint32_t size,
                                               uint32_t sample_rate, uint32_t channels,
                                               uint32_t ahead) {
  std::unique_ptr<audio::music::Track> track;
  std::string name;
  {
    std::lock_guard lock(g_mutex);
    if (g_menu_bank) {
      track = RollMenuSong(input, data, size);
      if (!track) {
        return nullptr;
      }
      name = g_current->name;
    } else if (!g_current) {
      return nullptr;
    } else {
      name = g_current->name;
      if (g_pending) {
        track = std::move(g_pending);
        g_input = input;
      } else if (input == g_input) {
        // The same music started again: the same track, from the start.
        track = Open(*g_current);
        if (track && Logging()) {
          REXLOG_INFO("custom_music: {} restarted", name);
        }
      } else {
        if (Logging()) {
          REXLOG_INFO("custom_music: another stereo sound ({:08X}) while {} plays; the game's",
                      input, name);
        }
        return nullptr;
      }
    }
  }
  if (!track) {
    return nullptr;
  }
  if (!track->Begin(sample_rate, channels, ahead)) {
    REXLOG_ERROR("custom_music: {} can't be decoded to {} Hz, {} channel(s); the game's music "
                 "plays",
                 name, sample_rate, channels);
    return nullptr;
  }
  if (Logging()) {
    REXLOG_INFO("custom_music: {} starts ({} Hz, {} channel(s))", name, sample_rate, channels);
  }
  return track;
}

std::unique_ptr<audio::music::Track> NextTrack(uint32_t sample_rate, uint32_t channels,
                                               uint32_t ahead) {
  if (!REXCVAR_GET(custom_music_shuffle)) {
    return nullptr;
  }
  std::unique_ptr<audio::music::Track> track;
  std::string name;
  {
    std::lock_guard lock(g_mutex);
    if (!g_current || g_category.empty()) {
      return nullptr;
    }
    auto choice = g_library.Pick(g_category, g_random, &g_current->path);
    if (!choice) {
      return nullptr;
    }
    track = Open(*choice);
    if (!track) {
      return nullptr;
    }
    name = choice->name;
    REXLOG_INFO("custom_music: {} ({}): {} ended; {} next", g_category, "shuffle",
                g_current->name, name);
    g_current = std::move(choice);
  }
  if (!track->Begin(sample_rate, channels, ahead)) {
    REXLOG_ERROR("custom_music: {} can't be decoded to {} Hz, {} channel(s); looping instead",
                 name, sample_rate, channels);
    return nullptr;
  }
  return track;
}

bool Shuffle() {
  return REXCVAR_GET(custom_music_shuffle);
}

bool Logging() {
  return REXCVAR_GET(custom_music_log);
}

}  // namespace custom_music

// Bgm::Load (r4 the bank's name): the roll, then the game's own load.
extern "C" REX_FUNC(sub_82106540) {
  REX_FUNC_PROLOGUE();
  Choose(uint32_t(ctx.lr), GuestString(base, ctx.r4.u32, 32));
  __imp__sub_82106540(ctx, base);
}
