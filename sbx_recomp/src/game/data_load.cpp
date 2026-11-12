// The boot data's set-up as our own C++ (--native_data_load, on by
// default): the boot's load (once) and the objects it creates (the save
// slot, the race data, the defaults, the input recordings, the replay and
// its records, the data tables), their constructors, the defaults' bit
// sets and the loading screen's load.
//
// The constructors only set up their vectors: each vector's w word set
// (0.0, or 1.0 for some of a record's). Their vectors are listed as runs of
// byte offsets, since the objects' fields are only known by offset. The
// heap, the archive, the defaults' own init, the data tables' load and the
// other boot steps stay recompiled.

#include "game/math.h"
#include "game/replay_snapshots.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_data_load, true, "SBX",
                    "Our own C++ for the boot data's set-up (the boot's load, the objects it "
                    "creates and their constructors, the defaults' bit sets) instead of the "
                    "recompiled functions; the heap and the loads stay recompiled");

namespace {

using game::InputRecordings;
using game::Record5232;
using game::Replay;
using game::Vector;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_data_load); }

// Where the boot's objects are kept (writable).
constexpr uint32_t kRaceData = 0x826B580C;
constexpr uint32_t kInputRecordings = 0x826B582C;
constexpr uint32_t kReplay = 0x826B5830;
constexpr uint32_t kBootLoaded = 0x826B586C;

// The loading screen's two words, cleared before its movies load
// (writable).
constexpr uint32_t kLoadingScreenWords[] = {0x8280EEB0, 0x8280EE68};
// The loading screen's movies, by key.
constexpr uint32_t kLoadingMovies[] = {0x582E, 0x582D, 0x582F};

// The two static replays' records (seven each, 5232 bytes apart) and their
// destructors, registered with atexit.
constexpr uint32_t kReplay826D8520Records = 0x826FD440;
constexpr uint32_t kReplay826D8520Destructor = 0x825C0510;
constexpr uint32_t kReplay82816E50Records = 0x828452D0;
constexpr uint32_t kReplay82816E50Destructor = 0x825C0550;
constexpr uint32_t kStaticReplayRecords = 7;

// A run of vectors' constructors: `count` vectors from byte `first` of the
// object, `stride` bytes apart, each one's w set.
void ConstructVectors(uint32_t object, uint32_t first, uint32_t count, uint32_t stride,
                      float w = 0.0f) {
  for (uint32_t i = 0; i < count; i++) {
    At<Vector>(object + first + stride * i)->w = w;
  }
}

// Part464::Part464's vectors.
void ConstructPart464(uint32_t part) {
  ConstructVectors(part, 0, 10, 16);
  ConstructVectors(part, 176, 6, 32);
  ConstructVectors(part, 384, 4, 16);
}

// Member1136::Member1136's vectors.
void ConstructMember1136(uint32_t member) {
  ConstructVectors(member, 0, 3, 16);
  ConstructVectors(member, 64, 2, 32);
  ConstructVectors(member, 112, 2, 16);
  ConstructVectors(member, 336, 4, 16);
  ConstructVectors(member, 464, 2, 16);
  ConstructVectors(member, 512, 4, 16);
  // The same three runs in three blocks 368 bytes apart.
  for (uint32_t wheel = 0; wheel < 3; wheel++) {
    const uint32_t at = 736 + 368 * wheel;
    ConstructVectors(member, at, 2, 96);
    ConstructVectors(member, at + 112, 2, 32);
    ConstructVectors(member, at + 160, 3, 16);
  }
  ConstructVectors(member, 1840, 2, 32);
  ConstructVectors(member, 1888, 5, 16);
  ConstructVectors(member, 2000, 6, 16);
  ConstructVectors(member, 2416, 2, 16);
  ConstructVectors(member, 2480, 6, 16);
  ConstructVectors(member, 2864, 4, 16);
}

// The bits of a set: its words cleared and its first `bits` set (bit i at
// word i / 32, 1 << (i % 32)).
void FillBits(u32_be* words, uint32_t word_count, uint32_t bits) {
  for (uint32_t i = 0; i < word_count; i++) {
    const uint32_t from = i * 32;
    uint32_t word = 0;
    if (bits >= from + 32) {
      word = 0xFFFFFFFFu;
    } else if (bits > from) {
      word = (1u << (bits - from)) - 1;
    }
    words[i] = word;
  }
}

}  // namespace

#define SBX_DATA_LOAD_FALLBACK(name) \
  REX_FUNC_PROLOGUE();               \
  if (!On()) {                       \
    __imp__##name(ctx, base);        \
    return;                          \
  }                                  \
  Guest g(ctx, base);

// LoadingScreen::Load: its two words cleared, then its three movies loaded
// (sub_82245658).
extern "C" REX_FUNC(sub_82245750) {
  SBX_DATA_LOAD_FALLBACK(sub_82245750)
  for (uint32_t word : kLoadingScreenWords) {
    *At<u32_be>(word) = 0;
  }
  for (uint32_t movie : kLoadingMovies) {
    g.Call(sub_82245658, movie);
  }
}

// InputRecordings::InputRecordings: r3 the recordings, returned. Its
// fourteen records made.
extern "C" REX_FUNC(sub_82105618) {
  SBX_DATA_LOAD_FALLBACK(sub_82105618)
  const uint32_t address = ctx.r3.u32;
  InputRecordings* recordings = At<InputRecordings>(address);
  for (Record5232& record : recordings->records_a) {
    g.Call(sub_82104D90, &record);
  }
  for (Record5232& record : recordings->records_b) {
    g.Call(sub_82104D90, &record);
  }
  ctx.r3.u64 = address;
}

// Replay826D8520::StaticInit: the static replay's seven records made, its
// destructor registered (atexit, sub_8258C7E8).
extern "C" REX_FUNC(sub_825BF238) {
  SBX_DATA_LOAD_FALLBACK(sub_825BF238)
  for (uint32_t i = 0; i < kStaticReplayRecords; i++) {
    g.Call(sub_82104D90, kReplay826D8520Records + uint32_t(sizeof(Record5232)) * i);
  }
  g.Call(sub_8258C7E8, kReplay826D8520Destructor);
}

// Replay82816E50::StaticInit: the same for the other static replay.
extern "C" REX_FUNC(sub_825BF340) {
  SBX_DATA_LOAD_FALLBACK(sub_825BF340)
  for (uint32_t i = 0; i < kStaticReplayRecords; i++) {
    g.Call(sub_82104D90, kReplay82816E50Records + uint32_t(sizeof(Record5232)) * i);
  }
  g.Call(sub_8258C7E8, kReplay82816E50Destructor);
}

// Part464::Part464: r3 the part, returned.
extern "C" REX_FUNC(sub_82104410) {
  SBX_DATA_LOAD_FALLBACK(sub_82104410)
  ConstructPart464(ctx.r3.u32);
}

// Member4080::Member4080: r3 the member, returned. Its two parts (at +16
// and +480) made as Part464s, then its own vectors.
extern "C" REX_FUNC(sub_82104800) {
  SBX_DATA_LOAD_FALLBACK(sub_82104800)
  const uint32_t member = ctx.r3.u32;
  ConstructPart464(member + 16);
  ConstructPart464(member + 480);
  ConstructVectors(member, 944, 2, 16);
  ConstructVectors(member, 1056, 2, 16);
  ConstructVectors(member, 1104, 2, 32);
}

// Member1136::Member1136: r3 the member, returned.
extern "C" REX_FUNC(sub_82102A10) {
  SBX_DATA_LOAD_FALLBACK(sub_82102A10)
  ConstructMember1136(ctx.r3.u32);
}

// Record5232::Record5232: r3 the record, returned. Its own vectors (five
// runs of four with w = 1.0 first), its block at +1136 made as a
// Member1136, then its Member4080.
extern "C" REX_FUNC(sub_82104D90) {
  SBX_DATA_LOAD_FALLBACK(sub_82104D90)
  const uint32_t address = ctx.r3.u32;
  for (uint32_t i = 0; i < 5; i++) {
    ConstructVectors(address, 144 + 96 * i, 4, 16, 1.0f);
  }
  ConstructVectors(address, 624, 2, 16);
  ConstructVectors(address, 672, 3, 32);
  ConstructVectors(address, 752, 2, 32);
  ConstructVectors(address, 800, 2, 16);
  ConstructVectors(address, 960, 2, 16);
  ConstructVectors(address, 1008, 2, 80);
  ConstructVectors(address, 1104, 1, 16);
  Record5232* record = At<Record5232>(address);
  ConstructMember1136(guest::AddressOf(record->member_1136));
  g.Call(sub_82104800, &record->member_4080);
  ctx.r3.u64 = address;
}

// Bits224::Fill: 7 words, bits 0 to 198 set.
extern "C" REX_FUNC(sub_820CCAB0) {
  SBX_DATA_LOAD_FALLBACK(sub_820CCAB0)
  FillBits(At<u32_be>(ctx.r3.u32), 7, 199);
}

// Bits480::Fill: 15 words, bits 0 to 453 set.
extern "C" REX_FUNC(sub_820CCB08) {
  SBX_DATA_LOAD_FALLBACK(sub_820CCB08)
  FillBits(At<u32_be>(ctx.r3.u32), 15, 454);
}

// RaceData::Create: the race data (827,904 bytes) made (sub_82102340) into
// [0x826B580C], or 0.
extern "C" REX_FUNC(sub_82102390) {
  SBX_DATA_LOAD_FALLBACK(sub_82102390)
  uint32_t data = g.Call(sub_8258C000, 827904u);  // operator new
  if (data != 0) {
    data = g.Call(sub_82102340, data);
  }
  *At<u32_be>(kRaceData) = data;
}

// InputRecordings::Create: made into [0x826B582C] (0 when the allocation
// fails), and its +2238992 cleared.
extern "C" REX_FUNC(sub_82105678) {
  SBX_DATA_LOAD_FALLBACK(sub_82105678)
  uint32_t recordings = g.Call(sub_8258C000, uint32_t(sizeof(InputRecordings)));
  if (recordings != 0) {
    recordings = g.Call(sub_82105618, recordings);
  }
  *At<u32_be>(kInputRecordings) = recordings;
  // The original clears the word even when the allocation failed.
  *At<u32_be>(recordings + uint32_t(offsetof(InputRecordings, unknown_2238992))) = 0;
  ctx.r3.u64 = recordings;
}

// Replay::Create: made with its seven records into [0x826B5830] (0 when
// the allocation fails), and its +215372 = 1.
extern "C" REX_FUNC(sub_821056D0) {
  SBX_DATA_LOAD_FALLBACK(sub_821056D0)
  const uint32_t address = g.Call(sub_8258C000, uint32_t(sizeof(Replay)));
  if (address != 0) {
    for (Record5232& record : At<Replay>(address)->records) {
      g.Call(sub_82104D90, &record);
    }
  }
  *At<u32_be>(kReplay) = address;
  // As the original: stored even when the allocation failed.
  *At<u32_be>(address + uint32_t(offsetof(Replay, unknown_215372))) = 1;
}

// BootData::CreateObjects: save slot 0, the race data, sub_82102330, the
// defaults, the input recordings, the replay, the data tables' init and
// load.
extern "C" REX_FUNC(sub_821059D8) {
  SBX_DATA_LOAD_FALLBACK(sub_821059D8)
  g.Call(sub_82105938, 0u);
  g.Call(sub_82102390);
  g.Call(sub_82102330);
  g.Call(sub_820CCB60);
  g.Call(sub_82105678);
  g.Call(sub_821056D0);
  g.Call(sub_82116F18);
  g.Call(sub_821173A8);
}

// BootData::Load: once ([0x826B586C]), the boot's steps between two calls
// of sub_822927E0 with a local of the original's 128-byte frame (+80).
extern "C" REX_FUNC(sub_821067A0) {
  SBX_DATA_LOAD_FALLBACK(sub_821067A0)
  u32_be* loaded = At<u32_be>(kBootLoaded);
  if (*loaded != 0) {
    return;
  }
  *loaded = 1;
  guest::StackFrame frame(ctx, 128);
  const uint32_t local = frame.Local(80);
  g.Call(sub_822927E0, local);
  g.Call(sub_820A0A88);
  g.Call(sub_8210AF20);
  g.Call(sub_821059D8);
  g.Call(sub_82245750);
  g.Call(sub_8228E6E0);
  g.Call(sub_820C9700);
  g.Call(sub_820CC048);
  g.Call(sub_822927E0, local);
}
