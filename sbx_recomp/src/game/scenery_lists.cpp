// The scenery's lists as our own C++ (--native_scenery_lists, on by
// default): a scenery model's draw state reset, a model list's point
// buffers, a pass's billboard set-up and the scene lights' projectors.

#include "game/math.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_scenery_lists, true, "SBX",
                    "Our own C++ for the scenery lists' resets and set-ups instead of the "
                    "recompiled functions");

namespace {

using game::Vector;
using guest::At;
using guest::Guest;
using guest::u32_be;

struct SceneryModel {
  uint8_t unknown_0[432];
  u32_be visible;
  u32_be unknown_436[2];
};

// A model list's 20 point buffers, 256 bytes each in one static block.
constexpr uint32_t kPointBuffers = 0x826076B0;
constexpr int kPointBufferCount = 20;
constexpr uint32_t kPointBufferBytes = 256;

struct ModelList {
  uint8_t unknown_0[528];
  u32_be point_buffers[kPointBufferCount];
  u32_be unknown_608[3];
  u32_be point_buffer_count;
};
GUEST_OFFSET(ModelList, point_buffers, 528);
GUEST_OFFSET(ModelList, point_buffer_count, 620);

struct PassDescription {
  u32_be unknown_0;
  u32_be unknown_4;
};

// The render state the scene lights' projector record copies: the eye
// (the one the reflections are worked out against), then the projectors'
// rows and matrices. All in the game's writable data, set every frame.
constexpr uint32_t kEyePosition = 0x8264EA60;     // x, y, z.
constexpr uint32_t kProjectorRows = 0x8296DDE0;   // 6 vectors.
constexpr uint32_t kProjectorMatrices = 0x8296DF40;  // 16 vectors.

struct ProjectorRecord {
  Vector eye;  // w 1.
  Vector projector_rows[6];
  Vector projector_matrices[16];
};
GUEST_SIZE(ProjectorRecord, 368);

bool On() { return REXCVAR_GET(native_scenery_lists); }

}  // namespace

#define SBX_SCENERY_LISTS_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                     \
  if (!On()) {                             \
    __imp__##name(ctx, base);              \
    return;                                \
  }                                        \
  Guest g(ctx, base);

// SceneryModel::ResetDrawState: r3 the model. Returns it.
extern "C" REX_FUNC(sub_82145338) {
  SBX_SCENERY_LISTS_FALLBACK(sub_82145338)
  SceneryModel* model = At<SceneryModel>(ctx.r3.u32);
  model->unknown_436[0] = 0;
  model->visible = 1;
  model->unknown_436[1] = 0;
}

// ModelList::LoadPoints: r3 the list. Its points loaded (recompiled), then
// its point buffers set.
extern "C" REX_FUNC(sub_82146DD0) {
  SBX_SCENERY_LISTS_FALLBACK(sub_82146DD0)
  ModelList* list = At<ModelList>(ctx.r3.u32);
  g.Call(sub_82146CD0, list);
  for (int i = 0; i < kPointBufferCount; i++) {
    list->point_buffers[i] = kPointBuffers + kPointBufferBytes * i;
  }
  for (u32_be& word : list->unknown_608) {
    word = 0;
  }
  list->point_buffer_count = kPointBufferCount;
}

// PassDescription::SetBillboardPass: r3 the pass, r4 and r6 its two words.
// Returns r3.
extern "C" REX_FUNC(sub_82146EF8) {
  SBX_SCENERY_LISTS_FALLBACK(sub_82146EF8)
  PassDescription* pass = At<PassDescription>(ctx.r3.u32);
  pass->unknown_0 = ctx.r4.u32;
  pass->unknown_4 = ctx.r6.u32;
}

// SceneLights::RecordProjectors: r3 the record; the eye and the projectors
// copied in. Returns it.
extern "C" REX_FUNC(sub_8256F6D8) {
  SBX_SCENERY_LISTS_FALLBACK(sub_8256F6D8)
  ProjectorRecord* record = At<ProjectorRecord>(ctx.r3.u32);
  const Vector* eye = At<Vector>(kEyePosition);
  record->eye = Vector{eye->x, eye->y, eye->z, 1.0f};
  const Vector* rows = At<Vector>(kProjectorRows);
  for (int i = 0; i < 6; i++) {
    record->projector_rows[i] = rows[i];
  }
  const Vector* matrices = At<Vector>(kProjectorMatrices);
  for (int i = 0; i < 16; i++) {
    record->projector_matrices[i] = matrices[i];
  }
}
