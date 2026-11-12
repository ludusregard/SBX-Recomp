// The course scenery's objects as our own C++ (--native_course_scenery, on
// by default): the scenery pools' objects (their flags, keys, data and
// constructor), the pools' lookups and loads, the scenery's physical
// allocations and their count, a config's model asset, the course set's
// resets, and the texture animations' step (every frame).
//
// TextureAnimations::Advance (sub_820F1038) keeps subtick's override,
// which calls ours below in place of the recompiled function; the
// scenery's lists, parse, loads and draw stay recompiled.

#include "game/course_scenery.h"
#include "hooks/native_calls.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_course_scenery, true, "SBX",
                    "Our own C++ for the course scenery's pool objects and lookups, its "
                    "physical allocations, the course set's resets and the texture animations' "
                    "step instead of the recompiled functions; the scenery's loads and draw "
                    "stay recompiled");

namespace {

using game::Animation;
using game::AnimationSet;
using game::AreaLights;
using game::AreaModel;
using game::CourseModelList;
using game::CourseParts;
using game::CourseSet;
using game::ModelAssets;
using game::ObjectPools;
using game::PoolData;
using game::PoolEntry;
using game::PoolObject;
using game::Scenery;
using game::SceneryConfig;
using game::SceneryList;
using game::XPR;
using guest::At;
using guest::Guest;
using guest::u32_be;

bool On() { return REXCVAR_GET(native_course_scenery); }

constexpr uint32_t kPoolObjectVtable = 0x82006820;
// The scenery's physical memory in use, and a word Scenery::ClearGlobal
// clears (writable).
constexpr uint32_t kPhysicalBytes = 0x826B51F8;
constexpr uint32_t kSceneryGlobal = 0x826B51F4;
constexpr uint32_t kPageReadWrite = 4;

// A pool's object `index`.
uint32_t PoolObjectAddress(ObjectPools* pools, uint32_t pool, uint32_t index) {
  const PoolEntry* entry = pools->Pool(pool);
  return (entry->unknown_8 + entry->unknown_4) * index + entry->objects;
}

// Animation::Advance (sub_82145350): the frame back to 0 (and the key) once
// past the last; the key moved on when there's a next one and the frame
// has reached its first frame; then the frame counted on.
void AdvanceAnimation(Animation* a) {
  if (a->frame > a->last_frame) {
    a->frame = 0;
    a->key = 0;
  }
  const int32_t next = a->key + 1;
  if (next < a->key_count && !(a->frame < *a->KeyFrame(next))) {
    a->key = next;
  }
  a->frame += 1;
}

// A pool object's flag test: all of `bits` set.
void FlagsSet(PPCContext& ctx, uint32_t bits) {
  const uint32_t flags = At<PoolObject>(ctx.r3.u32)->flags;
  ctx.r3.u64 = (flags & bits) == bits ? 1 : 0;
}

// A pool object's visible or expiring bit: r4 0 clears it; else, once
// loaded, the two become this one.
void SetFlag(PPCContext& ctx, uint32_t bit) {
  PoolObject* object = At<PoolObject>(ctx.r3.u32);
  uint32_t flags = object->flags;
  if (int32_t(ctx.r4.u32) == 0) {
    flags &= ~bit;
  } else {
    if ((flags & PoolObject::kLoaded) != PoolObject::kLoaded) {
      return;
    }
    flags = (flags & ~uint32_t(PoolObject::kVisible | PoolObject::kExpiring)) | bit;
  }
  object->flags = flags;
}

}  // namespace

#define SBX_COURSE_SCENERY_FALLBACK(name) \
  REX_FUNC_PROLOGUE();                    \
  if (!On()) {                            \
    __imp__##name(ctx, base);             \
    return;                               \
  }                                       \
  Guest g(ctx, base);

// PoolObject::Reset: r3 the object, returned. No flags or key.
extern "C" REX_FUNC(sub_820FB8B8) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB8B8)
  PoolObject* object = At<PoolObject>(ctx.r3.u32);
  object->flags = 0;
  object->key = -1;
  object->data.used = 0;
}

// ModelList::Init: r3 the list, returned. Its fixed list reset, its word
// cleared.
extern "C" REX_FUNC(sub_820F18B0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F18B0)
  CourseModelList* list = At<CourseModelList>(ctx.r3.u32);
  list->list.Reset();
  list->unknown_0 = 0;
}

// Scenery::ClearGlobal: [0x826B51F4] cleared; r3 returned.
extern "C" REX_FUNC(sub_820F1310) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F1310)
  *At<u32_be>(kSceneryGlobal) = 0;
}

// CourseSet::InitParts: r3 the parts, returned. The XPR cleared, the model
// made (Model::Model), the six lists initialized as ModelList::Init does,
// the last words cleared.
extern "C" REX_FUNC(sub_820F20C0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F20C0)
  const uint32_t address = ctx.r3.u32;
  CourseParts* parts = At<CourseParts>(address);
  for (u32_be& word : parts->xpr.words) {
    word = 0;
  }
  g.Call(sub_8257A1C0, &parts->model);
  auto init = [](CourseModelList& list) {
    list.list.Reset();
    list.unknown_0 = 0;
  };
  init(parts->list_464);
  init(parts->list_1000);
  for (CourseModelList& list : parts->lists_1532) {
    init(list);
  }
  for (u32_be& word : parts->unknown_3724) {
    word = 0;
  }
  ctx.r3.u64 = address;
}

// CourseSet::Reset: r3 the set, returned. Its XPR reset (sub_8257C9C0),
// its model released, its lists reset (sub_82146848), then its pool
// object's flags, key and data.
extern "C" REX_FUNC(sub_820F0FC8) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F0FC8)
  const uint32_t address = ctx.r3.u32;
  CourseSet* set = At<CourseSet>(address);
  g.Call(sub_8257C9C0, &set->parts.xpr);
  g.Call(sub_8257A270, &set->parts.model);  // Model::Release
  g.Call(sub_82146848, &set->parts.list_464);
  g.Call(sub_82146848, &set->parts.list_1000);
  for (CourseModelList& list : set->parts.lists_1532) {
    g.Call(sub_82146848, &list);
  }
  set->object.flags = 0;
  set->object.key = -1;
  set->object.data.used = 0;
  ctx.r3.u64 = address;
}

// PoolObject::AllocData: r3 the object's data, r4 bytes (rounded up to
// 64) from it; returns where they start.
extern "C" REX_FUNC(sub_820FB798) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB798)
  PoolData* data = At<PoolData>(ctx.r3.u32);
  const uint32_t used = data->used;
  ctx.r3.u64 = used + data->start;
  data->used = ((ctx.r4.u32 + 63) & ~63u) + used;
}

// PoolObject::IsLoaded, IsVisible, IsExpiring.
extern "C" REX_FUNC(sub_820FB7D0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB7D0)
  FlagsSet(ctx, PoolObject::kLoaded);
}

extern "C" REX_FUNC(sub_820FB7E8) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB7E8)
  FlagsSet(ctx, PoolObject::kLoaded | PoolObject::kVisible);
}

extern "C" REX_FUNC(sub_820FB800) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB800)
  FlagsSet(ctx, PoolObject::kLoaded | PoolObject::kExpiring);
}

// PoolObject::SetVisible and SetExpiring.
extern "C" REX_FUNC(sub_820FB818) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB818)
  SetFlag(ctx, PoolObject::kVisible);
}

extern "C" REX_FUNC(sub_820FB848) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB848)
  SetFlag(ctx, PoolObject::kExpiring);
}

// PoolObject::ClearLoadFlags.
extern "C" REX_FUNC(sub_820FB878) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB878)
  At<PoolObject>(ctx.r3.u32)->flags &= ~uint32_t(PoolObject::kLoadFlags);
}

// PoolObject::MarkLoaded.
extern "C" REX_FUNC(sub_820FB888) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB888)
  PoolObject* object = At<PoolObject>(ctx.r3.u32);
  object->flags = (object->flags & ~uint32_t(PoolObject::kLoading)) | PoolObject::kDataLoaded;
}

// The keys (each class's vtable slot 0): the key's size into [r4], its
// address in r3. PoolObject::Key (shared): none.
extern "C" REX_FUNC(sub_824A7668) {
  SBX_COURSE_SCENERY_FALLBACK(sub_824A7668)
  *At<u32_be>(ctx.r4.u32) = 0;
  ctx.r3.u64 = 0;
}

// AreaLights::Key: its 4-byte key.
extern "C" REX_FUNC(sub_820F1AB0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F1AB0)
  *At<u32_be>(ctx.r4.u32) = uint32_t(sizeof(u32_be));
  ctx.r3.u64 = ctx.r3.u32 + uint32_t(offsetof(AreaLights, key));
}

// AreaModel::Key: its 4-byte key.
extern "C" REX_FUNC(sub_820F1B68) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F1B68)
  *At<u32_be>(ctx.r4.u32) = uint32_t(sizeof(u32_be));
  ctx.r3.u64 = ctx.r3.u32 + uint32_t(offsetof(AreaModel, key));
}

// PoolObject::PoolObject: r4 its data's offset from r3, r5 its size, r6 the
// pool.
extern "C" REX_FUNC(sub_820FB9F0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB9F0)
  const uint32_t address = ctx.r3.u32;
  const uint32_t start = ctx.r4.u32 + address, size = ctx.r5.u32;
  PoolObject* object = At<PoolObject>(address);
  object->vtable = kPoolObjectVtable;
  object->data.size = size;
  object->data.start = start;
  object->unknown_28 = 0;
  object->pool = ctx.r6.u32;
  object->data_end = start + size;
  object->flags = 0;
  object->key = -1;
  object->data.used = 0;
}

// PoolObject::SetKey: r4 the key's source, copied (memcpy) to where the
// object's Key slot says, for the size it gives (in the original's
// 112-byte frame at +80).
extern "C" REX_FUNC(sub_820FB8D0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB8D0)
  PoolObject* object = At<PoolObject>(ctx.r3.u32);
  const uint32_t source = ctx.r4.u32;
  guest::StackFrame frame(ctx, 112);
  const uint32_t size_at = frame.Local(80);
  *At<u32_be>(size_at) = 0;
  const uint32_t key = g.CallVirtual(object, 0, size_at);
  const uint32_t size = *At<u32_be>(size_at);
  if (int32_t(size) != 0 && key != 0) {
    g.Call(sub_8258CB90, key, source, size);  // memcpy
  }
}

// ObjectPools::Pool: r4's entry.
extern "C" REX_FUNC(sub_820FB9E0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FB9E0)
  ctx.r3.u64 = guest::AddressOf(At<ObjectPools>(ctx.r3.u32)->Pool(ctx.r4.u32));
}

// ObjectPools::Object: pool r4's object r5.
extern "C" REX_FUNC(sub_820FBFB8) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FBFB8)
  ctx.r3.u64 = PoolObjectAddress(At<ObjectPools>(ctx.r3.u32), ctx.r4.u32, ctx.r5.u32);
}

// ObjectPools::Load: the object, its Load slot (1) when there is one;
// returns it.
extern "C" REX_FUNC(sub_820FBEA0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820FBEA0)
  const uint32_t object =
      PoolObjectAddress(At<ObjectPools>(ctx.r3.u32), ctx.r4.u32, ctx.r5.u32);
  if (object != 0) {
    g.CallVirtual(At<PoolObject>(object), 1);
  }
  ctx.r3.u64 = object;
}

// SceneryConfig::ModelAsset: r4 the model, r5 the variant: the model's
// r5-th asset, or -1 past its last (or for no such model).
extern "C" REX_FUNC(sub_820F13C0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F13C0)
  SceneryConfig* config = At<Scenery>(ctx.r3.u32)->config.get();
  const uint32_t model = ctx.r4.u32, variant = ctx.r5.u32;
  ctx.r3.u64 = 0xFFFFFFFFFFFFFFFFull;
  if (model >= config->model_count) {
    return;
  }
  const ModelAssets* assets = config->Model(model);
  const uint32_t first = assets->first, last = assets->last;
  if (first <= last && variant <= last - first) {
    ctx.r3.u64 = first + variant;
  }
}

// SceneryList::Clear: r4's two words cleared.
extern "C" REX_FUNC(sub_820F1418) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F1418)
  SceneryList* list = At<SceneryList>(ctx.r4.u32);
  list->unknown_4[0] = 0;
  list->unknown_4[1] = 0;
}

// Scenery::AllocPhysical: r3 bytes of physical memory (sub_82293F10: -1,
// 0, PAGE_READWRITE), its size (sub_82294008) counted in.
extern "C" REX_FUNC(sub_820F1320) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F1320)
  const uint32_t block = g.Call(sub_82293F10, ctx.r3.u32, -1, 0u, kPageReadWrite);
  const uint32_t size = g.Call(sub_82294008, block);
  *At<u32_be>(kPhysicalBytes) += size;
  ctx.r3.u64 = block;
}

// Scenery::FreePhysical: r3's size counted out, then freed (sub_82294010).
extern "C" REX_FUNC(sub_820F1378) {
  SBX_COURSE_SCENERY_FALLBACK(sub_820F1378)
  const uint32_t block = ctx.r3.u32;
  const uint32_t size = g.Call(sub_82294008, block);
  *At<u32_be>(kPhysicalBytes) -= size;
  g.Call(sub_82294010, block);
}

// Animation::Advance.
extern "C" REX_FUNC(sub_82145350) {
  SBX_COURSE_SCENERY_FALLBACK(sub_82145350)
  AdvanceAnimation(At<Animation>(ctx.r3.u32));
}

// AnimationSet::Advance: each animation one step on.
extern "C" REX_FUNC(sub_821453B0) {
  SBX_COURSE_SCENERY_FALLBACK(sub_821453B0)
  AnimationSet* set = At<AnimationSet>(ctx.r3.u32);
  for (int32_t i = 0; i < int32_t(uint32_t(set->count)); i++) {
    AdvanceAnimation(set->Entry(uint32_t(i)));
  }
}

// TextureAnimations::Advance: r3 the course set; its animation set
// advanced (AnimationSet::Advance, sub_821453B0, a tail call) when it has
// one. Called from subtick's override (subtick/per_frame_logic.cpp), which
// skips it on render-only passes.
void hooks::NativeCourseScenery_AdvanceTextureAnimations(PPCContext& ctx, uint8_t* base) {
  if (!On()) {
    __imp__sub_820F1038(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const uint32_t animations = At<CourseSet>(ctx.r3.u32)->parts.animations.address;
  ctx.r3.u64 = animations;
  if (animations == 0) {
    return;
  }
  g.Call(sub_821453B0, animations);
}
