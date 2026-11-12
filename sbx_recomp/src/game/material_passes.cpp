// The part models' materials as our own C++ (--native_material_passes, on
// by default): counts and records, the alpha setters, the paint table's
// lookups and the two block-set tests (game/model.h).
//
// The paints, palette, textures and culling stay recompiled.

#include "game/model.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_material_passes, true, "SBX",
                    "Our own C++ for the part models' materials (counts, records, the alpha "
                    "setters, the paint table's lookups) instead of the recompiled functions");

namespace {

using game::Material;
using game::Model;
using game::Vector;
using guest::At;
using guest::Guest;

constexpr uint32_t kPaintVectors = 0x825F19B0;  // Vectors, 4 columns a row.
constexpr uint32_t kPaintColumns = 4;
constexpr uint32_t kPaintRecords = 0x82003660;
constexpr uint32_t kPasses = 2;

bool On() { return REXCVAR_GET(native_material_passes); }

// PartModel::MaterialCount and MaterialAt for a pass (recompiled).
int32_t MaterialCount(Guest& g, void* model, uint32_t mesh, uint32_t pass) {
  return int32_t(g.Call(sub_8257A3D8, model, mesh, pass));
}
Material* MaterialAt(Guest& g, void* model, uint32_t mesh, uint32_t index, uint32_t pass) {
  return At<Material>(g.Call(sub_8257A420, model, mesh, index, pass));
}

// Sets the alpha of a mesh's materials in both passes; with `only_1002`,
// only of the materials of that kind.
void SetAlpha(Guest& g, void* model, uint32_t mesh, float alpha, bool only_1002) {
  if (model == nullptr) {
    return;
  }
  const int32_t counts[kPasses] = {MaterialCount(g, model, mesh, 0),
                                   MaterialCount(g, model, mesh, 1)};
  for (uint32_t pass = 0; pass < kPasses; pass++) {
    for (int32_t i = 0; i < counts[pass]; i++) {
      Material* material = MaterialAt(g, model, mesh, uint32_t(i), pass);
      if (!only_1002 || material->kind == Material::kKind1002) {
        material->alpha = alpha;
      }
    }
  }
}

}  // namespace

// TableVector: r3 the result, r4 the row, r5 the column of the paint
// vectors.
extern "C" REX_FUNC(sub_820D3218) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3218(ctx, base);
    return;
  }
  Guest g(ctx, base);
  *At<Vector>(ctx.r3.u32) =
      *guest::Element<Vector>(kPaintVectors, ctx.r4.u32 * kPaintColumns + ctx.r5.u32);
}

// IsBlockSetA: 1 for 1, 26, 28, 33 and 36.
extern "C" REX_FUNC(sub_820D3248) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3248(ctx, base);
    return;
  }
  const int32_t v = ctx.r3.s32;
  ctx.r3.u64 = v == 1 || v == 26 || v == 28 || v == 33 || v == 36;
}

// IsBlockSetB: 1 for 10 and 37.
extern "C" REX_FUNC(sub_820D3280) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3280(ctx, base);
    return;
  }
  const int32_t v = ctx.r3.s32;
  ctx.r3.u64 = v == 10 || v == 37;
}

// RecordAt: the paint record r3.
extern "C" REX_FUNC(sub_820D3370) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3370(ctx, base);
    return;
  }
  ctx.r3.u64 = kPaintRecords + ctx.r3.u32 * uint32_t(sizeof(game::PaintRecord));
}

// Model::MaterialTotal: r3 the model, r4 the mesh: how many materials.
extern "C" REX_FUNC(sub_8257A3C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8257A3C0(ctx, base);
    return;
  }
  Guest g(ctx, base);
  ctx.r3.u64 = At<Model>(ctx.r3.u32)->MeshAt(ctx.r4.u32)->materials->count;
}

// Model::MaterialAt: r3 the model, r4 the mesh, r5 the material.
extern "C" REX_FUNC(sub_8257A400) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8257A400(ctx, base);
    return;
  }
  Guest g(ctx, base);
  const uint32_t list = At<Model>(ctx.r3.u32)->MeshAt(ctx.r4.u32)->materials.address;
  ctx.r3.u64 = list + offsetof(game::MaterialList, materials) +
               ctx.r5.u32 * uint32_t(sizeof(Material));
}

// PartModel::SetAlpha: r3 the model, r4 the mesh, f1 the alpha.
extern "C" REX_FUNC(sub_820D1848) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D1848(ctx, base);
    return;
  }
  Guest g(ctx, base);
  SetAlpha(g, At<void>(ctx.r3.u32), ctx.r4.u32, float(ctx.f1.f64), false);
}

// PartModel::SetPair1002Alpha: the same for the materials of kind 1002.
extern "C" REX_FUNC(sub_820D1780) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D1780(ctx, base);
    return;
  }
  Guest g(ctx, base);
  SetAlpha(g, At<void>(ctx.r3.u32), ctx.r4.u32, float(ctx.f1.f64), true);
}
