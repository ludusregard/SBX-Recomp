// The game's vector and matrix types, as they sit in guest memory.

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::f32_be;

// A point or direction: four floats, 16 bytes. Points carry w = 1.
struct Vector {
  f32_be x, y, z, w;
};
GUEST_SIZE(Vector, 16);

// A 4x4 matrix of row vectors: three rotation rows, then the position.
struct Matrix {
  Vector right, up, forward, position;
};
GUEST_SIZE(Matrix, 64);

}  // namespace game
