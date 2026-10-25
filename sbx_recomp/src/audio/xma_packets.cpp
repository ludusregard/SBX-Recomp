#include "audio/xma_packets.h"

#include <algorithm>

namespace audio::xma {

namespace {

uint32_t LoadBe32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

// A frame's length: the 15 bits at `bit` from the packet's start (read as
// the game does: the word at bit / 8, shifted by bit % 8).
uint32_t FrameLength(const uint8_t* packet, uint32_t bit) {
  return (LoadBe32(packet + (bit >> 3)) << (bit & 7)) >> 17;
}

}  // namespace

PacketHeader ReadPacketHeader(const uint8_t* packet) {
  const uint32_t word = LoadBe32(packet);
  const uint32_t offset = (word >> 11) & 0x7FFF;
  return {word >> 26, offset + 32, offset != 0x7FFF, word & 0xFF};
}

// SBX's banks hold XMA1-style packets: the header's top 4 bits are a
// sequence number and there's no frame count in it (ITC's banks, XMA2, have
// one in the top 6 bits), so frames are found by walking them: the packets'
// payloads (each after its 32-bit header) are one bitstream, each frame
// starting with its 15-bit length in bits (the field included), frames
// running on across packets. Checked against SBX's banks (2026-10-25):
// SB_SYSTEM_SE's 20 waves and bgm_tmp_04 (frames x 512 - 64 covers each
// wave's length; the music's loop start and end bits are frame starts).
// Single-stream data only (SBX's: every packet's skip count is 0).
namespace {

constexpr uint32_t kPacketBits = uint32_t(kPacketSize) * 8;
constexpr uint32_t kHeaderBits = 32;

// `bit` moved past a packet header if it's in one.
uint32_t PayloadBit(uint32_t bit) {
  uint32_t in_packet = bit % kPacketBits;
  return in_packet < kHeaderBits ? bit + (kHeaderBits - in_packet) : bit;
}

// The 15 bits from payload bit `bit` on (crossing a header if need be); false
// past the data's end.
bool ReadLength(const uint8_t* data, uint32_t total_bits, uint32_t bit, uint32_t& length) {
  length = 0;
  for (int i = 0; i < 15; i++) {
    bit = PayloadBit(bit);
    if (bit >= total_bits) {
      return false;
    }
    length = (length << 1) | ((data[bit >> 3] >> (7 - (bit & 7))) & 1);
    bit++;
  }
  return true;
}

// Calls f(index, absolute bit of the frame's start) for each frame of the
// data, until f returns false.
template <typename F>
void WalkFrames(const uint8_t* data, size_t size, F&& f) {
  const uint32_t packets = uint32_t(size / kPacketSize);
  if (!packets) {
    return;
  }
  const uint32_t total_bits = packets * kPacketBits;
  const PacketHeader first = ReadPacketHeader(data);
  if (!first.has_frame_start) {
    return;
  }
  uint32_t bit = first.first_frame_bit;
  for (uint32_t index = 0;; index++) {
    bit = PayloadBit(bit);
    uint32_t length;
    if (!ReadLength(data, total_bits, bit, length) || length < 15 || length == 0x7FFF) {
      return;
    }
    if (!f(index, bit)) {
      return;
    }
    // Past the frame's payload bits, skipping the headers on the way.
    for (uint32_t left = length; left > 0;) {
      bit = PayloadBit(bit);
      const uint32_t step = std::min(left, kPacketBits - bit % kPacketBits);
      bit += step;
      left -= step;
    }
    if (bit >= total_bits) {
      return;
    }
  }
}

}  // namespace

uint32_t CountFrames(const uint8_t* data, size_t size) {
  uint32_t frames = 0;
  WalkFrames(data, size, [&](uint32_t, uint32_t) {
    frames++;
    return true;
  });
  return frames;
}

uint32_t BitOffsetOfSample(const uint8_t* data, uint32_t size, uint32_t stream, uint32_t sample) {
  const uint32_t target = sample / kSamplesPerFrame;
  uint32_t at = stream * uint32_t(kPacketSize);
  uint32_t frames = 0;
  while (at < size) {
    const uint32_t word = LoadBe32(data + at);
    const uint32_t count = word >> 26;
    if (target < frames + count) {
      uint32_t bit = ((word >> 11) & 0x7FFF) + 32;
      for (uint32_t i = frames; i < target; i++) {
        bit += FrameLength(data + at, bit);
      }
      return at * 8 + bit;
    }
    frames += count;
    at += uint32_t(kPacketSize) * ((word & 0xFF) + 1);
  }
  return 0;
}

std::optional<uint32_t> FrameAtBitOffset(const uint8_t* data, uint32_t size, uint32_t stream,
                                         uint32_t bit) {
  if (stream != 0) {
    return std::nullopt;
  }
  std::optional<uint32_t> found;
  WalkFrames(data, size, [&](uint32_t index, uint32_t start) {
    if (start == bit) {
      found = index;
    }
    return start < bit;
  });
  return found;
}

}  // namespace audio::xma
