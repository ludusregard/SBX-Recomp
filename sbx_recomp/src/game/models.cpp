// The models as our own C++ (--native_models, on by default): the models'
// and part models' constructors, destructors and clears, an XPR's, the
// shaders' loads, the shared shaders set, and the scoped lock.

#include "game/model.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_models, true, "SBX",
                    "Our own C++ for the models' constructors, destructors and set-ups, and "
                    "the shaders' loads, instead of the recompiled functions");

namespace {

using game::Model;
using game::PartModel;
using game::TrackSet;
using game::XPR;
using guest::AddressOf;
using guest::At;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kModelVtable = 0x82069AF8;
constexpr uint32_t kPartModelVtable = 0x8200336C;

// The shaders: the global cleared before they load, and the shared ones
// set after.
constexpr uint32_t kShadersLoaded = 0x826B19B8;
constexpr uint32_t kSharedShaders = 0x8296DC14;  // Two words.
constexpr uint32_t kSharedVertexShaders = 0x826AF998;
constexpr uint32_t kSharedPixelShaders = 0x826AD978;

// The shader pairs loaded: each run's first file id and first slot, and
// how many (the file ids two apart).
struct ShaderRun {
  uint32_t first_file;
  uint32_t first_slot;
  uint32_t count;
};
constexpr ShaderRun kShaderRuns[] = {
    {3, 0, 34},
    {22800, 1000, 6},
    {24256, 2000, 56},
};

struct ScopedLock {
  u32_be lock;
};

bool On() { return REXCVAR_GET(native_models); }

void ClearPartModel(PartModel* part) {
  for (u32_be& word : part->vinyl_textures) {
    word = 0;
  }
  for (u32_be& word : part->unknown_452) {
    word = 0;
  }
  part->unknown_432 = 0;
  part->unknown_436 = -1;
  part->unknown_440 = 0;
}

}  // namespace

#define SBX_MODELS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();              \
  if (!On()) {                      \
    __imp__##name(ctx, base);       \
    return;                         \
  }                                 \
  Guest g(ctx, base);

// Shaders::LoadAll: every shader pair loaded, then the shared ones set.
extern "C" REX_FUNC(sub_820A0A88) {
  SBX_MODELS_FALLBACK(sub_820A0A88)
  *At<u32_be>(kShadersLoaded) = 0;
  for (const ShaderRun& run : kShaderRuns) {
    for (uint32_t i = 0; i < run.count; i++) {
      g.Call(sub_820A09D8, run.first_file + 2 * i, run.first_slot + i);  // Shaders::Load
    }
  }
  At<u32_be>(kSharedShaders)[0] = kSharedVertexShaders;
  At<u32_be>(kSharedShaders)[1] = kSharedPixelShaders;
}

// SetSharedShaders: r3 and r4 the shared shaders. Returns r3.
extern "C" REX_FUNC(sub_8256F240) {
  SBX_MODELS_FALLBACK(sub_8256F240)
  At<u32_be>(kSharedShaders)[0] = ctx.r3.u32;
  At<u32_be>(kSharedShaders)[1] = ctx.r4.u32;
}

// PartModel::PartModel: r3 the part. Returns it.
extern "C" REX_FUNC(sub_820CE6E8) {
  SBX_MODELS_FALLBACK(sub_820CE6E8)
  PartModel* part = At<PartModel>(ctx.r3.u32);
  g.Call(sub_8257A1C0, &part->model);  // Model::Model
  part->model.vtable = kPartModelVtable;
  ClearPartModel(part);
  ctx.r3.u64 = AddressOf(part);
}

// PartModel::~PartModel: r3 the part.
extern "C" REX_FUNC(sub_820CE750) {
  SBX_MODELS_FALLBACK(sub_820CE750)
  PartModel* part = At<PartModel>(ctx.r3.u32);
  part->model.vtable = kPartModelVtable;
  ClearPartModel(part);
  g.Call(sub_8257A858, &part->model);  // Model::~Model
}

// PartModel::Clear: r3 the part. Returns it.
extern "C" REX_FUNC(sub_820CE788) {
  SBX_MODELS_FALLBACK(sub_820CE788)
  ClearPartModel(At<PartModel>(ctx.r3.u32));
}

// Model::SetVinylTextures: r3 the part, r4 and r5 the textures. Returns r3.
extern "C" REX_FUNC(sub_820CE7B8) {
  SBX_MODELS_FALLBACK(sub_820CE7B8)
  PartModel* part = At<PartModel>(ctx.r3.u32);
  part->vinyl_textures[0] = ctx.r4.u32;
  part->vinyl_textures[1] = ctx.r5.u32;
}

// ScopedLock::ScopedLock: r3 the lock holder, r4 the lock, taken. Returns
// r3.
extern "C" REX_FUNC(sub_82574180) {
  SBX_MODELS_FALLBACK(sub_82574180)
  ScopedLock* holder = At<ScopedLock>(ctx.r3.u32);
  holder->lock = ctx.r4.u32;
  g.Call(sub_82574178, holder->lock);  // Lock
  ctx.r3.u64 = AddressOf(holder);
}

// ScopedLock::~ScopedLock: r3 the holder; its lock let go.
extern "C" REX_FUNC(sub_825741B8) {
  SBX_MODELS_FALLBACK(sub_825741B8)
  g.Call(sub_8258C4A8, At<ScopedLock>(ctx.r3.u32)->lock);  // Unlock
}

// Model::Model: r3 the model. Returns it.
extern "C" REX_FUNC(sub_8257A1C0) {
  SBX_MODELS_FALLBACK(sub_8257A1C0)
  Model* model = At<Model>(ctx.r3.u32);
  model->vtable = kModelVtable;
  model->xpr = XPR{};
  g.Call(sub_8257A150, model->member_80, -1);
  g.Call(sub_8257EEF0, model->skeleton);  // Skeleton::Skeleton
  for (TrackSet& tracks : model->tracks) {
    g.Call(sub_825866D8, &tracks);  // TrackSet::TrackSet
  }
  for (u32_be& word : model->unknown_16) {
    word = 0;
  }
  model->shader_pairs = 0;
  model->unknown_28 = 0;
  model->meshes = 0;
  model->unknown_48 = 0;
  for (u32_be& word : model->unknown_404) {
    word = 0;
  }
  model->shader_pair_count = 0;
  ctx.r3.u64 = AddressOf(model);
}

// Model::~Model: r3 the model; its members destroyed, the last first.
extern "C" REX_FUNC(sub_8257A858) {
  SBX_MODELS_FALLBACK(sub_8257A858)
  Model* model = At<Model>(ctx.r3.u32);
  model->vtable = kModelVtable;
  g.Call(sub_8257A270, model);  // Model::Release
  g.Call(sub_82587120, &model->tracks[4]);
  g.Call(sub_82587120, &model->tracks[3]);
  g.Call(sub_825872B8, &model->tracks[2]);
  g.Call(sub_825872B8, &model->tracks[1]);
  g.Call(sub_82587120, &model->tracks[0]);
  g.Call(sub_8257F180, model->skeleton);  // Skeleton::~Skeleton
  g.Call(sub_82579948, model->member_80);
  g.Call(sub_8257CA60, &model->xpr);  // XPR::~XPR
}

// XPR::XPR: r3 the XPR, empty. Returns it.
extern "C" REX_FUNC(sub_8257C780) {
  SBX_MODELS_FALLBACK(sub_8257C780)
  *At<XPR>(ctx.r3.u32) = XPR{};
}

// XPR::~XPR: r3 the XPR, released (recompiled).
extern "C" REX_FUNC(sub_8257CA60) {
  SBX_MODELS_FALLBACK(sub_8257CA60)
  g.Call(sub_8257C9C0, ctx.r3.u32);
}
