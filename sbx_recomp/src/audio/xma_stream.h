// One XMA sound decoded with FFmpeg's xma2 decoder into 16-bit big-endian
// samples (interleaved for stereo), read by position: the game's XMA source
// voice asks how many samples are ready, where to read them and how many it
// took. A loop is a jump of the position. No ring, blocks or XMA context.
// Plain C++: the data is given as a host pointer (the adapters,
// src/hooks/xma.cpp, translate guest addresses and copy samples out).
//
// From ITC-Recomp's (its B2 in docs/rexglue-removal.md), changed for SBX
// (2026-10-25):
// - nothing is worked out from the data up front: the decoded samples go
//   into a growing host buffer, not one sized from the data (SBX's banks
//   are XMA1-style, with no frame counts in the packet headers);
// - decoding stays only a little ahead of the reading (the caller's
//   `ahead`), as the 360's decoder did with its small output ring, and a
//   packet whose first 16 bytes are zero (never an XMA packet) is waited
//   for, not decoded;
// - the start and loop points (bit offsets into the data) are turned into
//   samples once the data up to them has been fed to the decoder, and a loop
//   ends where the decoded samples do if that's sooner.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace audio::xma {

class Stream {
 public:
  Stream();
  ~Stream();
  Stream(const Stream&) = delete;
  Stream& operator=(const Stream&) = delete;

  // Starts a sound: `data` and `size` the game's XMA data (stream 0).
  bool Begin(const uint8_t* data, uint32_t size, uint32_t channels, uint32_t sample_rate);
  void End();
  bool active() const { return data_ != nullptr; }

  // Where playing starts: the frame at bit offset `bit` of the data, less
  // `skip_subframes` subframes of 128 samples (taken once the data is fed).
  void SetStart(uint32_t bit, uint32_t skip_subframes);
  // The loop: from the frame at `start_bit` (after `skip_subframes`) to the
  // frame at `end_bit` (its subframes 0 to `end_subframe`), `count` times,
  // 255 forever (taken once the data is fed).
  void SetLoop(uint32_t start_bit, uint32_t end_bit, uint32_t skip_subframes,
               uint32_t end_subframe, uint32_t count);

  // Decodes until `ahead` samples past the position are ready, the data
  // ends, or the next packet isn't loaded yet.
  void DecodeAhead(uint32_t ahead);

  // Samples ready to read in one go from the position (up to the loop end or
  // what's decoded), and in all (past a loop jump too), at most `cap`.
  uint32_t Contiguous(uint32_t cap) const;
  uint32_t Ready(uint32_t cap) const;
  // Takes up to `count` samples from the position (no further than
  // Contiguous); returns how many. At the loop end, the position jumps back.
  uint32_t Consume(uint32_t count);
  // The decoded samples from `position` on (big-endian, interleaved).
  const uint8_t* SamplesAt(uint32_t position) const {
    return pcm_.data() + size_t(position) * channels_ * 2;
  }
  // The position, in samples from the sound's start.
  uint32_t position() const { return position_; }
  // False once the sound is played to its end with no loops left.
  bool playing() const;

  uint32_t channels() const { return channels_; }
  uint32_t loop_start() const { return loop_start_; }
  uint32_t loop_end() const { return loop_end_; }
  uint32_t decoded() const { return decoded_; }
  uint32_t total() const { return total_; }
  // Packets fed to the decoder, and those waited for (not loaded) so far.
  uint32_t fed_packets() const;
  uint32_t waits() const { return waits_; }

 private:
  // Timeline samples (frames x 512 from the data's start) to decoded
  // samples: the decoder drops the first 64.
  static uint32_t ToDecoded(uint32_t timeline);
  void Drain();
  uint32_t Limit() const;
  uint32_t LoopEnd() const;
  void ResolvePoints();

  struct Decoder;
  std::unique_ptr<Decoder> decoder_;
  const uint8_t* data_ = nullptr;
  uint32_t size_ = 0;
  uint32_t channels_ = 0;
  std::vector<uint8_t> pcm_;
  uint32_t fed_ = 0;  // bytes of data given to the decoder
  bool flushed_ = false;
  uint32_t decoded_ = 0;  // samples a channel in pcm_
  uint32_t total_ = UINT32_MAX;  // samples a channel once all is decoded
  uint32_t position_ = 0;
  uint32_t waits_ = 0;
  // The start and loop as given (bit offsets), until resolved.
  bool start_pending_ = false;
  uint32_t start_bit_ = 0, start_skip_ = 0;
  bool loop_pending_ = false;
  uint32_t loop_start_bit_ = 0, loop_end_bit_ = 0, loop_skip_ = 0, loop_end_subframe_ = 0,
           loop_count_ = 0;
  uint32_t loop_start_ = 0;
  uint32_t loop_end_ = 0;
  uint32_t loops_left_ = 0;
};

}  // namespace audio::xma
