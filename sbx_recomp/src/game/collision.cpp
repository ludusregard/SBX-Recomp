// The collision shapes as our own C++ (--native_collision, on by default):
// the shapes' and queries' constructors, the box shape's getters and
// tables, the collision shape's bounds (every tick, for each pair's broad
// test), its previous and current placements, a contact's set and flip,
// and the shape's translate (game/collision.h).
//
// The vector library (copy, add, dot, scale), the box-box tests, the
// impacts and the pairs' response and solve stay recompiled.

#include "game/collision.h"
#include "game/physics.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_collision, true, "SBX",
                    "Our own C++ for the collision shapes (their constructors, getters, "
                    "bounds, placements, contacts) instead of the recompiled functions; the "
                    "box-box tests, impacts and pair solver stay recompiled");

namespace {

using game::BoxFace;
using game::BoxShape;
using game::CollisionShape;
using game::Contact;
using game::Query;
using game::Shape;
using game::Vector;
using guest::AddressOf;
using guest::At;
using guest::Guest;
using guest::u32_be;

constexpr uint32_t kShapeVtable = 0x82002A88;
constexpr uint32_t kBoxShapeVtable = 0x82002B10;

// A box's 12 edges as pairs of corners, and for each axis one of the two
// corners BoxShape::LowerCorner picks from (the other is it with bit 1
// flipped).
constexpr uint32_t kEdgeStarts = 0x825F0D04;
constexpr uint32_t kEdgeEnds = 0x825F0D34;
constexpr uint32_t kAxisCorners = 0x82002AEC;

bool On() { return REXCVAR_GET(native_collision); }

// The vector library, recompiled.
void VectorCopy(Guest& g, Vector* to, const Vector* from) { g.Call(sub_82291B20, to, from); }
void VectorAdd(Guest& g, Vector* to, const Vector* v) { g.Call(sub_82291DF8, to, v); }
double Dot(Guest& g, const Vector* a, const Vector* b) {
  g.Call(sub_82292048, a, b);
  return g.ctx.f1.f64;
}
void VectorScale(Guest& g, Vector* v, double by) {
  g.ctx.f1.f64 = by;
  g.Call(sub_82291D70, v);
}

void ClearW(Vector& v) { v.w = 0.0f; }

void ConstructShape(Shape* shape) {
  shape->vtable = kShapeVtable;
  shape->center = *At<Vector>(game::kZeroVector);
  shape->radius = 0.0f;
  shape->unknown_16 = 0.5f;
  shape->unknown_20 = 0.5f;
  shape->unknown_24 = 0.25f;
  shape->unknown_28 = 0.05f;
  // No rotation.
  shape->axes[0] = Vector{1.0f, 0.0f, 0.0f, 0.0f};
  shape->axes[1] = Vector{0.0f, 1.0f, 0.0f, 0.0f};
  shape->axes[2] = Vector{0.0f, 0.0f, 1.0f, 0.0f};
  shape->unknown_112 = 0;
}

void ConstructQuery(Query* query) {
  for (Vector& v : query->unknown_32) {
    ClearW(v);
  }
  ClearW(query->unknown_96);
  for (int i = 0; i < 4; i++) {
    for (auto& point : query->sets[i].points) {
      ClearW(point.point);
    }
    query->set_pointers[i] = &query->sets[i];
  }
  query->shape_a = 0;
  query->shape_b = 0;
}

// The shape brought up to date for this tick (its virtual slot 4).
void Refresh(Guest& g, CollisionShape* shape) { g.CallVirtual(shape->shape.get(), 4); }

// The box around the previous and current centers, grown by the shape's
// radius.
void UpdateBounds(CollisionShape* shape) {
  const Vector& previous = shape->previous->center;
  const Vector& current = shape->current->center;
  const float radius = shape->shape->radius;
  auto axis = [radius](float a, float b, guest::f32_be& low, guest::f32_be& high) {
    if (a < b) {
      low = a - radius;
      high = b + radius;
    } else {
      low = b - radius;
      high = a + radius;
    }
  };
  axis(previous.x, current.x, shape->bounds_low.x, shape->bounds_high.x);
  axis(previous.y, current.y, shape->bounds_low.y, shape->bounds_high.y);
  axis(previous.z, current.z, shape->bounds_low.z, shape->bounds_high.z);
}

}  // namespace

#define SBX_COLLISION_FALLBACK(name)   \
  REX_FUNC_PROLOGUE();                 \
  if (!On()) {                         \
    __imp__##name(ctx, base);          \
    return;                            \
  }                                    \
  Guest g(ctx, base);

// Query::Query: r3 the query. Returns it.
extern "C" REX_FUNC(sub_820BD000) {
  SBX_COLLISION_FALLBACK(sub_820BD000)
  ConstructQuery(At<Query>(ctx.r3.u32));
}

// Shape::Shape: r3 the shape. Returns it.
extern "C" REX_FUNC(sub_820BAE70) {
  SBX_COLLISION_FALLBACK(sub_820BAE70)
  ConstructShape(At<Shape>(ctx.r3.u32));
}

// BoxShape::BoxShape: r3 the box, r4 and r5 passed on to
// BoxShape::SetCorners. Returns the box.
extern "C" REX_FUNC(sub_820BAFD0) {
  SBX_COLLISION_FALLBACK(sub_820BAFD0)
  BoxShape* box = At<BoxShape>(ctx.r3.u32);
  const uint32_t r4 = ctx.r4.u32, r5 = ctx.r5.u32;
  g.Call(sub_820BAE70, box);  // Shape::Shape
  box->shape.vtable = kBoxShapeVtable;
  for (Vector& v : box->unknown_128) {
    ClearW(v);
  }
  for (Vector& corner : box->corners) {
    ClearW(corner);
  }
  for (BoxFace& face : box->faces) {
    ClearW(face.direction);
  }
  g.Call(sub_820B9840, box, r4, r5);  // BoxShape::SetCorners
  ctx.r3.u64 = AddressOf(box);
}

// The box shape's counts (shared by the linker with other classes' getters
// of the same constant).
#define SBX_CONSTANT_GETTER(address, value) \
  extern "C" REX_FUNC(sub_##address) {      \
    REX_FUNC_PROLOGUE();                    \
    if (!On()) {                            \
      __imp__sub_##address(ctx, base);      \
      return;                               \
    }                                       \
    ctx.r3.u64 = value;                     \
  }

SBX_CONSTANT_GETTER(82468988, BoxShape::kCorners)  // BoxShape::CornerCount
SBX_CONSTANT_GETTER(82469648, BoxShape::kFaces)    // BoxShape::FaceCount
SBX_CONSTANT_GETTER(820B9D58, BoxShape::kEdges)    // BoxShape::EdgeCount

#undef SBX_CONSTANT_GETTER

// BoxShape::Corner: r3 the box, r4 the corner.
extern "C" REX_FUNC(sub_820B9D38) {
  SBX_COLLISION_FALLBACK(sub_820B9D38)
  ctx.r3.u64 = ctx.r3.u32 + offsetof(BoxShape, corners) + ctx.r4.u32 * sizeof(Vector);
}

// BoxShape::Face: r3 the box, r4 the face.
extern "C" REX_FUNC(sub_820B9D48) {
  SBX_COLLISION_FALLBACK(sub_820B9D48)
  ctx.r3.u64 = ctx.r3.u32 + offsetof(BoxShape, faces) + ctx.r4.u32 * sizeof(BoxFace);
}

// An edge's first or second corner: the edge in r4 (the box shape's
// getters) or r3 (BoxEdges').
#define SBX_EDGE_GETTER(address, table, reg)                    \
  extern "C" REX_FUNC(sub_##address) {                          \
    REX_FUNC_PROLOGUE();                                        \
    if (!On()) {                                                \
      __imp__sub_##address(ctx, base);                          \
      return;                                                   \
    }                                                           \
    guest::Guest g(ctx, base);                                  \
    ctx.r3.u64 = *guest::Element<u32_be>(table, ctx.reg.u32);   \
  }

SBX_EDGE_GETTER(820B9DF8, kEdgeStarts, r4)  // BoxShape::EdgeStart
SBX_EDGE_GETTER(820B9E10, kEdgeEnds, r4)    // BoxShape::EdgeEnd
SBX_EDGE_GETTER(820B9E28, kEdgeStarts, r3)  // BoxEdges::Start
SBX_EDGE_GETTER(820B9E40, kEdgeEnds, r3)    // BoxEdges::End

#undef SBX_EDGE_GETTER

// BoxShape::SetCacheWord: r3 the box, r4 the word, r5 its value.
extern "C" REX_FUNC(sub_820BA4C8) {
  SBX_COLLISION_FALLBACK(sub_820BA4C8)
  *guest::Element<u32_be>(ctx.r3.u32 + offsetof(BoxShape, cache), ctx.r4.u32) = ctx.r5.u32;
}

// BoxShape::CacheWord: r3 the box, r4 the word.
extern "C" REX_FUNC(sub_820BA4D8) {
  SBX_COLLISION_FALLBACK(sub_820BA4D8)
  ctx.r3.u64 = *guest::Element<u32_be>(ctx.r3.u32 + offsetof(BoxShape, cache), ctx.r4.u32);
}

// BoxShape::CopyCache: r4's cache into r3's.
extern "C" REX_FUNC(sub_820BA8A8) {
  SBX_COLLISION_FALLBACK(sub_820BA8A8)
  BoxShape* to = At<BoxShape>(ctx.r3.u32);
  const BoxShape* from = At<BoxShape>(ctx.r4.u32);
  for (int i = 0; i < 4; i++) {
    to->cache[i] = from->cache[i];
  }
}

// Shape::Radius: r3 the shape; the radius in f1.
extern "C" REX_FUNC(sub_820BA998) {
  SBX_COLLISION_FALLBACK(sub_820BA998)
  ctx.f1.f64 = double(float(At<Shape>(ctx.r3.u32)->radius));
}

// BoxShape::LowerCorner: r3 the box, r4 the axis: of the axis' two corners,
// the one that's lower (in y).
extern "C" REX_FUNC(sub_820BA1C8) {
  SBX_COLLISION_FALLBACK(sub_820BA1C8)
  const uint32_t box = ctx.r3.u32;
  const uint32_t first = *guest::Element<u32_be>(kAxisCorners, ctx.r4.u32);
  const uint32_t second = first ^ 2;
  const uint32_t corners = box + offsetof(BoxShape, corners);
  const float a = guest::Element<Vector>(corners, first)->y;
  const float b = guest::Element<Vector>(corners, second)->y;
  ctx.r3.u64 = corners + (a < b ? first : second) * sizeof(Vector);
}

// BoxShape::FaceDistance: r3 the box, r4 the face, r5 a point: how far the
// point is outside the face, in f1.
extern "C" REX_FUNC(sub_820BA210) {
  SBX_COLLISION_FALLBACK(sub_820BA210)
  BoxFace* face =
      guest::Element<BoxFace>(ctx.r3.u32 + offsetof(BoxShape, faces), ctx.r4.u32);
  const float along = float(Dot(g, &face->direction, At<Vector>(ctx.r5.u32)));
  ctx.f1.f64 = double(along - face->distance);
}

// BoxShape::Translate: r3 the box, r4 the offset it's moved by.
extern "C" REX_FUNC(sub_820BA150) {
  SBX_COLLISION_FALLBACK(sub_820BA150)
  BoxShape* box = At<BoxShape>(ctx.r3.u32);
  const Vector* offset = At<Vector>(ctx.r4.u32);
  for (Vector& corner : box->corners) {
    VectorAdd(g, &corner, offset);
  }
  for (BoxFace& face : box->faces) {
    const float along = float(Dot(g, &face.direction, offset));
    face.distance = along + face.distance;
  }
  VectorAdd(g, &box->shape.center, offset);
}

// WordAt8 (shared by the linker): r3's word at +8.
extern "C" REX_FUNC(sub_822B8A90) {
  SBX_COLLISION_FALLBACK(sub_822B8A90)
  ctx.r3.u64 = *At<u32_be>(ctx.r3.u32 + 8);
}

// CollisionShape::CurrentCenter and PreviousCenter: r3 the shape.
extern "C" REX_FUNC(sub_820BA978) {
  SBX_COLLISION_FALLBACK(sub_820BA978)
  ctx.r3.u64 = At<CollisionShape>(ctx.r3.u32)->current.address + offsetof(Shape, center);
}

extern "C" REX_FUNC(sub_820BA988) {
  SBX_COLLISION_FALLBACK(sub_820BA988)
  ctx.r3.u64 = At<CollisionShape>(ctx.r3.u32)->previous.address + offsetof(Shape, center);
}

// CollisionShape::Type: the shape's virtual slot 4 (a tail call).
extern "C" REX_FUNC(sub_820BAB08) {
  SBX_COLLISION_FALLBACK(sub_820BAB08)
  Refresh(g, At<CollisionShape>(ctx.r3.u32));
}

// CollisionShape::Previous and Current: the placements, brought up to date
// first.
extern "C" REX_FUNC(sub_820BAC48) {
  SBX_COLLISION_FALLBACK(sub_820BAC48)
  CollisionShape* shape = At<CollisionShape>(ctx.r3.u32);
  Refresh(g, shape);
  ctx.r3.u64 = shape->previous.address;
}

extern "C" REX_FUNC(sub_820BAC88) {
  SBX_COLLISION_FALLBACK(sub_820BAC88)
  CollisionShape* shape = At<CollisionShape>(ctx.r3.u32);
  Refresh(g, shape);
  ctx.r3.u64 = shape->current.address;
}

// CollisionShape::UpdateBounds: r3 the shape.
extern "C" REX_FUNC(sub_820BAA50) {
  SBX_COLLISION_FALLBACK(sub_820BAA50)
  UpdateBounds(At<CollisionShape>(ctx.r3.u32));
}

// CollisionShape::~CollisionShape: r3 the shape; its two placements
// deleted.
extern "C" REX_FUNC(sub_820BA8F8) {
  SBX_COLLISION_FALLBACK(sub_820BA8F8)
  CollisionShape* shape = At<CollisionShape>(ctx.r3.u32);
  for (guest::ptr<Shape>* placement : {&shape->previous, &shape->current}) {
    if (Shape* object = *placement) {
      g.CallVirtual(object, 0, 1u);  // Its deleting destructor.
      *placement = nullptr;
    }
  }
}

// Query::SetShapes (shared by the linker): r3's first two words = r4, r5.
extern "C" REX_FUNC(sub_822DF518) {
  SBX_COLLISION_FALLBACK(sub_822DF518)
  Query* query = At<Query>(ctx.r3.u32);
  query->shape_a = ctx.r4.u32;
  query->shape_b = ctx.r5.u32;
}

// Contact::Set: r3 the contact, r4 the position, r5 the normal, f1 and f2
// its two numbers.
extern "C" REX_FUNC(sub_820BC740) {
  SBX_COLLISION_FALLBACK(sub_820BC740)
  Contact* contact = At<Contact>(ctx.r3.u32);
  const Vector* position = At<Vector>(ctx.r4.u32);
  const Vector* normal = At<Vector>(ctx.r5.u32);
  const double a = ctx.f1.f64, b = ctx.f2.f64;
  VectorCopy(g, &contact->position, position);
  VectorCopy(g, &contact->normal, normal);
  VectorCopy(g, &contact->normal2, normal);
  contact->unknown_48 = float(a);
  contact->unknown_52 = float(b);
}

// Contact::Flip: r3 the contact; its normals turned around.
extern "C" REX_FUNC(sub_820BC690) {
  SBX_COLLISION_FALLBACK(sub_820BC690)
  Contact* contact = At<Contact>(ctx.r3.u32);
  VectorScale(g, &contact->normal, -1.0);
  VectorScale(g, &contact->normal2, -1.0);
}

// BoxBox::FindImpact: r3 the box-box test; its two steps run twice
// (recompiled).
extern "C" REX_FUNC(sub_820BEE70) {
  SBX_COLLISION_FALLBACK(sub_820BEE70)
  const uint32_t test = ctx.r3.u32;
  for (int pass = 0; pass < 2; pass++) {
    g.Call(sub_820BEAD8, test);
    g.Call(sub_820BE2C8, test);
  }
}
