// The matrix math as our own C++ (--native_matrix_math, on by default): the
// small matrix and vector helpers (copies, identity, the points' w, an
// object's inverse, a swap). The math kernels themselves stay recompiled.

#include "game/math.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_matrix_math, true, "SBX",
                    "Our own C++ for the matrix math's small helpers instead of the "
                    "recompiled functions");

namespace {

using game::Matrix;
using game::Vector;
using guest::AddressOf;
using guest::At;
using guest::Guest;

// The identity matrix, kept in the game's writable data.
constexpr uint32_t kIdentity = 0x82955CB0;
// The origin point (0, 0, 0, 1), also in the writable data.
constexpr uint32_t kOrigin = 0x82628220;

// An object with a matrix and its inverse.
struct InvertibleObject {
  uint8_t unknown_0[208];
  Matrix matrix;
  uint8_t unknown_272[288];
  Matrix inverse;
};
GUEST_OFFSET(InvertibleObject, inverse, 560);

bool On() { return REXCVAR_GET(native_matrix_math); }

}  // namespace

#define SBX_MATRIX_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();              \
  if (!On()) {                      \
    __imp__##name(ctx, base);       \
    return;                         \
  }                                 \
  Guest g(ctx, base);

// SetWsToOne: r3 the matrix; each row's w 1. Returns it.
extern "C" REX_FUNC(sub_8209B158) {
  SBX_MATRIX_FALLBACK(sub_8209B158)
  Matrix* m = At<Matrix>(ctx.r3.u32);
  m->right.w = 1.0f;
  m->up.w = 1.0f;
  m->forward.w = 1.0f;
  m->position.w = 1.0f;
}

// ObjectInverse: r3 the object; its inverse from its matrix.
extern "C" REX_FUNC(sub_820A9B80) {
  SBX_MATRIX_FALLBACK(sub_820A9B80)
  InvertibleObject* object = At<InvertibleObject>(ctx.r3.u32);
  g.Call(sub_820A9AD8, &object->inverse, &object->matrix);  // Matrix::Inverse
}

// Matrix::Set: r4 copied into r3. Returns r3's position row.
extern "C" REX_FUNC(sub_82290FE0) {
  SBX_MATRIX_FALLBACK(sub_82290FE0)
  Matrix* m = At<Matrix>(ctx.r3.u32);
  *m = *At<Matrix>(ctx.r4.u32);
  ctx.r3.u64 = AddressOf(&m->position);
}

// Matrix::SetIdentity: r3 the matrix.
extern "C" REX_FUNC(sub_82291120) {
  SBX_MATRIX_FALLBACK(sub_82291120)
  g.Call(sub_82290FE0, ctx.r3.u32, kIdentity);  // Matrix::Set
}

// Matrix::FromQuaternionAndRow: r3 the matrix, its rotation from the
// quaternion (recompiled) and no translation. Returns its position row.
extern "C" REX_FUNC(sub_82291218) {
  SBX_MATRIX_FALLBACK(sub_82291218)
  Matrix* m = At<Matrix>(ctx.r3.u32);
  g.Call(sub_82291130, m);  // Matrix::FromQuaternion (r4 as given)
  m->position = *At<Vector>(kOrigin);
  ctx.r3.u64 = AddressOf(&m->position);
}

// Matrix::MultiplyReversed: r3 = r5 x r4.
extern "C" REX_FUNC(sub_822913C8) {
  SBX_MATRIX_FALLBACK(sub_822913C8)
  g.Call(sub_82290FD8, ctx.r3.u32, ctx.r5.u32, ctx.r4.u32);  // Matrix::Multiply
}

// Vec3::SwapXZ: r3 the vector.
extern "C" REX_FUNC(sub_820AB640) {
  SBX_MATRIX_FALLBACK(sub_820AB640)
  Vector* v = At<Vector>(ctx.r3.u32);
  const guest::f32_be x = v->x;
  v->x = v->z;
  v->z = x;
}
