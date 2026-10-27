// One custom music file (ogg, opus, flac, mp3, wav) decoded with our FFmpeg
// into the samples the game's music voice reads: 16-bit big-endian,
// interleaved, at the voice's sample rate and channel count. It plays in the
// place of the XMA sound the game started (src/hooks/xma.cpp), read the same
// way as audio::xma::Stream: how many samples are ready, where, how many
// taken, from a position. A loop is a jump of the position.
//
// Opening reads the file's headers and starts the decoder (the slow part,
// done on the game thread when the music is chosen); Begin sets the output
// format once the voice is known. Decoded samples are kept, as the XMA
// stream keeps them, so a loop jumps back without decoding again: about
// 11 MB a minute for 48 kHz stereo. A shuffling track hands over to the next
// track at its end and frees what it decoded.
//
// Plain C++: only this library sees FFmpeg's headers.

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace audio::music {

// What a file holds, read from its headers.
struct FileInfo {
  double seconds = 0;  // 0 when the file doesn't say
  uint32_t sample_rate = 0;
  uint32_t channels = 0;
  std::string codec;
};

class Track {
 public:
  ~Track();
  Track(const Track&) = delete;
  Track& operator=(const Track&) = delete;

  // The headers of `path`, or nullopt (and why, in `error`) if FFmpeg can't
  // open it or it has no audio.
  static std::optional<FileInfo> Probe(const std::filesystem::path& path, std::string* error);

  // Opens `path` and its decoder; nullptr (and why, in `error`) on failure.
  static std::unique_ptr<Track> Open(const std::filesystem::path& path, std::string* error);

  // How loud a file is, found by decoding all of it at its own rate and
  // channels: lufs is its integrated loudness (ITU-R BS.1770, gated), peak_db
  // its sample peak (0 is full scale). -200 for either when there's nothing
  // to measure (silence, or under 400 ms). nullopt (and why, in `error`) if
  // the file can't be decoded.
  struct Loudness {
    double lufs = -200;
    double peak_db = -200;
  };
  static std::optional<Loudness> Measure(const std::filesystem::path& path, std::string* error);

  // Sets the output format (the voice's) and decodes the first `ahead`
  // samples. False if the resampler can't be made.
  bool Begin(uint32_t sample_rate, uint32_t channels, uint32_t ahead);

  // The level the samples are output at, as a factor (1: as decoded, the
  // default). Set before Begin; the level comes out of the samples as they
  // are converted, so a shuffle's next track keeps its own.
  void SetGain(double gain) { gain_ = gain; }
  double gain() const { return gain_; }

  // The loop region, in seconds from the track's start; `end` 0 or past the
  // track's end means the track's end. Without one, the whole track loops.
  void SetRegion(double start, double end);
  // Whether to loop and how often, as the game asked for its own sound (its
  // XMA loop data's count): 0 never (the track plays once), 255 forever.
  void SetLoopCount(uint32_t count);
  // Shuffle instead of loop: where the track would loop (the game's loop
  // count not 0), it plays the whole file to its end and goes on to the
  // track given to Chain, without a gap. The loop region is ignored. If
  // shuffle is cleared before the end, the track loops as usual.
  void SetShuffle(bool shuffle);
  bool shuffle() const { return shuffle_; }
  // True when a shuffling track is decoded to its end, will go on, and has
  // no next track yet.
  bool WantsNext() const;
  // The track to go on to (begun, as this one was). It takes this one's
  // place when this one ends, keeping the loop count.
  void Chain(std::unique_ptr<Track> next);

  // Decodes until `ahead` samples past the position are ready or the file
  // ends.
  void DecodeAhead(uint32_t ahead);

  // As audio::xma::Stream: samples readable in one go from the position (up
  // to the loop end or what's decoded), and in all (past a loop jump too), at
  // most `cap`.
  uint32_t Contiguous(uint32_t cap) const;
  uint32_t Ready(uint32_t cap) const;
  // Takes up to `count` samples (no further than Contiguous); returns how
  // many. At the loop end, the position jumps back.
  uint32_t Consume(uint32_t count);
  const uint8_t* SamplesAt(uint32_t position) const {
    return pcm_.data() + size_t(position) * channels_ * 2;
  }
  uint32_t position() const { return position_; }
  // False once played to the end with no loops left (or on a decode error).
  bool playing() const;

  uint32_t decoded() const { return decoded_; }
  // Samples a channel once all is decoded; UINT32_MAX until then.
  uint32_t total() const { return total_; }
  uint32_t loop_start() const { return loop_start_; }
  uint32_t loop_end() const { return LoopEnd(); }
  uint32_t loops_left() const { return loops_left_; }
  const std::filesystem::path& path() const { return path_; }

 private:
  struct Impl;
  explicit Track(std::unique_ptr<Impl> impl);
  void Convert(const uint8_t* const* data, int samples);
  void ResolveRegion();
  uint32_t LoopEnd() const;
  uint32_t Limit() const;
  void Adopt(Track& next);

  std::unique_ptr<Impl> impl_;
  std::filesystem::path path_;
  uint32_t channels_ = 0;
  uint32_t sample_rate_ = 0;
  std::vector<uint8_t> pcm_;
  uint32_t decoded_ = 0;
  uint32_t total_ = UINT32_MAX;
  uint32_t position_ = 0;
  bool finished_ = false;  // the decoder and resampler are drained
  bool failed_ = false;
  // The region as given (seconds), turned into samples at the output rate.
  double region_start_ = 0, region_end_ = 0;
  uint32_t loop_start_ = 0;
  uint32_t loop_end_ = UINT32_MAX;  // UINT32_MAX: the track's end
  uint32_t loops_left_ = 0;
  bool shuffle_ = false;
  double gain_ = 1.0;
  std::unique_ptr<Track> next_;
};

}  // namespace audio::music
