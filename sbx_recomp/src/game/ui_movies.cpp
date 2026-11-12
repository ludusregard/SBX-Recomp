// The UI movies' file lookups as our own C++ (--native_ui_movies, on by
// default): the movie file's tables (its animations, node tracks, keys,
// sub-animations, vertices), the two magic checks, two of the movie's
// per-node lookups, the movie's constructor and destroy, its update and
// the track step. The update and the track step (sub_8211D4B8,
// sub_82585C40) keep subtick's overrides (subtick/per_frame_logic.cpp),
// which call ours (at the end of this file) in place of the recompiled
// functions. The movies' text and drawing stay recompiled.

#include "game/ui_movies.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

#include <climits>
#include <cmath>
#include <cstdint>

REXCVAR_DEFINE_BOOL(native_ui_movies, true, "SBX",
                    "Our own C++ for the UI movies' file lookups (animations, tracks, keys, the "
                    "magic checks, two node lookups) instead of the recompiled functions; the "
                    "movies' update, text and drawing stay recompiled");

namespace {

using game::AnimationList;
using game::AnimationSlot;
using game::MovieFile;
using game::MovieNode;
using game::MovieTrack;
using game::NamedAnimation;
using game::PropertyDescriptor;
using game::TrackKey;
using game::TrackNode;
using game::UiMovie;
using guest::At;
using guest::Guest;
using guest::f32_be;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_ui_movies); }

// The read-only floats the originals load.
constexpr float kFrameStep = 2.0f;   // 0x820021A8: the time a step adds.
constexpr float kNearOne = 1.0e-5f;  // 0x82002A20: how near 1 a step key's time counts as there.
constexpr float kCurve = 3.0f;       // 0x820021A4: the curve keys' factor.

// The 8-byte magic at `at`: four characters, a 0, then the version's
// three and a 0.
bool Magic(const uint8_t* at, const char (&magic)[8]) {
  for (int i = 0; i < 8; i++) {
    if (at[i] != uint8_t(magic[i])) {
      return false;
    }
  }
  return true;
}

// A node in the movie's map by its key (sub_8211B4E0, the key in the
// original's 96-byte frame at +80); 0 when there's none.
MovieNode* NodeByKey(Guest& g, UiMovie* movie, uint32_t key) {
  guest::StackFrame frame(g.ctx, 96);
  const uint32_t local = frame.Local(80);
  *At<u32_be>(local) = key;
  const uint32_t entry = g.Call(sub_8211B4E0, movie->node_map, local);
  return At<MovieNode>(*At<u32_be>(entry));
}

// UiMovie::ApplyAnimation (sub_82585FD8): the slot's movie file and
// animation at its time (f1), r6 as given. Returns its r3: whether the
// animation is still going.
uint32_t ApplyAnimation(Guest& g, AnimationSlot* slot, uint32_t r6) {
  g.ctx.f1.f64 = double(float(slot->time));
  g.ctx.r6.u64 = r6;  // The original leaves r5 as it is.
  return g.Call(sub_82585FD8, slot->file, slot->animation);
}

// The movie's nodes laid out and drawn into its lists after the
// animations: sub_825850F8, sub_8211CF58 (r5 = whether this is a draw-only
// pass), sub_825831B8 twice, and the movie's state put back
// (sub_82119D50).
void FinishUpdate(Guest& g, UiMovie* movie, uint32_t draw_only) {
  g.Call(sub_825850F8, movie->member_64, movie->state_16, movie->member_28, movie->member_40, -1);
  g.Call(sub_8211CF58, movie, movie->member_40, draw_only);
  g.Call(sub_825831B8, movie->member_64, 676u);
  g.Call(sub_825831B8, movie->member_64, 148u);
  g.Call(sub_82119D50, movie, movie->state_16, movie->state_20, 0u, 0u);
}

// sub_82119D50 with the movie's state words the other way round (the
// update's first call, and its last after the ended animations).
void SwapState(Guest& g, UiMovie* movie) {
  g.Call(sub_82119D50, movie, movie->state_20, movie->state_16, 0u, 0u);
}

// A list's first link taken out of use and put first on its free links.
void UnlinkFirst(AnimationList& list) {
  const int16_t first = list.head;
  AnimationList::Node* node = list.NodeAt(first);
  list.head = node->next;
  node->next = list.free_head;
  list.free_head = first;
  list.count = int16_t(list.count - 1);
}

// The list as a frame's local list starts: every link free.
void ResetEnded(AnimationList& list) {
  for (int k = 0; k < 32; k++) {
    list.nodes[k].slot = int16_t(k);
    list.nodes[k].next = int16_t(k + 1);
  }
  list.free_head = 0;
  list.count = 0;
  list.nodes[31].next = -1;
  list.head = -1;
  list.unknown_head = -1;
  list.unknown_free = -1;
}

// A single-precision result of a double (the originals' fmuls, fadds,
// fsubs and fmadds, on the caller's f1 as it came).
double Single(double value) { return double(float(value)); }

// Whether a step key's time counts as its end: |t - 1| not above
// kNearOne (so a NaN counts as there, as the original's ble has it).
bool AtStepEnd(double t) {
  const double one = 1.0;
  const double distance = t > one ? Single(t - one) : Single(one - t);
  return !(distance > double(kNearOne));
}

float KeyFloat(const TrackKey* key) { return std::bit_cast<float>(uint32_t(key->value)); }

}  // namespace

#define SBX_UI_MOVIES_FALLBACK(name) \
  REX_FUNC_PROLOGUE();               \
  if (!On()) {                       \
    __imp__##name(ctx, base);        \
    return;                          \
  }                                  \
  Guest g(ctx, base);

// UiMovie::DestroyNamedAnimations: r3 the movie's named animations, each
// destroyed (sub_820913F0, r4 = 1), last first.
extern "C" REX_FUNC(sub_8211D378) {
  SBX_UI_MOVIES_FALLBACK(sub_8211D378)
  NamedAnimation* named = At<NamedAnimation>(ctx.r3.u32);
  for (int i = UiMovie::kNamedAnimations - 1; i >= 0; i--) {
    g.Call(sub_820913F0, &named[i], 1u, 0u);
  }
}

// UiMovie::UiMovie: r3 the movie, returned. Its members and named
// animations made, its words and node map cleared.
extern "C" REX_FUNC(sub_8211E380) {
  SBX_UI_MOVIES_FALLBACK(sub_8211E380)
  const uint32_t address = ctx.r3.u32;
  UiMovie* movie = At<UiMovie>(address);
  g.Call(sub_8211AFE8, movie->member_28);
  g.Call(sub_8211B1E8, movie->member_40);
  movie->unknown_48 = 0;
  g.Call(sub_82583028, movie->member_64);
  for (NamedAnimation& named : movie->named) {
    g.Call(sub_8211D3D0, &named);
  }
  movie->unknown_37040 = 0;
  for (u32_be& word : movie->node_map) {
    word = 0;
  }
  ctx.r3.u64 = address;
}

// Animation::NodeTrack: the track r4 of an animation, r3 + 16 x (r4 + 1).
extern "C" REX_FUNC(sub_8258BCC8) {
  SBX_UI_MOVIES_FALLBACK(sub_8258BCC8)
  ctx.r3.u64 = ((ctx.r4.u32 + 1) << 4) + ctx.r3.u32;
}

// The key r4 of a track: r3 + 32 x r4 + 16.
extern "C" REX_FUNC(sub_8258BCD8) {
  SBX_UI_MOVIES_FALLBACK(sub_8258BCD8)
  ctx.r3.u64 = (ctx.r4.u32 << 5) + ctx.r3.u32 + 16;
}

// The movie file's tables: r3 the file, r4 the index.
extern "C" REX_FUNC(sub_8258BCE8) {  // NodeTrackList
  SBX_UI_MOVIES_FALLBACK(sub_8258BCE8)
  MovieFile* file = At<MovieFile>(ctx.r3.u32);
  ctx.r3.u64 = file->EntryAfter(file->node_track_list_count, ctx.r4.u32, MovieFile::kListsFirst);
}

extern "C" REX_FUNC(sub_8258BD00) {  // KeyList
  SBX_UI_MOVIES_FALLBACK(sub_8258BD00)
  MovieFile* file = At<MovieFile>(ctx.r3.u32);
  ctx.r3.u64 = file->EntryAfter(file->key_list_count, ctx.r4.u32, MovieFile::kListsFirst);
}

extern "C" REX_FUNC(sub_8258BD18) {  // SubAnimation
  SBX_UI_MOVIES_FALLBACK(sub_8258BD18)
  MovieFile* file = At<MovieFile>(ctx.r3.u32);
  ctx.r3.u64 = file->EntryAfter(file->sub_animation_count, ctx.r4.u32, MovieFile::kListsFirst);
}

extern "C" REX_FUNC(sub_825865D0) {  // FileAnimation
  SBX_UI_MOVIES_FALLBACK(sub_825865D0)
  MovieFile* file = At<MovieFile>(ctx.r3.u32);
  ctx.r3.u64 = file->EntryAfter(file->animation_count, ctx.r4.u32, MovieFile::kAnimationsFirst);
}

// VertexOffset: 16 x (r3 + 2 x (r4 + 4)).
extern "C" REX_FUNC(sub_82582CF8) {
  SBX_UI_MOVIES_FALLBACK(sub_82582CF8)
  ctx.r3.u64 = (((ctx.r4.u32 + 4) << 1) + ctx.r3.u32) << 4;
}

// IsXmd: "XMD\0001\0" at r3.
extern "C" REX_FUNC(sub_82586358) {
  SBX_UI_MOVIES_FALLBACK(sub_82586358)
  ctx.r3.u64 = Magic(At<uint8_t>(ctx.r3.u32), {'X', 'M', 'D', 0, '0', '0', '1', 0}) ? 1 : 0;
}

// IsMovieFile: "X2D\0005\0" at r3 + 8.
extern "C" REX_FUNC(sub_82586538) {
  SBX_UI_MOVIES_FALLBACK(sub_82586538)
  ctx.r3.u64 = Magic(At<uint8_t>(ctx.r3.u32 + 8), {'X', '2', 'D', 0, '0', '0', '5', 0}) ? 1 : 0;
}

// A node's +24 and +4 by its key (r4); 0 when the movie has none.
extern "C" REX_FUNC(sub_8211B988) {
  SBX_UI_MOVIES_FALLBACK(sub_8211B988)
  const MovieNode* node = NodeByKey(g, At<UiMovie>(ctx.r3.u32), ctx.r4.u32);
  ctx.r3.u64 = node != nullptr ? uint32_t(node->unknown_24) : 0;
}

extern "C" REX_FUNC(sub_8211BB88) {
  SBX_UI_MOVIES_FALLBACK(sub_8211BB88)
  const MovieNode* node = NodeByKey(g, At<UiMovie>(ctx.r3.u32), ctx.r4.u32);
  ctx.r3.u64 = node != nullptr ? uint32_t(node->unknown_4) : 0;
}

// UiMovie::Update: r3 the movie, r4 nonzero for a draw-only pass. Returns
// whether the movie is still going (1 on a draw-only pass).
//
// A draw-only pass applies the queued animations (up to the last that
// runs with its next) and the looping ones at their times, and finishes.
// Otherwise each queued animation is applied and its time stepped (unless
// held; backwards when set to, kept within 0 and its length). One that
// has ended (the apply returns 0) and is taken off when it ends moves to a
// list in the original's 1616-byte frame (+80; sub_8211B6A0, then
// sub_8211B250 takes it off the queue); the walk stops at the first that
// doesn't run with its next. When none is still going, the ones walked
// are all moved to the frame's list. The looping animations are applied
// and stepped (wrapping to 0 past their length, to their length below 0).
// After the finish, the frame's list's animations are applied once more,
// in order, with the movie's state swapped back after them.
void hooks::NativeUiMovies_Update(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_8211D4B8(ctx, base);
    return;
  }
  Guest g(ctx, base);
  UiMovie* movie = At<UiMovie>(ctx.r3.u32);
  const bool draw_only = ctx.r4.u32 != 0;
  guest::StackFrame frame(ctx, 1616);
  AnimationList& ended = *At<AnimationList>(frame.Local(80));
  AnimationList& queue = movie->queue;
  AnimationList& looping = movie->looping;

  SwapState(g, movie);
  ResetEnded(ended);

  if (draw_only) {
    const int32_t count = queue.count;
    for (int32_t i = 0; i < count; i++) {
      AnimationSlot* slot = queue.ItemAtPosition(uint32_t(i));
      ApplyAnimation(g, slot, slot->backwards);
      if (slot->with_next == 0) {
        break;
      }
    }
    for (uint32_t i = 0; i < uint32_t(int32_t(looping.count)); i++) {
      ApplyAnimation(g, looping.ItemAtPosition(i), 0);
    }
    FinishUpdate(g, movie, 1);
    ctx.r3.u64 = 1;
    return;
  }

  ctx.fpscr.disableFlushMode();
  const float zero = 0.0f;
  uint32_t going = 0;
  int32_t count = queue.count;
  if (count == 0) {
    going = 1;
  } else {
    uint32_t walked = 0;
    for (int32_t i = 0; i < count; i++) {
      AnimationSlot* slot = queue.ItemAtPosition(uint32_t(i));
      const uint32_t still = ApplyAnimation(g, slot, slot->backwards);
      walked++;
      if (slot->hold == 0) {
        ctx.fpscr.disableFlushMode();
        float time = slot->time;
        time = slot->backwards != 0 ? time - kFrameStep : time + kFrameStep;
        if (time < zero) {
          time = zero;
        } else {
          const float length = slot->animation->length;
          if (time > length) {
            time = length;
          }
        }
        slot->time = time;
      }
      if (still != 0) {
        going = 1;
      }
      if (slot->with_next == 0) {
        break;  // The last of the ones that run together.
      }
      if (still == 0 && slot->remove_at_end != 0) {
        g.Call(sub_8211B6A0, &ended, slot, int32_t(ended.count));
        g.Call(sub_8211B250, &queue, i);
        walked--;
        count--;
        i--;
      }
    }
    // The walked ones are moved only when none is still going.
    if (going == 0 && uint16_t(int16_t(queue.count)) != 0) {
      for (; walked != 0; walked--) {
        g.Call(sub_8211B6A0, &ended, queue.ItemAtPosition(0), int32_t(ended.count));
        UnlinkFirst(queue);
      }
    }
  }

  for (uint32_t i = 0; i < uint32_t(int32_t(looping.count)); i++) {
    AnimationSlot* slot = looping.ItemAtPosition(i);
    ApplyAnimation(g, slot, 0);
    if (slot->hold == 0) {
      ctx.fpscr.disableFlushMode();
      float time = slot->time + kFrameStep;
      if (time < zero) {
        time = slot->animation->length;
      } else if (time > slot->animation->length) {
        time = zero;
      }
      slot->time = time;
    }
  }

  FinishUpdate(g, movie, 0);

  uint32_t left = uint32_t(int32_t(ended.count));
  if (left != 0) {
    for (; left != 0; left--) {
      ApplyAnimation(g, ended.ItemAtPosition(0), 0);
      UnlinkFirst(ended);
    }
    SwapState(g, movie);
    if (uint16_t(int16_t(queue.count)) != 0) {
      going = 1;
    }
  }
  ctx.r3.u64 = going;
}

// UiMovie::ApplyTrack: r3 the node, r4 the track, r5 and r6 the keys the
// time is between (either may be 0), f1 how far between them (0 to 1). The
// property's descriptor (sub_82585560) gives the value's address.
//
// With no first key, a key in the original's 224-byte frame (+112) stands
// for it, holding the value now when the property is set; with no second,
// the first is used. The blended value is stored when the property is set,
// else added to the value there.
void hooks::NativeUiMovies_ApplyTrack(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_82585C40(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const uint32_t node_address = ctx.r3.u32;
  const TrackNode* node = At<TrackNode>(node_address);
  const MovieTrack* track = At<MovieTrack>(ctx.r4.u32);
  const TrackKey* from = At<TrackKey>(ctx.r5.u32);
  const TrackKey* to = At<TrackKey>(ctx.r6.u32);
  const double t = ctx.f1.f64;
  guest::StackFrame frame(ctx, 224);
  TrackKey* local = At<TrackKey>(frame.Local(112));

  const PropertyDescriptor* descriptor =
      At<PropertyDescriptor>(g.Call(sub_82585560, track->property));
  ctx.fpscr.disableFlushMode();
  const float zero = 0.0f;
  uint32_t target = descriptor->offset + node_address;
  const bool in_array = descriptor->in_array != 0;
  local->unknown_4[0] = 0;  // As the original's float store of 0 leaves it.
  local->unknown_4[1] = 0;
  local->unknown_4[2] = 0;
  local->unknown_4[3] = 0;
  local->curve_12 = zero;
  local->curve_16 = zero;
  local->blend = 0;
  local->value = 0;
  if (in_array) {
    target += g.Call(sub_82582CF8, node->array, track->index);  // VertexOffset
  }
  if (from == nullptr) {
    from = local;
    if (descriptor->set != 0 && descriptor->type < 3) {
      // The value now, a float's or a word's bits alike.
      local->value = uint32_t(*At<u32_be>(target));
    }
  }
  if (to == nullptr) {
    to = from;
  }
  const uint32_t type = descriptor->type;
  if (type >= 3) {
    return;
  }
  const uint32_t blend = to->blend;
  const double one = 1.0;
  ctx.fpscr.disableFlushMode();

  if (type == PropertyDescriptor::kFloat) {
    double value = double(zero);
    if (blend < 1) {
      value = double(KeyFloat(AtStepEnd(t) ? to : from));
    } else if (blend == 1) {
      const double scaled = Single(double(KeyFloat(to)) * t);
      value = Single(std::fma(Single(one - t), double(KeyFloat(from)), scaled));
    } else if (blend == 2) {
      ctx.f1.f64 = t;
      g.Call(sub_82585228, from, to);
      value = ctx.f1.f64;
    } else if (blend == 3) {
      // The original's register steps, kept in their order.
      double f11 = Single(double(float(to->curve_16)) * t);
      double f10 = double(float(to->curve_12));
      const double f9 = double(KeyFloat(to)), f8 = double(KeyFloat(from));
      double f13 = one;
      double f0 = Single(f13 - t);
      double f12 = double(kCurve);
      f11 = Single(f11 * f12);
      f10 = Single(f10 * f0);
      f11 = Single(f11 * f0);
      f12 = Single(std::fma(f10, f12, t));
      f0 = Single(f11 * f0);
      f12 = Single(f12 * t);
      f0 = Single(std::fma(f12, t, f0));
      f13 = Single(f13 - f0);
      f0 = Single(f9 * f0);
      value = Single(std::fma(f13, f8, f0));
    }
    f32_be* out = At<f32_be>(target);
    const double before = descriptor->set != 0 ? double(zero) : double(float(*out));
    *out = float(before + value);
    return;
  }

  // A word: type 1 signed, type 2 unsigned.
  uint32_t value = 0;
  if (blend < 1) {
    value = (AtStepEnd(t) ? to : from)->value;
  } else if (blend == 1) {
    const uint32_t to_word = to->value, from_word = from->value;
    const bool is_signed = type == PropertyDescriptor::kSigned;
    const double to_value =
        is_signed ? double(float(double(int32_t(to_word)))) : double(float(double(to_word)));
    const double from_value =
        is_signed ? double(float(double(int32_t(from_word)))) : double(float(double(from_word)));
    const double scaled = Single(to_value * t);
    const double blended = Single(std::fma(from_value, Single(one - t), scaled));
    if (is_signed) {
      // fctiwz: toward zero, held at the largest word; a NaN gives the
      // smallest.
      value = std::isnan(blended)          ? 0x80000000u
              : blended >= double(INT_MAX) ? uint32_t(INT_MAX)
              : blended <= double(INT_MIN) ? 0x80000000u
                                           : uint32_t(int32_t(blended));
    } else {
      // fctidz, its low word kept.
      constexpr double kTwoTo63 = 9223372036854775808.0;
      const int64_t wide = std::isnan(blended)                             ? INT64_MIN
                           : blended > kTwoTo63                            ? LLONG_MAX
                           : (blended >= kTwoTo63 || blended < -kTwoTo63) ? INT64_MIN
                                                                           : int64_t(blended);
      value = uint32_t(wide);
    }
  } else if (blend == 2) {
    ctx.f1.f64 = t;
    value = g.Call(type == PropertyDescriptor::kSigned ? sub_82585310 : sub_82585438, from, to);
  } else if (blend == 3) {
    ctx.f1.f64 = t;
    value = g.Call(type == PropertyDescriptor::kSigned ? sub_82585118 : sub_825851A0, from, to);
  }
  u32_be* out = At<u32_be>(target);
  const uint32_t before = descriptor->set != 0 ? 0 : uint32_t(*out);
  *out = before + value;
}
