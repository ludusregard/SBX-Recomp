// The models' animation pieces as our own C++ (--native_model_animation, on
// by default): the skeleton's constructor and its base's destructor, and
// the track sets', records' and cursors' clears.

#include "game/model.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_model_animation, true, "SBX",
                    "Our own C++ for the skeletons' set-up and the animation tracks' clears "
                    "instead of the recompiled functions");

namespace {

using guest::At;
using guest::f32_be;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kSkeletonVtable = 0x82069C30;
constexpr uint32_t kSkeletonBaseVtable = 0x82069C40;

struct Skeleton {
  u32_be vtable;
  uint8_t unknown_4[172];
  u32_be unknown_176;
};

// A skeleton's cursor has a track set's layout (game/model.h).
using game::TrackSet;

struct TrackRecord {
  f32_be time;
  u32_be unknown_4[2];
};

void ClearTrackSet(TrackSet* set) {
  set->time = 0.0f;
  for (u32_be& word : set->unknown_0) {
    word = 0;
  }
}

bool On() { return REXCVAR_GET(native_model_animation); }

}  // namespace

#define SBX_ANIMATION_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                 \
  if (!On()) {                         \
    __imp__##name(ctx, base);          \
    return;                            \
  }                                    \
  Guest g(ctx, base);

// Skeleton::Skeleton: r3 the skeleton. Returns it.
extern "C" REX_FUNC(sub_8257EEF0) {
  SBX_ANIMATION_FALLBACK(sub_8257EEF0)
  Skeleton* skeleton = At<Skeleton>(ctx.r3.u32);
  g.Call(sub_8257F918, skeleton);  // SkeletonBase::SkeletonBase
  skeleton->unknown_176 = 0;
  skeleton->vtable = kSkeletonVtable;
  ctx.r3.u64 = guest::AddressOf(skeleton);
}

// SkeletonBase::~SkeletonBase: r3 the skeleton.
extern "C" REX_FUNC(sub_82580088) {
  SBX_ANIMATION_FALLBACK(sub_82580088)
  Skeleton* skeleton = At<Skeleton>(ctx.r3.u32);
  skeleton->vtable = kSkeletonBaseVtable;
  g.Call(sub_8257FA00, skeleton);
}

// TrackSet::TrackSet and SkeletonCursor::Clear: r3 the set or cursor.
// Return r3.
extern "C" REX_FUNC(sub_825866D8) {
  SBX_ANIMATION_FALLBACK(sub_825866D8)
  TrackSet* set = At<TrackSet>(ctx.r3.u32);
  ClearTrackSet(set);
  set->unknown_16 = 0;
}

extern "C" REX_FUNC(sub_825877D8) {
  SBX_ANIMATION_FALLBACK(sub_825877D8)
  TrackSet* cursor = At<TrackSet>(ctx.r3.u32);
  ClearTrackSet(cursor);
  cursor->unknown_16 = 0;
}

// RecordClear: r3 a track record. Returns it.
extern "C" REX_FUNC(sub_82586FC0) {
  SBX_ANIMATION_FALLBACK(sub_82586FC0)
  TrackRecord* record = At<TrackRecord>(ctx.r3.u32);
  record->time = 0.0f;
  record->unknown_4[0] = 0;
  record->unknown_4[1] = 0;
}

// TrackSet::Clear: r3 the set. Returns it.
extern "C" REX_FUNC(sub_82587638) {
  SBX_ANIMATION_FALLBACK(sub_82587638)
  ClearTrackSet(At<TrackSet>(ctx.r3.u32));
}
