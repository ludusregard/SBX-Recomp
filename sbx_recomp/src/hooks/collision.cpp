// The collision shapes as our own C++ (--native_collision, on by default):
// the shapes' and queries' constructors, the box shape's getters and
// tables, the collision shape's bounds (every tick, for each pair's broad
// test), its previous and current placements, a contact's set and flip,
// and the shape's translate.
//
// Written from SBX's generated code. Float steps are the originals': each
// single-precision add, subtract, multiply or divide is done in float
// (the same result as the generated code's double and rounding), compares
// as they are. The vector library (copy, add, dot, scale), the box-box
// tests, the impacts and the pairs' response and solve stay recompiled.
//
// A box shape: +32 its radius, +48 its center, +176 its 8 corners (16
// bytes each), +304 its 6 faces (32 bytes: the distance, then the
// direction at +16), +160 a 16-byte cache. A collision shape: +0 its shape,
// +4 the previous placement, +8 the current one (the center at +48 of
// each), +16 and +32 its bounds' corners.

#include "guest.h"
#include "hooks/guest_memory.h"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(native_collision, true, "SBX",
                    "Our own C++ for the collision shapes (their constructors, getters, "
                    "bounds, placements, contacts) instead of the recompiled functions; the "
                    "box-box tests, impacts and pair solver stay recompiled");

namespace {

using guest_memory::Fill;
using guest_memory::kAbsByte;
using guest_memory::kAbsHalf;
using guest_memory::kAbsImage;
using guest_memory::kAbsWord;
using guest_memory::kArg;
using guest_memory::kByte;
using guest_memory::kHalf;
using guest_memory::kImage;
using guest_memory::kSelf;
using guest_memory::kWord;

// Query::Query (sub_820BD000).
// 42 stores
constexpr Fill kQuery_Query0[] = {
    {44, kImage, 0x82000A24, 3, 16}, {108, kImage, 0x82000A24, 2, 96}, {236, kImage, 0x82000A24, 7, 32}, {540, kImage, 0x82000A24, 8, 32},
    {876, kImage, 0x82000A24, 8, 32}, {1212, kImage, 0x82000A24, 8, 32}, {0, kWord, 0x0, 2, 4}, {8, kSelf, 112},
    {12, kSelf, 448}, {16, kSelf, 784}, {20, kSelf, 1120},
};

// Shape::Shape (sub_820BAE70).
// 27 stores
constexpr Fill kShape_Shape0[] = {
    {0, kWord, 0x82002A88}, {60, kImage, 0x82000A24, 4, 16}, {48, kImage, 0x82955D00}, {52, kImage, 0x82955D04},
    {56, kImage, 0x82955D08}, {60, kImage, 0x82955D0C}, {32, kImage, 0x82000A24}, {16, kImage, 0x82001BDC, 2, 4},
    {24, kImage, 0x82002940}, {28, kImage, 0x820021C4}, {64, kImage, 0x82000A20}, {68, kImage, 0x82000A24, 4, 4},
    {84, kImage, 0x82000A20}, {88, kImage, 0x82000A24, 4, 4}, {104, kImage, 0x82000A20}, {108, kImage, 0x82000A24},
    {112, kWord, 0x0},
};

// BoxShape::BoxShape (sub_820BAFD0).
// 17 stores
constexpr Fill kBoxShape_BoxShape0[] = {
    {0, kWord, 0x82002B10}, {140, kImage, 0x82000A24, 2, 16}, {188, kImage, 0x82000A24, 8, 16}, {332, kImage, 0x82000A24, 6, 32},
};

bool On() { return REXCVAR_GET(native_collision); }

using guest::LoadFloat;
using guest::LoadU32;
using guest::StoreFloat;
using guest::StoreU32;

constexpr uint32_t kEdgeStarts = 0x825F0D04;  // the 12 edges' first corners
constexpr uint32_t kEdgeEnds = 0x825F0D34;    // and their second
constexpr uint32_t kAxisCorners = 0x82002AEC;  // a face's corner by its axis
constexpr uint32_t kMinusOne = 0x82001BD8;     // -1.0f

// The vector library, recompiled: copy (sub_82291B20), r3 += r4
// (sub_82291DF8), dot (sub_82292048, in f1), r3 *= f1 (sub_82291D70).
void VectorCopy(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t from) {
  ctx.r3.u64 = to;
  ctx.r4.u64 = from;
  sub_82291B20(ctx, base);
}

void VectorAdd(PPCContext& ctx, uint8_t* base, uint32_t to, uint32_t v) {
  ctx.r3.u64 = to;
  ctx.r4.u64 = v;
  sub_82291DF8(ctx, base);
}

double Dot(PPCContext& ctx, uint8_t* base, uint32_t a, uint32_t b) {
  ctx.r3.u64 = a;
  ctx.r4.u64 = b;
  sub_82292048(ctx, base);
  return ctx.f1.f64;
}

void VectorScale(PPCContext& ctx, uint8_t* base, uint32_t v, double by) {
  ctx.r3.u64 = v;
  ctx.f1.f64 = by;
  sub_82291D70(ctx, base);
}

// A shape's vtable slot 4 (+16): its placement brought up to date.
void Refresh(PPCContext& ctx, uint8_t* base, uint32_t shape) {
  ctx.r3.u64 = LoadU32(base, shape);
  REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, ctx.r3.u32) + 16));
}

// CollisionShape::UpdateBounds (sub_820BAA50): the box around the previous
// and current centers (+4, +8: their +48), grown by the shape's radius
// (+0's +32): +16 the low corner, +32 the high one, axis by axis.
void UpdateBounds(uint8_t* base, uint32_t shape) {
  const uint32_t previous = LoadU32(base, shape + 4) + 48;
  const uint32_t current = LoadU32(base, shape + 8) + 48;
  const float radius = LoadFloat(base, LoadU32(base, shape + 0) + 32);
  for (uint32_t axis = 0; axis < 12; axis += 4) {
    const float a = LoadFloat(base, previous + axis);
    const float b = LoadFloat(base, current + axis);
    float high;
    if (!(a >= b)) {  // bge not taken: lower, or unordered
      StoreFloat(base, shape + 16 + axis, a - radius);
      high = LoadFloat(base, current + axis);
    } else {
      StoreFloat(base, shape + 16 + axis, b - radius);
      high = LoadFloat(base, previous + axis);
    }
    StoreFloat(base, shape + 32 + axis, high + radius);
  }
}

}  // namespace

// Query::Query.
extern "C" REX_FUNC(sub_820BD000) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BD000(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kQuery_Query0, args);
  ctx.r3.u64 = object;
}

// Shape::Shape.
extern "C" REX_FUNC(sub_820BAE70) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BAE70(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  guest_memory::Apply(base, object, kShape_Shape0, args);
  ctx.r3.u64 = object;
}

// BoxShape::BoxShape.
extern "C" REX_FUNC(sub_820BAFD0) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BAFD0(ctx, base);
    return;
  }
  const uint32_t object = ctx.r3.u32;
  const guest_memory::Args args(ctx);
  ctx.r3.u64 = object + 0;
  sub_820BAE70(ctx, base);
  guest_memory::Apply(base, object, kBoxShape_BoxShape0, args);
  ctx.r3.u64 = object + 0;
  ctx.r4.u64 = args[4];
  ctx.r5.u64 = args[5];
  sub_820B9840(ctx, base);
  ctx.r3.u64 = object;
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

SBX_CONSTANT_GETTER(82468988, 8)   // BoxShape::CornerCount
SBX_CONSTANT_GETTER(82469648, 6)   // BoxShape::FaceCount
SBX_CONSTANT_GETTER(820B9D58, 12)  // BoxShape::EdgeCount

#undef SBX_CONSTANT_GETTER

// BoxShape::Corner: r4 the corner, r3 + 176 + 16 x corner.
extern "C" REX_FUNC(sub_820B9D38) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B9D38(ctx, base);
    return;
  }
  ctx.r3.u64 = ctx.r3.u32 + ((ctx.r4.u32 + 11) << 4);
}

// BoxShape::Face: r4 the face, r3 + 304 + 32 x face.
extern "C" REX_FUNC(sub_820B9D48) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820B9D48(ctx, base);
    return;
  }
  ctx.r3.u64 = (ctx.r4.u32 << 5) + ctx.r3.u32 + 304;
}

// The edges' corners: by r4 (the box shape's) or r3 (BoxEdges').
#define SBX_EDGE_GETTER(address, table, reg) \
  extern "C" REX_FUNC(sub_##address) {       \
    REX_FUNC_PROLOGUE();                     \
    if (!On()) {                             \
      __imp__sub_##address(ctx, base);       \
      return;                                \
    }                                        \
    ctx.r3.u64 = LoadU32(base, table + (ctx.reg.u32 << 2)); \
  }

SBX_EDGE_GETTER(820B9DF8, kEdgeStarts, r4)  // BoxShape::EdgeStart
SBX_EDGE_GETTER(820B9E10, kEdgeEnds, r4)    // BoxShape::EdgeEnd
SBX_EDGE_GETTER(820B9E28, kEdgeStarts, r3)  // BoxEdges::Start
SBX_EDGE_GETTER(820B9E40, kEdgeEnds, r3)    // BoxEdges::End

#undef SBX_EDGE_GETTER

// BoxShape::SetCacheWord: r4 the word, r5 its value (+160 + 4 x word).
extern "C" REX_FUNC(sub_820BA4C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA4C8(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + ((ctx.r4.u32 + 40) << 2), ctx.r5.u32);
}

// BoxShape::CacheWord.
extern "C" REX_FUNC(sub_820BA4D8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA4D8(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, ctx.r3.u32 + ((ctx.r4.u32 + 40) << 2));
}

// BoxShape::CopyCache: r4's 16-byte cache (+160) into r3's.
extern "C" REX_FUNC(sub_820BA8A8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA8A8(ctx, base);
    return;
  }
  for (uint32_t i = 0; i < 16; i += 4) {
    StoreU32(base, ctx.r3.u32 + 160 + i, LoadU32(base, ctx.r4.u32 + 160 + i));
  }
}

// Shape::Radius: f1 = +32.
extern "C" REX_FUNC(sub_820BA998) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA998(ctx, base);
    return;
  }
  ctx.f1.f64 = LoadFloat(base, ctx.r3.u32 + 32);
}

// BoxShape::LowerCorner: r4 the axis; of the axis' two faces (the table at
// 0x82002AEC, and its opposite: xor 2), the corner of the one whose
// distance (+180 + 16 x i) is lower (the second when they're equal or
// unordered).
extern "C" REX_FUNC(sub_820BA1C8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA1C8(ctx, base);
    return;
  }
  const uint32_t box = ctx.r3.u32;
  const uint32_t first = LoadU32(base, kAxisCorners + (ctx.r4.u32 << 2));
  const uint32_t second = first ^ 2;
  const float a = LoadFloat(base, box + 180 + (first << 4));
  const float b = LoadFloat(base, box + 180 + (second << 4));
  const uint32_t corner = a < b ? first : second;
  ctx.r3.u64 = box + ((corner + 11) << 4);
}

// BoxShape::FaceDistance: r4 the face, r5 a point: the point's dot with the
// face's direction less its distance.
extern "C" REX_FUNC(sub_820BA210) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA210(ctx, base);
    return;
  }
  const uint32_t box = ctx.r3.u32, face = ctx.r4.u32;
  const float dot = float(Dot(ctx, base, box + ((face + 10) << 5), ctx.r5.u32));
  ctx.f1.f64 = double(dot - LoadFloat(base, box + (face << 5) + 304));
}

// BoxShape::Translate: r4 the offset added to the 8 corners and the center
// (+48), and each face's distance moved by the offset's dot with its
// direction.
extern "C" REX_FUNC(sub_820BA150) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA150(ctx, base);
    return;
  }
  const uint32_t box = ctx.r3.u32, offset = ctx.r4.u32;
  for (uint32_t i = 0; i < 8; i++) {
    VectorAdd(ctx, base, box + 176 + 16 * i, offset);
  }
  for (uint32_t i = 0; i < 6; i++) {
    const uint32_t face = box + 304 + 32 * i;
    const float dot = float(Dot(ctx, base, face + 16, offset));
    StoreFloat(base, face, dot + LoadFloat(base, face));
  }
  VectorAdd(ctx, base, box + 48, offset);
}

// WordAt8 (sub_822B8A90, shared): r3's +8.
extern "C" REX_FUNC(sub_822B8A90) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822B8A90(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, ctx.r3.u32 + 8);
}

// CollisionShape::CurrentCenter and PreviousCenter: +8's or +4's +48.
extern "C" REX_FUNC(sub_820BA978) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA978(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, ctx.r3.u32 + 8) + 48;
}

extern "C" REX_FUNC(sub_820BA988) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA988(ctx, base);
    return;
  }
  ctx.r3.u64 = LoadU32(base, ctx.r3.u32 + 4) + 48;
}

// CollisionShape::Type: the shape's vtable slot 4 (a tail call).
extern "C" REX_FUNC(sub_820BAB08) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BAB08(ctx, base);
    return;
  }
  Refresh(ctx, base, ctx.r3.u32);
}

// CollisionShape::Previous and Current: the shape's slot 4, then +4 or +8.
extern "C" REX_FUNC(sub_820BAC48) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BAC48(ctx, base);
    return;
  }
  const uint32_t shape = ctx.r3.u32;
  Refresh(ctx, base, shape);
  ctx.r3.u64 = LoadU32(base, shape + 4);
}

extern "C" REX_FUNC(sub_820BAC88) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BAC88(ctx, base);
    return;
  }
  const uint32_t shape = ctx.r3.u32;
  Refresh(ctx, base, shape);
  ctx.r3.u64 = LoadU32(base, shape + 8);
}

extern "C" REX_FUNC(sub_820BAA50) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BAA50(ctx, base);
    return;
  }
  UpdateBounds(base, ctx.r3.u32);
}

// CollisionShape::~CollisionShape: the two placements deleted (each one's
// vtable slot 0 with r4 = 1) and cleared.
extern "C" REX_FUNC(sub_820BA8F8) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BA8F8(ctx, base);
    return;
  }
  const uint32_t shape = ctx.r3.u32;
  for (uint32_t at = 4; at <= 8; at += 4) {
    const uint32_t placement = LoadU32(base, shape + at);
    if (placement == 0) {
      continue;
    }
    ctx.r3.u64 = placement;
    ctx.r4.u64 = 1;
    REX_CALL_INDIRECT_FUNC(LoadU32(base, LoadU32(base, placement)));
    StoreU32(base, shape + at, 0);
  }
}

// Query::SetShapes (sub_822DF518, shared): +0 and +4 = r4 and r5.
extern "C" REX_FUNC(sub_822DF518) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_822DF518(ctx, base);
    return;
  }
  StoreU32(base, ctx.r3.u32 + 0, ctx.r4.u32);
  StoreU32(base, ctx.r3.u32 + 4, ctx.r5.u32);
}

// Contact::Set: r4 copied to +0, r5 to +16 and +32; f1 and f2 to +48 and
// +52.
extern "C" REX_FUNC(sub_820BC740) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BC740(ctx, base);
    return;
  }
  const uint32_t contact = ctx.r3.u32, normal = ctx.r5.u32;
  const double a = ctx.f1.f64, b = ctx.f2.f64;
  VectorCopy(ctx, base, contact, ctx.r4.u32);
  VectorCopy(ctx, base, contact + 16, normal);
  VectorCopy(ctx, base, contact + 32, normal);
  StoreFloat(base, contact + 48, float(a));
  StoreFloat(base, contact + 52, float(b));
}

// Contact::Flip: +16 and +32 scaled by -1.
extern "C" REX_FUNC(sub_820BC690) {
  REX_FUNC_PROLOGUE();
  if (!On()) {
    __imp__sub_820BC690(ctx, base);
    return;
  }
  const uint32_t contact = ctx.r3.u32;
  const double minus_one = LoadFloat(base, kMinusOne);
  VectorScale(ctx, base, contact + 16, minus_one);
  VectorScale(ctx, base, contact + 32, minus_one);
}
