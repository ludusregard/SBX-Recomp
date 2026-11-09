// The part models' materials as our own C++ (--native_material_passes, on
// by default): a model's material counts and records, the part models'
// alpha setters (every material, or the ones of kind 1002), the paint
// table's vectors and records, and the two block-set tests.
//
// Written from SBX's generated code. A model's materials, by its mesh
// (+40, 544 bytes each, +396 their list) and pass (0 or 1): the count and
// each material (512 bytes from +16; +12 its alpha, +392 its kind). The
// paints themselves (SetPaint, the palette), the textures, the culling and
// the projector's matrices stay recompiled.

#include "guest.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_material_passes, true, "SBX",
                    "Our own C++ for the part models' materials (counts, records, the alpha "
                    "setters, the paint table's lookups) instead of the recompiled functions; "
                    "the paints and textures stay recompiled");

namespace {

using guest::LoadU32;
using guest::StoreFloat;
using guest::StoreU32;

constexpr uint32_t kPaintVectors = 0x825F19B0;  // 16 bytes each, 4 a row
constexpr uint32_t kPaintRecords = 0x82003660;  // 48 bytes each

// PartModel::MaterialCount (sub_8257A3D8) and MaterialAt (sub_8257A420),
// recompiled: r3 the model, r4 the mesh, r5 the pass (and r5 the index, r6
// the pass for MaterialAt).
int32_t MaterialCount(PPCContext& ctx, uint8_t* base, uint32_t model, uint32_t mesh,
                      uint32_t pass) {
  ctx.r3.u64 = model;
  ctx.r4.u64 = mesh;
  ctx.r5.u64 = pass;
  sub_8257A3D8(ctx, base);
  return int32_t(ctx.r3.u32);
}

uint32_t MaterialAt(PPCContext& ctx, uint8_t* base, uint32_t model, uint32_t mesh,
                    uint32_t index, uint32_t pass) {
  ctx.r3.u64 = model;
  ctx.r4.u64 = mesh;
  ctx.r5.u64 = index;
  ctx.r6.u64 = pass;
  sub_8257A420(ctx, base);
  return ctx.r3.u32;
}

// The alpha (f1) into each of the mesh's materials in both passes, or only
// the ones of kind 1002; nothing for a null model.
void SetAlpha(PPCContext& ctx, uint8_t* base, uint32_t model, uint32_t mesh, double alpha,
              bool only_1002) {
  if (model == 0) {
    return;
  }
  const float value = float(alpha);
  const int32_t counts[2] = {MaterialCount(ctx, base, model, mesh, 0),
                             MaterialCount(ctx, base, model, mesh, 1)};
  for (uint32_t pass = 0; pass < 2; pass++) {
    for (int32_t i = 0; i < counts[pass]; i++) {
      const uint32_t material = MaterialAt(ctx, base, model, mesh, uint32_t(i), pass);
      if (!only_1002 || LoadU32(base, material + 392) == 1002) {
        StoreFloat(base, material + 12, value);
      }
    }
  }
}

bool On() { return REXCVAR_GET(native_material_passes); }

}  // namespace

// TableVector: r3 the result (16 bytes), r4 the row, r5 the column.
extern "C" REX_FUNC(sub_820D3218) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3218(ctx, base);
    return;
  }
  const uint32_t from = kPaintVectors + (((ctx.r4.u32 << 2) + ctx.r5.u32) << 4);
  for (uint32_t i = 0; i < 16; i += 4) {
    StoreU32(base, ctx.r3.u32 + i, LoadU32(base, from + i));
  }
}

// IsBlockSetA: 1 for 1, 26, 28, 33 and 36.
extern "C" REX_FUNC(sub_820D3248) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3248(ctx, base);
    return;
  }
  const int32_t v = int32_t(ctx.r3.u32);
  ctx.r3.u64 = (v == 36 || v == 26 || v == 28 || v == 33 || v == 1) ? 1 : 0;
}

// IsBlockSetB: 1 for 10 and 37.
extern "C" REX_FUNC(sub_820D3280) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3280(ctx, base);
    return;
  }
  const int32_t v = int32_t(ctx.r3.u32);
  ctx.r3.u64 = (v == 10 || v == 37) ? 1 : 0;
}

// RecordAt: the paint record r3.
extern "C" REX_FUNC(sub_820D3370) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D3370(ctx, base);
    return;
  }
  ctx.r3.u64 = ctx.r3.u32 * 48 + kPaintRecords;
}

// Model::MaterialTotal: r4 the mesh: its list's count.
extern "C" REX_FUNC(sub_8257A3C0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8257A3C0(ctx, base);
    return;
  }
  const uint32_t mesh = LoadU32(base, ctx.r3.u32 + 40) + 544 * ctx.r4.u32;
  ctx.r3.u64 = LoadU32(base, LoadU32(base, mesh + 396));
}

// Model::MaterialAt: r4 the mesh, r5 the material: its list + 512 x r5 +
// 16.
extern "C" REX_FUNC(sub_8257A400) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_8257A400(ctx, base);
    return;
  }
  const uint32_t mesh = LoadU32(base, ctx.r3.u32 + 40) + 544 * ctx.r4.u32;
  ctx.r3.u64 = LoadU32(base, mesh + 396) + (ctx.r5.u32 << 9) + 16;
}

// PartModel::SetAlpha: r3 the model, r4 the mesh, f1 the alpha.
extern "C" REX_FUNC(sub_820D1848) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D1848(ctx, base);
    return;
  }
  SetAlpha(ctx, base, ctx.r3.u32, ctx.r4.u32, ctx.f1.f64, false);
}

// PartModel::SetPair1002Alpha: the same for the materials of kind 1002.
extern "C" REX_FUNC(sub_820D1780) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820D1780(ctx, base);
    return;
  }
  SetAlpha(ctx, base, ctx.r3.u32, ctx.r4.u32, ctx.f1.f64, true);
}
