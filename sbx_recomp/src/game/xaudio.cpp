// The XAudio library's small pieces as our own C++ (--native_xaudio, on by
// default): the voices', codecs' and allocator's vtables set and torn
// down, the reverb's defaults, the codecs' packet lists, and the library's
// allocation calls. The mixer and the codecs' decoding stay recompiled.

#include "game/guest_types.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_xaudio, true, "SBX",
                    "Our own C++ for the XAudio library's constructors, destructors and small "
                    "set-ups instead of the recompiled functions");

namespace {

using guest::AddressOf;
using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u16_be;
using guest::u32_be;

// The classes' vtables.
constexpr uint32_t kVoiceRootVtable = 0x82047590;  // The base every voice part ends as.
constexpr uint32_t kVoiceOutputsVtable = 0x82047A08;
constexpr uint32_t kAllocatorVtable = 0x82047838;
constexpr uint32_t kAllocatorInterfaceVtable = 0x82047850;
constexpr uint32_t kMatrixVtable = 0x82047940;
constexpr uint32_t kMatrixInterfaceVtable = 0x82047924;
constexpr uint32_t kXAudioCodecVtable = 0x820479D8;
constexpr uint32_t kXAudioCodecBaseVtable = 0x82047980;
constexpr uint32_t kCodecBaseVtable = 0x82048818;
constexpr uint32_t kPcmCodecVtables[3] = {0x820489E8, 0x820489C0, 0x82048968};

// The mixer's engine, for its callback.
constexpr uint32_t kMixerEngine = 0x82957364;

// Two rings of packets, each a list head linked to itself while empty,
// and the block their packets are in.
struct PacketLists {
  struct Ring {
    u32_be next;
    u32_be previous;
  };
  Ring free;
  u32_be free_count;
  Ring queued;
  u32_be queued_count;
  u32_be packets;  // From PacketLists::Allocate.
};
GUEST_SIZE(PacketLists, 28);

struct CodecBase {
  u32_be vtable;
  PacketLists packets;
  uint8_t format[56];  // The wave format it decodes.
  u16_be unknown_88[2];
  f32_be unknown_92[2];
  uint8_t unknown_100[48];
  u32_be unknown_148;
  uint8_t unknown_152;
};
GUEST_OFFSET(CodecBase, format, 32);
GUEST_OFFSET(CodecBase, unknown_148, 148);

struct XAudioCodec {
  u32_be vtable;
  uint8_t unknown_4[8];
  CodecBase base;
};
GUEST_OFFSET(XAudioCodec, base, 12);

struct PcmCodec {
  u32_be vtable;
  XAudioCodec codec;  // From +4; its +12 (the codec's base) has the third vtable.
};

// A voice's parts begin with one or two vtables.
struct VtablePair {
  u32_be first;
  u32_be second;
};

struct Allocator {
  VtablePair vtables;
  uint8_t unknown_8[16];
  u32_be unknown_24;
};

// The reverb's settings.
struct ReverbCore {
  u32_be unknown_0[4];
  uint8_t unknown_16[8];
  u32_be unknown_24[7];
  f32_be unknown_52[8];
  uint8_t unknown_84[16];
  u32_be stereo;
};
GUEST_OFFSET(ReverbCore, unknown_52, 52);
GUEST_OFFSET(ReverbCore, stereo, 100);

struct Voice {
  uint8_t unknown_0[52];
  u32_be format_words[2];
};

bool On() { return REXCVAR_GET(native_xaudio); }

void InitRing(PacketLists::Ring& ring) {
  ring.next = AddressOf(&ring);
  ring.previous = AddressOf(&ring);
}

void InitPacketLists(PacketLists* lists) {
  InitRing(lists->free);
  lists->free_count = 0;
  InitRing(lists->queued);
  lists->queued_count = 0;
}

void DestroyCodecBase(Guest& g, CodecBase* codec) {
  codec->vtable = kCodecBaseVtable;
  g.Call(sub_824565E8, &codec->packets);  // PacketLists::Clear
}

void DestroyXAudioCodec(Guest& g, XAudioCodec* codec) {
  codec->vtable = kXAudioCodecVtable;
  codec->base.vtable = kXAudioCodecBaseVtable;
  DestroyCodecBase(g, &codec->base);
  codec->vtable = kVoiceRootVtable;
}

}  // namespace

#define SBX_XAUDIO_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();              \
  if (!On()) {                      \
    __imp__##name(ctx, base);       \
    return;                         \
  }                                 \
  Guest g(ctx, base);

// XAudioSourceVoice and XAudioSubmixVoice::DeletingDestructor: r3 the
// voice, destroyed (recompiled). Return it.
extern "C" REX_FUNC(sub_82447AF0) {
  SBX_XAUDIO_FALLBACK(sub_82447AF0)
  const uint32_t voice = ctx.r3.u32;
  g.Call(sub_82447508, voice);
  ctx.r3.u64 = voice;
}

extern "C" REX_FUNC(sub_824483B0) {
  SBX_XAUDIO_FALLBACK(sub_824483B0)
  const uint32_t voice = ctx.r3.u32;
  g.Call(sub_82448118, voice);
  ctx.r3.u64 = voice;
}

// XAudio::Alloc and XAudio::Free: r4 and r5 on to XMemAlloc and XMemFree.
extern "C" REX_FUNC(sub_82447F90) {
  SBX_XAUDIO_FALLBACK(sub_82447F90)
  g.Call(sub_82295118, ctx.r4.u32, ctx.r5.u32);
}

extern "C" REX_FUNC(sub_82447FA0) {
  SBX_XAUDIO_FALLBACK(sub_82447FA0)
  g.Call(sub_822951B0, ctx.r4.u32, ctx.r5.u32);
}

// XAudioVoice::SetRootVtable: r3 the voice part. Returns it.
extern "C" REX_FUNC(sub_82448AB0) {
  SBX_XAUDIO_FALLBACK(sub_82448AB0)
  At<VtablePair>(ctx.r3.u32)->first = kVoiceRootVtable;
}

// XAudio::MixerCallback: the mixer's engine run (recompiled).
extern "C" REX_FUNC(sub_82448B90) {
  SBX_XAUDIO_FALLBACK(sub_82448B90)
  g.Call(sub_82444FB8, *At<u32_be>(kMixerEngine));
}

// XAudioDefaultAllocator::~XAudioDefaultAllocator: r3 the allocator.
// Returns it.
extern "C" REX_FUNC(sub_82449498) {
  SBX_XAUDIO_FALLBACK(sub_82449498)
  Allocator* allocator = At<Allocator>(ctx.r3.u32);
  allocator->vtables.first = kAllocatorVtable;
  allocator->vtables.second = kAllocatorInterfaceVtable;
  allocator->unknown_24 = 0;
  g.Call(sub_824492B8, allocator);
  ctx.r3.u64 = AddressOf(allocator);
}

// XAudioMatrix::Destruct: r3 the matrix. Returns it.
extern "C" REX_FUNC(sub_8244DCE0) {
  SBX_XAUDIO_FALLBACK(sub_8244DCE0)
  VtablePair* matrix = At<VtablePair>(ctx.r3.u32);
  matrix->first = kMatrixVtable;
  matrix->second = kMatrixInterfaceVtable;
  matrix->second = kVoiceRootVtable;
}

// XAudioCodec::Destruct: r3 the codec.
extern "C" REX_FUNC(sub_8244E760) {
  SBX_XAUDIO_FALLBACK(sub_8244E760)
  DestroyXAudioCodec(g, At<XAudioCodec>(ctx.r3.u32));
}

// XAudioVoice::ConstructOutputs: r3 the voice part. Returns it.
extern "C" REX_FUNC(sub_8244EDC0) {
  SBX_XAUDIO_FALLBACK(sub_8244EDC0)
  VtablePair* voice = At<VtablePair>(ctx.r3.u32);
  g.Call(sub_8244FB88, voice);
  voice->first = kVoiceOutputsVtable;
  ctx.r3.u64 = AddressOf(voice);
}

// ReverbCore::Defaults: r3 the reverb. Returns it.
extern "C" REX_FUNC(sub_82450598) {
  SBX_XAUDIO_FALLBACK(sub_82450598)
  ReverbCore* reverb = At<ReverbCore>(ctx.r3.u32);
  static constexpr uint32_t kWords[4] = {5, 5, 6, 6};
  static constexpr uint32_t kWords24[7] = {8, 8, 8, 4, 8, 4, 5};
  static constexpr float kFloats[8] = {5000.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 100.0f, 100.0f};
  for (int i = 0; i < 4; i++) {
    reverb->unknown_0[i] = kWords[i];
  }
  for (int i = 0; i < 7; i++) {
    reverb->unknown_24[i] = kWords24[i];
  }
  for (int i = 0; i < 8; i++) {
    reverb->unknown_52[i] = kFloats[i];
  }
}

// ReverbCore::SetStereo: r3 the reverb, r4 the setting. Returns r3.
extern "C" REX_FUNC(sub_82450A58) {
  SBX_XAUDIO_FALLBACK(sub_82450A58)
  At<ReverbCore>(ctx.r3.u32)->stereo = ctx.r4.u32;
}

// CodecBase::CodecBase: r3 the codec; its packet rings empty. Returns it.
extern "C" REX_FUNC(sub_82452980) {
  SBX_XAUDIO_FALLBACK(sub_82452980)
  CodecBase* codec = At<CodecBase>(ctx.r3.u32);
  codec->vtable = kCodecBaseVtable;
  InitPacketLists(&codec->packets);
}

// CodecBase::~CodecBase: r3 the codec.
extern "C" REX_FUNC(sub_824529C0) {
  SBX_XAUDIO_FALLBACK(sub_824529C0)
  DestroyCodecBase(g, At<CodecBase>(ctx.r3.u32));
}

// CodecBase::Init: r3 the codec, r4 its wave format, r5 how many packets,
// r7 the allocator they come from.
extern "C" REX_FUNC(sub_824529D8) {
  SBX_XAUDIO_FALLBACK(sub_824529D8)
  CodecBase* codec = At<CodecBase>(ctx.r3.u32);
  const uint32_t format = ctx.r4.u32, packets = ctx.r5.u32, allocator = ctx.r7.u32;
  struct Format {
    uint8_t bytes[56];
  };
  *reinterpret_cast<Format*>(codec->format) = *At<Format>(format);
  codec->unknown_92[0] = 1.0f;
  codec->unknown_92[1] = 1.0f;
  codec->unknown_152 = 32;
  codec->unknown_88[0] = 1;
  codec->unknown_88[1] = 1;
  codec->unknown_148 = 0;
  g.Call(sub_82456620, &codec->packets, packets, allocator);  // PacketLists::Allocate
}

// PcmCodec::~PcmCodec: r3 the codec. Returns it.
extern "C" REX_FUNC(sub_824554E8) {
  SBX_XAUDIO_FALLBACK(sub_824554E8)
  PcmCodec* codec = At<PcmCodec>(ctx.r3.u32);
  codec->vtable = kPcmCodecVtables[0];
  codec->codec.vtable = kPcmCodecVtables[1];
  codec->codec.base.vtable = kPcmCodecVtables[2];
  DestroyXAudioCodec(g, &codec->codec);
  ctx.r3.u64 = AddressOf(codec);
}

// PacketLists::Clear: r3 the lists; both rings emptied (recompiled).
extern "C" REX_FUNC(sub_824565E8) {
  SBX_XAUDIO_FALLBACK(sub_824565E8)
  PacketLists* lists = At<PacketLists>(ctx.r3.u32);
  g.Call(sub_824448F0, &lists->queued);
  g.Call(sub_824448F0, &lists->free);
}

// PacketLists::Init: r3 the lists. Returns them.
extern "C" REX_FUNC(sub_824566E0) {
  SBX_XAUDIO_FALLBACK(sub_824566E0)
  InitPacketLists(At<PacketLists>(ctx.r3.u32));
}

// XAudioVoice::SetFormatWords: r3 the voice, r4 the two words. Returns r3.
extern "C" REX_FUNC(sub_8244F7C8) {
  SBX_XAUDIO_FALLBACK(sub_8244F7C8)
  Voice* voice = At<Voice>(ctx.r3.u32);
  const u32_be* words = At<u32_be>(ctx.r4.u32);
  voice->format_words[0] = words[0];
  voice->format_words[1] = words[1];
}
