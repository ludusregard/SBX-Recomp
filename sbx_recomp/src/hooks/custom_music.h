// Custom music: tracks from ADDON_DATA/CUSTOM_MUSIC/<CATEGORY>/ played in
// the place of the game's music (src/hooks/custom_music.cpp,
// docs/custom_music.md).

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

namespace audio::music {
class Track;
}

namespace custom_music {

// The game data root (assets/); its ADDON_DATA/CUSTOM_MUSIC is scanned now.
// Set by SbxApp before the game starts.
void SetGameDataRoot(const std::filesystem::path& root);

// For a stereo XMA sound starting (src/hooks/xma.cpp; `input` its data's
// guest address, `data` the same in host memory, `size` its bytes): the custom track chosen when the game last picked its
// music, begun at the voice's format with `ahead` samples decoded; nullptr to
// play the game's own. The first such sound after the pick takes it; after
// that only the same sound (the same input) starting again does.
// While the menu bank (BGM_SYSTEM) is loaded each sound that starts rolls
// for itself: the bank holds four songs and the game starts them without
// loading the bank again.
std::unique_ptr<audio::music::Track> TakeTrack(uint32_t input, const uint8_t* data, uint32_t size,
                                               uint32_t sample_rate, uint32_t channels,
                                               uint32_t ahead);

// Shuffle: the track to go on to when the playing custom track ends (and the
// game's sound would loop): another of the same folder, opened and begun as
// above. nullptr if shuffle is off or nothing can be picked; the caller then
// lets the track loop.
std::unique_ptr<audio::music::Track> NextTrack(uint32_t sample_rate, uint32_t channels,
                                               uint32_t ahead);

// Whether --custom_music_shuffle is on.
bool Shuffle();

// Whether --custom_music_log is on (for xma.cpp's own messages).
bool Logging();

}  // namespace custom_music
