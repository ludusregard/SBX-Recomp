// The collision shapes: what the physics tests cars and walls with.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"

namespace game {

using guest::f32_be;
using guest::ptr;
using guest::u32_be;

// Any shape: where it is and how big.
struct Shape {
  u32_be vtable;
  uint8_t unknown_4[12];
  f32_be unknown_16;  // 0.5
  f32_be unknown_20;  // 0.5
  f32_be unknown_24;  // 0.25
  f32_be unknown_28;  // 0.05
  f32_be radius;      // Of the sphere around it.
  uint8_t unknown_36[12];
  Vector center;
  Vector axes[3];  // Its orientation.
  u32_be unknown_112;
};
GUEST_SIZE(Shape, 116);
GUEST_OFFSET(Shape, center, 48);

// One side of a box: the plane direction . p = distance.
struct BoxFace {
  f32_be distance;
  uint8_t unknown_4[12];
  Vector direction;
};
GUEST_SIZE(BoxFace, 32);

struct BoxShape {
  static constexpr uint32_t kCorners = 8;
  static constexpr uint32_t kFaces = 6;
  static constexpr uint32_t kEdges = 12;

  Shape shape;  // The class it's built on.
  uint8_t unknown_116[12];
  Vector unknown_128[2];
  u32_be cache[4];  // Kept by the box-box test between ticks.
  Vector corners[kCorners];
  BoxFace faces[kFaces];
};
GUEST_SIZE(BoxShape, 496);
GUEST_OFFSET(BoxShape, cache, 160);
GUEST_OFFSET(BoxShape, corners, 176);
GUEST_OFFSET(BoxShape, faces, 304);

// A body's shape in the collision world: the shape as it was last tick and
// as it is now, and the box around both.
struct CollisionShape {
  ptr<Shape> shape;     // The shape in the body's own space.
  ptr<Shape> previous;  // Placed in the world, last tick and this one.
  ptr<Shape> current;
  uint8_t unknown_12[4];
  Vector bounds_low;
  Vector bounds_high;
};
GUEST_SIZE(CollisionShape, 48);
GUEST_OFFSET(CollisionShape, bounds_low, 16);

// Where two shapes touch.
struct Contact {
  Vector position;
  Vector normal;
  Vector normal2;  // A second copy of the normal.
  f32_be unknown_48;
  f32_be unknown_52;
  uint8_t unknown_56[8];
  f32_be strength;
};
GUEST_OFFSET(Contact, unknown_48, 48);
GUEST_OFFSET(Contact, strength, 64);

// A box-box test's working data: the two shapes and four sets of points.
struct Query {
  struct PointSet {
    uint8_t unknown_0[80];
    struct Point {
      Vector point;
      uint8_t unknown_16[16];
    } points[8];
  };

  u32_be shape_a;
  u32_be shape_b;
  ptr<PointSet> set_pointers[4];  // Each to its set below.
  uint8_t unknown_24[8];
  Vector unknown_32[3];
  uint8_t unknown_80[16];
  Vector unknown_96;
  PointSet sets[4];
};
GUEST_SIZE(Query::PointSet, 336);
GUEST_OFFSET(Query, sets, 112);
GUEST_SIZE(Query, 1456);

}  // namespace game
