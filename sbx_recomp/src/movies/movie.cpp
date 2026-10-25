#include "movies/movie.h"

#include "guest.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <memory>
#include <mutex>
#include <string>

#include "base/log.h"
#include "movies/player.h"
#include "native/renderer.h"
#include "native/state.h"

namespace movies::movie {

namespace {

// The players by movie object. Play and Release run on the main thread,
// RenderFrame on the render thread, which keeps its own reference while it
// draws (the original's Release waited for the frame on the device lock).
std::mutex g_mutex;
std::filesystem::path g_data_root;
std::map<uint32_t, std::shared_ptr<Player>> g_players;

std::shared_ptr<Player> PlayerOf(uint32_t movie) {
  std::lock_guard<std::mutex> lock(g_mutex);
  auto it = g_players.find(movie);
  return it == g_players.end() ? nullptr : it->second;
}

bool CommandDone(uint8_t* base, uint32_t movie) {
  return guest::LoadU32(base, movie + kCommandStatus) == kCommandDone;
}

void SetState(uint8_t* base, uint32_t movie, uint32_t state) {
  guest::StoreU32(base, movie + kState, state);
}

std::string GuestString(uint8_t* base, uint32_t address, uint32_t max) {
  std::string s;
  for (uint32_t i = 0; address && i < max; i++) {
    char c = char(guest::LoadU8(base, address + i));
    if (!c) {
      break;
    }
    s += c;
  }
  return s;
}

// The host file a game path names ("DATA\TXR\MOVIE\X.wmv", with or without
// a "game:\" prefix), under the data root; nothing for another device.
std::optional<std::filesystem::path> HostPath(const std::string& game_path) {
  std::string relative = game_path;
  const std::string kGame = "game:\\";
  if (relative.size() >= kGame.size() &&
      std::equal(kGame.begin(), kGame.end(), relative.begin(),
                 [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
    relative = relative.substr(kGame.size());
  }
  if (relative.find(':') != std::string::npos || g_data_root.empty()) {
    return std::nullopt;
  }
  std::replace(relative.begin(), relative.end(), '\\', '/');
  std::filesystem::path file = g_data_root / relative;
  std::error_code error;
  if (!std::filesystem::is_regular_file(file, error)) {
    return std::nullopt;
  }
  return file;
}

}  // namespace

void SetDataRoot(const std::filesystem::path& root) {
  g_data_root = root;
}

void Play(uint8_t* base, uint32_t movie, uint32_t path) {
  // strncpy(+48, path, 260): the path, then zeros to 260 bytes.
  std::string name = GuestString(base, path, kPathBytes);
  for (uint32_t i = 0; i < kPathBytes; i++) {
    guest::StoreU8(base, movie + kPath + i, i < name.size() ? uint8_t(name[i]) : 0);
  }
  std::shared_ptr<Player> player;
  std::string error = "isn't in the game's data";
  if (auto file = HostPath(name)) {
    player = Player::Open(*file, &error);
  }
  if (player) {
    ITC_LOG_INFO("movies: {} opened{}", name, player->HasSound() ? "" : " (no sound)");
  } else {
    ITC_LOG_ERROR("movies: {} {}; it ends at once", name, error);
  }
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_players[movie] = player;
  }
  guest::StoreU32(base, movie + kCommandStatus, kCommandDone);
  SetState(base, movie, kLoaded);
}

void Start(uint8_t* base, uint32_t movie) {
  if (!CommandDone(base, movie) || GetState(base, movie) == kNone) {
    return;
  }
  SetState(base, movie, kPlaying);
  if (std::shared_ptr<Player> player = PlayerOf(movie)) {
    player->Start();
    SetSounding(player.get());
  }
}

void Stop(uint8_t* base, uint32_t movie) {
  uint32_t state = GetState(base, movie);
  if (!CommandDone(base, movie) || (state != kPlaying && state != kPaused)) {
    return;
  }
  SetState(base, movie, kLoaded);
  if (std::shared_ptr<Player> player = PlayerOf(movie)) {
    player->Pause();
    SetSounding(nullptr);
  }
}

void Pause(uint8_t* base, uint32_t movie) {
  if (!CommandDone(base, movie) || GetState(base, movie) != kPlaying) {
    return;
  }
  SetState(base, movie, kPaused);
  if (std::shared_ptr<Player> player = PlayerOf(movie)) {
    player->Pause();
  }
}

void Resume(uint8_t* base, uint32_t movie) {
  if (!CommandDone(base, movie) || GetState(base, movie) != kPaused) {
    return;
  }
  SetState(base, movie, kPlaying);
  if (std::shared_ptr<Player> player = PlayerOf(movie)) {
    player->Resume();
  }
}

uint32_t GetState(uint8_t* base, uint32_t movie) {
  return guest::LoadU32(base, movie + kState);
}

void Release(uint8_t* base, uint32_t movie) {
  if (!CommandDone(base, movie) || GetState(base, movie) == kPlaying) {
    return;
  }
  std::shared_ptr<Player> player;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_players.find(movie);
    if (it == g_players.end()) {
      return;
    }
    player = std::move(it->second);
    g_players.erase(it);
  }
  SetSounding(nullptr);
}

void RenderFrame(uint8_t* base, uint32_t movie) {
  std::shared_ptr<Player> player = PlayerOf(movie);
  if (!player) {
    // Not opened (the file's missing): it ends at once.
    SetState(base, movie, kLoaded);
    return;
  }
  if (const Frame* frame = player->CurrentFrame()) {
    native::Renderer_DrawMovie(base, guest::LoadU32(base, native::kDevicePointer), frame->width,
                               frame->height, frame->y.data(), frame->u.data(),
                               frame->v.data());
  }
  if (player->Ended()) {
    SetState(base, movie, kLoaded);
  }
}

}  // namespace movies::movie
