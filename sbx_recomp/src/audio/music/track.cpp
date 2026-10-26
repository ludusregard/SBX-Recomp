#include "audio/music/track.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/log.h>
#include <libavutil/mem.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace audio::music {

namespace {

constexpr uint32_t kLoopForever = 255;
constexpr size_t kIoBufferSize = 64 * 1024;

// The file read through our own AVIO (our FFmpeg has no file protocol), as
// the movie player reads its files (src/movies/player.cpp). 64-bit
// positions: POSIX's fseeko and ftello, the CRT's _fseeki64 and _ftelli64.
int Seek64(FILE* file, int64_t offset, int whence) {
#ifdef _WIN32
  return _fseeki64(file, offset, whence);
#else
  return fseeko(file, offset, whence);
#endif
}

int64_t Tell64(FILE* file) {
#ifdef _WIN32
  return _ftelli64(file);
#else
  return ftello(file);
#endif
}

FILE* OpenForReading(const std::filesystem::path& path) {
#ifdef _WIN32
  return _wfopen(path.c_str(), L"rb");
#else
  return std::fopen(path.c_str(), "rb");
#endif
}

int ReadFile(void* opaque, uint8_t* buffer, int size) {
  size_t read = std::fread(buffer, 1, size_t(size), static_cast<FILE*>(opaque));
  return read ? int(read) : AVERROR_EOF;
}

int64_t SeekFile(void* opaque, int64_t offset, int whence) {
  FILE* file = static_cast<FILE*>(opaque);
  if (whence == AVSEEK_SIZE) {
    int64_t at = Tell64(file);
    Seek64(file, 0, SEEK_END);
    int64_t size = Tell64(file);
    Seek64(file, at, SEEK_SET);
    return size;
  }
  return Seek64(file, offset, whence & ~AVSEEK_FORCE) == 0 ? Tell64(file) : -1;
}

std::string ErrorText(int error) {
  char text[AV_ERROR_MAX_STRING_SIZE] = {};
  av_strerror(error, text, sizeof(text));
  return text;
}

uint32_t Samples(double seconds, uint32_t rate) {
  return seconds > 0 ? uint32_t(std::llround(seconds * rate)) : 0;
}

}  // namespace

struct Track::Impl {
  FILE* file = nullptr;
  AVIOContext* io = nullptr;
  AVFormatContext* format = nullptr;
  AVCodecContext* codec = nullptr;
  int stream = -1;
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;
  SwrContext* resampler = nullptr;
  bool input_ended = false;  // every packet read and sent
  std::vector<int16_t> converted;

  ~Impl() {
    swr_free(&resampler);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec);
    avformat_close_input(&format);
    if (io) {
      av_freep(&io->buffer);
      avio_context_free(&io);
    }
    if (file) {
      std::fclose(file);
    }
  }

  bool Open(const std::filesystem::path& path, std::string* error) {
    static std::once_flag log_level;
    std::call_once(log_level, [] { av_log_set_level(AV_LOG_ERROR); });
    file = OpenForReading(path);
    if (!file) {
      *error = "can't be opened";
      return false;
    }
    auto* buffer = static_cast<uint8_t*>(av_malloc(kIoBufferSize));
    io = buffer ? avio_alloc_context(buffer, int(kIoBufferSize), 0, file, ReadFile, nullptr,
                                     SeekFile)
                : nullptr;
    format = avformat_alloc_context();
    if (!io || !format) {
      av_free(buffer);
      *error = "out of memory";
      return false;
    }
    format->pb = io;
    format->flags |= AVFMT_FLAG_CUSTOM_IO;
    // The name helps the probe (an mp3 by its extension).
    const std::u8string name = path.filename().u8string();
    int result = avformat_open_input(&format, reinterpret_cast<const char*>(name.c_str()),
                                     nullptr, nullptr);
    if (result < 0) {
      *error = "isn't a format we read (" + ErrorText(result) + ")";
      return false;
    }
    result = avformat_find_stream_info(format, nullptr);
    if (result < 0) {
      *error = "has no stream info (" + ErrorText(result) + ")";
      return false;
    }
    const AVCodec* decoder = nullptr;
    stream = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
    if (stream < 0 || !decoder) {
      *error = stream == AVERROR_DECODER_NOT_FOUND ? "has audio in a codec we don't decode"
                                                   : "has no audio";
      return false;
    }
    codec = avcodec_alloc_context3(decoder);
    if (!codec ||
        avcodec_parameters_to_context(codec, format->streams[stream]->codecpar) < 0 ||
        avcodec_open2(codec, decoder, nullptr) < 0) {
      *error = std::string("can't start the ") + decoder->name + " decoder";
      return false;
    }
    packet = av_packet_alloc();
    frame = av_frame_alloc();
    if (!packet || !frame) {
      *error = "out of memory";
      return false;
    }
    return true;
  }

  double Seconds() const {
    const AVStream* s = format->streams[stream];
    if (s->duration > 0 && s->duration != AV_NOPTS_VALUE) {
      return s->duration * av_q2d(s->time_base);
    }
    if (format->duration > 0 && format->duration != AV_NOPTS_VALUE) {
      return double(format->duration) / AV_TIME_BASE;
    }
    return 0;
  }
};

std::optional<FileInfo> Track::Probe(const std::filesystem::path& path, std::string* error) {
  Impl impl;
  if (!impl.Open(path, error)) {
    return std::nullopt;
  }
  FileInfo info;
  info.seconds = impl.Seconds();
  info.sample_rate = uint32_t(std::max(impl.codec->sample_rate, 0));
  info.channels = uint32_t(std::max(impl.codec->ch_layout.nb_channels, 0));
  info.codec = impl.codec->codec ? impl.codec->codec->name : "?";
  return info;
}

Track::Track(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
Track::~Track() = default;

std::unique_ptr<Track> Track::Open(const std::filesystem::path& path, std::string* error) {
  auto impl = std::make_unique<Impl>();
  if (!impl->Open(path, error)) {
    return nullptr;
  }
  std::unique_ptr<Track> track(new Track(std::move(impl)));
  track->path_ = path;
  return track;
}

bool Track::Begin(uint32_t sample_rate, uint32_t channels, uint32_t ahead) {
  if (channels < 1 || channels > 2 || sample_rate == 0) {
    return false;
  }
  sample_rate_ = sample_rate;
  channels_ = channels;
  // Room for the whole track from its length (no reallocating a big buffer
  // on the audio thread), with a little to spare.
  const double seconds = impl_->Seconds();
  if (seconds > 0) {
    pcm_.reserve(size_t(seconds * 1.02 * sample_rate + 4096) * channels * 2);
  }
  ResolveRegion();
  DecodeAhead(ahead);
  return !failed_;
}

void Track::SetRegion(double start, double end) {
  region_start_ = std::max(start, 0.0);
  region_end_ = std::max(end, 0.0);
  ResolveRegion();
}

void Track::ResolveRegion() {
  if (!sample_rate_) {
    return;
  }
  loop_start_ = Samples(region_start_, sample_rate_);
  loop_end_ = region_end_ > 0 ? Samples(region_end_, sample_rate_) : UINT32_MAX;
  if (loop_end_ <= loop_start_) {
    loop_start_ = 0;
    loop_end_ = UINT32_MAX;
  }
}

void Track::SetLoopCount(uint32_t count) {
  loops_left_ = count;
}

void Track::SetShuffle(bool shuffle) {
  shuffle_ = shuffle;
}

bool Track::WantsNext() const {
  return shuffle_ && loops_left_ && !next_ && finished_ && !failed_;
}

void Track::Chain(std::unique_ptr<Track> next) {
  next_ = std::move(next);
}

// Takes `next`'s place: its file, decoder and samples, from its start. The
// loop count and shuffle stay.
void Track::Adopt(Track& next) {
  impl_ = std::move(next.impl_);
  path_ = std::move(next.path_);
  pcm_ = std::move(next.pcm_);
  decoded_ = next.decoded_;
  total_ = next.total_;
  position_ = 0;
  finished_ = next.finished_;
  failed_ = next.failed_;
  region_start_ = next.region_start_;
  region_end_ = next.region_end_;
  loop_start_ = next.loop_start_;
  loop_end_ = next.loop_end_;
}

void Track::Convert(const uint8_t* const* data, int samples) {
  SwrContext* resampler = impl_->resampler;
  const int room = swr_get_out_samples(resampler, samples);
  if (room <= 0) {
    return;
  }
  impl_->converted.resize(size_t(room) * channels_);
  uint8_t* out = reinterpret_cast<uint8_t*>(impl_->converted.data());
  const int made = swr_convert(resampler, &out, room, data, samples);
  if (made <= 0) {
    if (made < 0) {
      failed_ = true;
    }
    return;
  }
  // Big-endian, as the voice reads the XMA decoder's samples.
  const size_t at = pcm_.size();
  pcm_.resize(at + size_t(made) * channels_ * 2);
  uint8_t* p = pcm_.data() + at;
  for (size_t i = 0; i < size_t(made) * channels_; i++) {
    const uint16_t v = uint16_t(impl_->converted[i]);
    *p++ = uint8_t(v >> 8);
    *p++ = uint8_t(v);
  }
  decoded_ += uint32_t(made);
}

void Track::DecodeAhead(uint32_t ahead) {
  if (next_) {
    next_->DecodeAhead(ahead);
  }
  if (!sample_rate_ || finished_ || failed_) {
    return;
  }
  Impl& d = *impl_;
  const uint64_t want = uint64_t(position_) + ahead;
  while (decoded_ < want && !finished_ && !failed_) {
    const int received = avcodec_receive_frame(d.codec, d.frame);
    if (received == 0) {
      if (!d.resampler) {
        // From the first frame's format (a decoder may only know it then).
        AVChannelLayout in_layout{}, out_layout{};
        if (d.frame->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
          av_channel_layout_default(&in_layout, d.frame->ch_layout.nb_channels);
        } else {
          av_channel_layout_copy(&in_layout, &d.frame->ch_layout);
        }
        av_channel_layout_default(&out_layout, int(channels_));
        const int made = swr_alloc_set_opts2(
            &d.resampler, &out_layout, AV_SAMPLE_FMT_S16, int(sample_rate_), &in_layout,
            AVSampleFormat(d.frame->format), d.frame->sample_rate, 0, nullptr);
        // Mono to stereo at full level in both channels (FFmpeg's own mix
        // puts it 3 dB down, as a center channel).
        const bool mono_to_stereo = in_layout.nb_channels == 1 && channels_ == 2;
        const double both[2] = {1.0, 1.0};
        av_channel_layout_uninit(&in_layout);
        av_channel_layout_uninit(&out_layout);
        if (made < 0 || (mono_to_stereo && swr_set_matrix(d.resampler, both, 1) < 0) ||
            swr_init(d.resampler) < 0) {
          failed_ = true;
          break;
        }
      }
      Convert(d.frame->extended_data, d.frame->nb_samples);
      av_frame_unref(d.frame);
      continue;
    }
    if (received == AVERROR_EOF) {
      // Drained: what the resampler still holds, then the end.
      if (d.resampler) {
        Convert(nullptr, 0);
      }
      finished_ = true;
      total_ = decoded_;
      break;
    }
    if (received != AVERROR(EAGAIN)) {
      failed_ = true;
      break;
    }
    if (d.input_ended) {
      // Sent the flush already but nothing more came: done.
      finished_ = true;
      total_ = decoded_;
      break;
    }
    // The decoder wants a packet: the next of our stream, or the flush.
    const int read = av_read_frame(d.format, d.packet);
    if (read < 0) {
      avcodec_send_packet(d.codec, nullptr);
      d.input_ended = true;
      continue;
    }
    if (d.packet->stream_index == d.stream) {
      // A damaged packet is skipped, as players do.
      avcodec_send_packet(d.codec, d.packet);
    }
    av_packet_unref(d.packet);
  }
}

uint32_t Track::LoopEnd() const {
  return std::min(loop_end_, total_);
}

uint32_t Track::Limit() const {
  const uint32_t end = loops_left_ && !shuffle_ ? LoopEnd() : total_;
  return std::min(end, decoded_);
}

uint32_t Track::Contiguous(uint32_t cap) const {
  const uint32_t limit = Limit();
  return std::min(cap, limit > position_ ? limit - position_ : 0);
}

uint32_t Track::Ready(uint32_t cap) const {
  uint32_t ready = Contiguous(cap);
  const uint32_t end = LoopEnd();
  if (shuffle_) {
    if (loops_left_ && next_ && total_ != UINT32_MAX && position_ + ready >= total_) {
      ready += next_->Contiguous(cap);
    }
  } else if (loops_left_ && end != UINT32_MAX && position_ + ready >= end &&
             decoded_ > loop_start_) {
    ready += std::min(decoded_, end) - loop_start_;
  }
  return std::min(cap, ready);
}

uint32_t Track::Consume(uint32_t count) {
  const uint32_t taken = std::min(count, Contiguous(count));
  position_ += taken;
  const uint32_t end = LoopEnd();
  if (shuffle_) {
    if (loops_left_ && next_ && total_ != UINT32_MAX && position_ >= total_) {
      const uint32_t left = loops_left_;
      std::unique_ptr<Track> next = std::move(next_);
      Adopt(*next);
      loops_left_ = left != kLoopForever ? left - 1 : left;
      if (!loops_left_) {
        shuffle_ = false;  // the game's last repeat: this one plays out
      }
    }
  } else if (loops_left_ && end != UINT32_MAX && position_ >= end) {
    // A region starting past the track's end (found only now) loops the
    // whole track.
    position_ = loop_start_ < end ? loop_start_ : 0;
    if (loops_left_ != kLoopForever) {
      loops_left_--;
    }
  }
  return taken;
}

bool Track::playing() const {
  if (failed_) {
    return position_ < decoded_;
  }
  return loops_left_ || !finished_ || position_ < total_;
}

}  // namespace audio::music
