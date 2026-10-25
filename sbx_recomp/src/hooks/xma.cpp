// The audio library's XMA layer over our own decoding (audio/xma_stream.h):
// no XMA contexts, registers or hardware, so the SDK's XMA decoder has
// nothing to do. Ported from ITC-Recomp (its src/hooks/xma.cpp, B2 in its
// docs/rexglue-removal.md, the functions in its docs/functions/audio.md),
// 2026-10-25. SBX's addresses: ITC's 8245C650-8245D4D0 matched by code
// (docs/itc_to_sbx_functions.tsv, and the rest by comparing the asm: the
// same library, a few instructions apart); SBX's source voice is ITC's with
// its fields 4 bytes earlier, so its XMA object is at voice +180 (ITC +184),
// reached only through these helpers (checked: every load of +180 goes to
// r3 for a call here). SBX's library has no frame walk (ITC sub_8246A5F8)
// and no ITC 8245CE10 / 8245D010, and its flush (sub_824539F0) does call the
// input position query (sub_82446018, never called in ITC's play).
//
// A source voice's XMA object is an opaque handle (a small system heap
// block) for an Object here. Each stream decodes its sound with
// audio::xma::Stream (all its samples kept host-side, decoded only a little
// ahead of the reading, as the 360's output ring was; see
// audio/xma_stream.h) and answers "how
// many ready, where, how many taken" from a position; "where" is a staging
// buffer in guest memory (the ring's size) the samples at the position are
// copied into, which the voice's recompiled mixer reads. "Ready" is capped at
// the size of the output ring the object would have had, so the voice never
// sees more than it could on the 360. Lock, wait and unlock have nothing to
// wait for; unlock decodes ahead.
//
// --native_xma=false (read once, at the first XMA object) runs the game's own
// layer on the SDK's emulated XMA hardware instead.

#include "guest.h"

#include "audio/xma_packets.h"
#include "audio/xma_stream.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

REXCVAR_DEFINE_BOOL(native_xma, true, "SBX",
                    "Decode the game's XMA sounds (the music, the menu sounds) with our own "
                    "FFmpeg instead of the SDK's emulated XMA hardware");

namespace {

// How far decoding keeps ahead of the reading: about what the 360's output
// ring held (twice the voice's ring size; at least this many).
constexpr uint32_t kMinimumAhead = 4096;
constexpr uint32_t kBothInUse = 0x80070005;

struct StreamState {
  uint32_t sample_rate = 0;
  uint32_t ring_samples = 0;  // the output ring's size the voice asked for: the "ready" cap
  uint32_t channels = 0;
  uint32_t input = 0;    // the sound's data (guest), 0 when none
  uint32_t pending = 0;  // a second sound given while one plays (not seen in play)
  uint32_t pending_size = 0;
  uint32_t staging = 0;  // where the voice reads samples (guest, system heap)
  uint32_t staging_samples = 0;
  uint32_t consumed = 0;
  audio::xma::Stream stream;
};

struct Object {
  std::mutex mutex;
  bool contexts_made = false;
  std::vector<std::unique_ptr<StreamState>> streams;
};

std::mutex g_objects_mutex;
std::unordered_map<uint32_t, std::shared_ptr<Object>> g_objects;

rex::memory::Memory* Memory() {
  return rex::system::kernel_state()->memory();
}

// --native_xma, latched at the first call: objects made one way are only
// ever handled that way.
bool Native() {
  static const bool native = REXCVAR_GET(native_xma);
  return native;
}

std::shared_ptr<Object> Find(uint32_t handle) {
  std::lock_guard lock(g_objects_mutex);
  auto it = g_objects.find(handle);
  return it == g_objects.end() ? nullptr : it->second;
}

uint32_t Ahead(const StreamState& s) {
  return std::max(s.ring_samples * 2, kMinimumAhead);
}

void FreeBuffer(StreamState& s) {
  s.stream.End();
  if (s.staging) {
    Memory()->SystemHeapFree(s.staging);
    s.staging = 0;
  }
  s.input = 0;
}

// Copies the samples readable in one go from the position (at most the
// ring's size) into the staging buffer; returns how many.
uint32_t Stage(uint8_t* base, StreamState& s) {
  const uint32_t count = std::min(s.stream.Contiguous(s.ring_samples), s.staging_samples);
  if (count) {
    std::memcpy(REX_RAW_ADDR(s.staging), s.stream.SamplesAt(s.stream.position()),
                size_t(count) * s.channels * 2);
  }
  return count;
}

void Reset(StreamState& s) {
  FreeBuffer(s);
  s.pending = s.pending_size = 0;
  s.consumed = 0;
}

bool Start(uint8_t* base, StreamState& s, uint32_t data, uint32_t size) {
  FreeBuffer(s);
  // REX_RAW_ADDR, not base + data: the 0xE0000000-0xFFFFFFFF range sits
  // 4 KB further on in host memory (REX_PHYS_HOST_OFFSET), and SBX's banks
  // load there (FD8C6800...). Read without it, every sound was the 4 KB
  // before its own: the wrong sound, silence or garbage (the user's first
  // three runs, 2026-10-25).
  const uint8_t* host = REX_RAW_ADDR(data);
  // The voice reads at most a ring's worth at a time, from here (a copy of
  // the decoded samples at the position; audio/xma_stream.h keeps them all).
  s.staging_samples = std::max(s.ring_samples, 1u);
  const uint32_t bytes = s.staging_samples * s.channels * 2;
  s.staging = Memory()->SystemHeapAlloc(bytes);
  if (!s.staging) {
    REXLOG_ERROR("xma: no system heap for {} bytes", bytes);
    return false;
  }
  if (!s.stream.Begin(host, size, s.channels, s.sample_rate)) {
    REXLOG_ERROR("xma: couldn't start the decoder ({} channels, {} Hz)", s.channels,
                 s.sample_rate);
    FreeBuffer(s);
    return false;
  }
  s.input = data;
  s.stream.DecodeAhead(Ahead(s));
  return true;
}

// The stream's sound is still playing (its input "valid" to the voice).
bool Valid(const StreamState& s) {
  return s.input && s.stream.playing();
}

// Moves to a second sound once the first has played out.
void Advance(uint8_t* base, StreamState& s) {
  if (s.input && !s.stream.playing()) {
    FreeBuffer(s);
    if (s.pending) {
      const uint32_t data = s.pending, size = s.pending_size;
      s.pending = s.pending_size = 0;
      Start(base, s, data, size);
    }
  }
}

// Runs `f(state)` on stream r4 of object r3 under its lock; r3 = `none` for
// a handle or stream that isn't ours.
template <typename F>
void OnStream(PPCContext& ctx, uint32_t none, F f) {
  auto object = Find(ctx.r3.u32);
  const uint32_t index = ctx.r4.u32;
  if (!object) {
    REXLOG_ERROR("xma: unknown object {:08X} (from {:08X})", ctx.r3.u32, uint32_t(ctx.lr));
    ctx.r3.u64 = none;
    return;
  }
  std::lock_guard lock(object->mutex);
  if (index >= object->streams.size()) {
    REXLOG_ERROR("xma: no stream {} in {:08X}", index, ctx.r3.u32);
    ctx.r3.u64 = none;
    return;
  }
  ctx.r3.u64 = f(*object->streams[index]);
}

}  // namespace

// XmaObject::Create (r3 the stream count, r4 their descriptions, 12 bytes
// each: sample rate +0, output ring samples +4, channels +8; r5 flags, bit 0
// = contexts made later; r6 out): the object and its handle.
extern "C" REX_FUNC(sub_82445690) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445690(ctx, base);
    return;
  }
  const uint32_t count = ctx.r3.u32, descriptions = ctx.r4.u32, flags = ctx.r5.u32,
                 out = ctx.r6.u32;
  auto object = std::make_shared<Object>();
  object->contexts_made = !(flags & 1);
  for (uint32_t i = 0; i < count; i++) {
    auto s = std::make_unique<StreamState>();
    const uint32_t d = descriptions + 12 * i;
    s->sample_rate = guest::LoadU32(base, d);
    s->ring_samples = guest::LoadU32(base, d + 4);
    s->channels = guest::LoadU8(base, d + 8);
    object->streams.push_back(std::move(s));
  }
  const uint32_t handle = Memory()->SystemHeapAlloc(16);
  if (!handle) {
    ctx.r3.u64 = 0x8007000E;
    return;
  }
  {
    std::lock_guard lock(g_objects_mutex);
    g_objects[handle] = std::move(object);
  }
  guest::StoreU32(base, out, handle);
  ctx.r3.u64 = 0;
}

// XmaObject::Destroy (r3 the object).
extern "C" REX_FUNC(sub_824458C0) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_824458C0(ctx, base);
    return;
  }
  const uint32_t handle = ctx.r3.u32;
  std::shared_ptr<Object> object;
  {
    std::lock_guard lock(g_objects_mutex);
    auto it = g_objects.find(handle);
    if (it != g_objects.end()) {
      object = std::move(it->second);
      g_objects.erase(it);
    }
  }
  if (object) {
    std::lock_guard lock(object->mutex);
    for (auto& s : object->streams) {
      Reset(*s);
    }
    Memory()->SystemHeapFree(handle);
  }
  ctx.r3.u64 = 0;
}

// XmaObject::ReleaseContexts and MakeContexts: only the "made" state is
// kept (sub_82445988 reads it).
extern "C" REX_FUNC(sub_82445910) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445910(ctx, base);
    return;
  }
  if (auto object = Find(ctx.r3.u32)) {
    std::lock_guard lock(object->mutex);
    object->contexts_made = false;
  }
  ctx.r3.u64 = 0;
}

extern "C" REX_FUNC(sub_824460E0) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_824460E0(ctx, base);
    return;
  }
  if (auto object = Find(ctx.r3.u32)) {
    std::lock_guard lock(object->mutex);
    object->contexts_made = true;
  }
  ctx.r3.u64 = 0;
}

extern "C" REX_FUNC(sub_82445988) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445988(ctx, base);
    return;
  }
  auto object = Find(ctx.r3.u32);
  ctx.r3.u64 = object && object->contexts_made ? 1 : 0;
}

// XmaObject::SetInputBuffer (object, stream, data, size in bytes).
extern "C" REX_FUNC(sub_82445998) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445998(ctx, base);
    return;
  }
  const uint32_t data = ctx.r5.u32, size = ctx.r6.u32;
  OnStream(ctx, kBothInUse, [&](StreamState& s) -> uint32_t {
    if (!s.input) {
      return Start(base, s, data, size) ? 0 : kBothInUse;
    }
    if (s.pending) {
      return kBothInUse;
    }
    REXLOG_WARN("xma: a second sound ({:08X}) given while {:08X} plays", data, s.input);
    s.pending = data;
    s.pending_size = size;
    return 0;
  });
}

// (object, stream, address): 1 if a sound at that address is still playing.
extern "C" REX_FUNC(sub_82445A70) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445A70(ctx, base);
    return;
  }
  const uint32_t address = ctx.r5.u32;
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t {
    return (Valid(s) && s.input == address) || s.pending == address;
  });
}

// (object, stream): 1 unless both input slots are taken.
extern "C" REX_FUNC(sub_82445AD0) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445AD0(ctx, base);
    return;
  }
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t { return !(Valid(s) && s.pending); });
}

// (object, stream): 1 if no sound is playing.
extern "C" REX_FUNC(sub_82445B28) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445B28(ctx, base);
    return;
  }
  OnStream(ctx, 1, [&](StreamState& s) -> uint32_t { return !Valid(s) && !s.pending; });
}

// (object, stream): samples ready.
extern "C" REX_FUNC(sub_82445B90) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445B90(ctx, base);
    return;
  }
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t {
    return s.input ? s.stream.Ready(s.ring_samples) : 0;
  });
}

// (object, stream, out address): where to read (to r5) and the samples
// readable there in one go.
extern "C" REX_FUNC(sub_82445DC0) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445DC0(ctx, base);
    return;
  }
  const uint32_t out = ctx.r5.u32;
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t {
    guest::StoreU32(base, out, s.staging);
    return s.input ? Stage(base, s) : 0;
  });
}

// (object, stream, samples, out address): takes up to that many samples;
// where they were read from to r6; returns how many.
extern "C" REX_FUNC(sub_82445C08) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445C08(ctx, base);
    return;
  }
  const uint32_t count = ctx.r5.u32, out = ctx.r6.u32;
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t {
    guest::StoreU32(base, out, s.staging);
    if (!s.input) {
      return 0;
    }
    Stage(base, s);  // what's taken, where the caller may read it
    const uint32_t taken = s.stream.Consume(std::min(count, s.staging_samples));
    s.consumed += taken;
    s.stream.DecodeAhead(Ahead(s));
    return taken;
  });
}

// (object, stream): samples taken so far.
extern "C" REX_FUNC(sub_82445FF8) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445FF8(ctx, base);
    return;
  }
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t { return s.consumed; });
}

// XmaObject::ResetStream (object, stream): the sound stopped.
extern "C" REX_FUNC(sub_82445E68) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445E68(ctx, base);
    return;
  }
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t {
    Reset(s);
    return 0;
  });
}

// (object, stream): the error status; none.
extern "C" REX_FUNC(sub_82445F30) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445F30(ctx, base);
    return;
  }
  OnStream(ctx, 0, [&](StreamState&) -> uint32_t { return 0; });
}

// XmaObject::SetLoopData (object, stream, r5 the loop data: start bit +0,
// end bit +4 (26 bits each), subframes played in the last frame - 1 +8,
// subframes skipped at the start +9, count +10).
extern "C" REX_FUNC(sub_82445F80) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445F80(ctx, base);
    return;
  }
  const uint32_t loop = ctx.r5.u32;
  const uint32_t start_bit = guest::LoadU32(base, loop) & 0x3FFFFFF;
  const uint32_t end_bit = guest::LoadU32(base, loop + 4) & 0x3FFFFFF;
  const uint32_t end_subframe = guest::LoadU8(base, loop + 8) & 3;
  const uint32_t skip = guest::LoadU8(base, loop + 9) & 7;
  const uint32_t count = guest::LoadU8(base, loop + 10);
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t {
    s.stream.SetLoop(start_bit, end_bit, skip, end_subframe, count);
    return 0;
  });
}

// (object, stream, r5 the start's bit offset, r6 subframes to skip).
extern "C" REX_FUNC(sub_82446088) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82446088(ctx, base);
    return;
  }
  const uint32_t bit = ctx.r5.u32 & 0x3FFFFFF, skip = ctx.r6.u32 & 7;
  OnStream(ctx, 0, [&](StreamState& s) -> uint32_t {
    s.stream.SetStart(bit, skip);
    return 0;
  });
}

// XmaObject::Lock, IsLocked, Unlock: nothing to wait for. Unlock decodes
// ahead and moves finished streams on.
extern "C" REX_FUNC(sub_824461B0) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_824461B0(ctx, base);
    return;
  }
  ctx.r3.u64 = 0;
}

extern "C" REX_FUNC(sub_82446310) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82446310(ctx, base);
    return;
  }
  ctx.r3.u64 = 1;
}

// The hardware wait (r3 the object; polled sub_82446310, and after an eighth
// of a second reset the XMA hardware through its registers): nothing to wait
// for, 0 (answered at once).
extern "C" REX_FUNC(sub_82446248) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82446248(ctx, base);
    return;
  }
  ctx.r3.u64 = 0;
}

extern "C" REX_FUNC(sub_82446490) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82446490(ctx, base);
    return;
  }
  if (auto object = Find(ctx.r3.u32)) {
    std::lock_guard lock(object->mutex);
    for (auto& s : object->streams) {
      Advance(base, *s);
      s->stream.DecodeAhead(Ahead(*s));
    }
  }
  ctx.r3.u64 = 0;
}

// Reads the XMA registers' context array address at boot: no hardware.
extern "C" REX_FUNC(sub_82445678) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82445678(ctx, base);
    return;
  }
}

extern "C" REX_FUNC(sub_82446018) {
  REX_FUNC_PROLOGUE();
  if (!Native()) {
    __imp__sub_82446018(ctx, base);
    return;
  }
  // (object, stream, r5 out): the current input buffer's address, the input
  // read offset and word 1's low 3 bits. From SBX's flush (sub_824539F0);
  // never called in ITC's play. Zeros (no input position kept); logged once.
  static bool logged = false;
  if (!logged) {
    logged = true;
    REXLOG_WARN("xma: sub_82446018 (the input position) called from {:08X}; answered 0",
                uint32_t(ctx.lr));
  }
  for (uint32_t i = 0; i < 3; i++) {
    guest::StoreU32(base, ctx.r5.u32 + 4 * i, 0);
  }
  ctx.r3.u64 = 0;
}

