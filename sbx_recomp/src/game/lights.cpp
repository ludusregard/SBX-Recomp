// Lights::FindNearest as our own C++ (--native_nearest_lights, on by
// default): the course lights nearest a point and within a cone, nearest
// first, for the car models' lighting. It was 13% of the main thread as
// recompiled (ITC-Recomp's measure, the same code): the translation
// switched the flush-to-zero mode around each vector instruction and read
// back through its stack frame what it had just stored.
//
// Here the distance and cone tests run on four lights at once, with the
// original's operations in its order (the dot product adds z² to y² + x²;
// the reciprocal square root is 1/sqrt and one Newton step), so the same
// lights pass. The lights that do are then merged and sorted one by one as
// the original does.

#include "game/data_file.h"
#include "game/lights.h"
#include "game/math.h"

#include <rex/cvar.h>

#include <smmintrin.h>

#include <cstring>
#include <iterator>
#include <utility>

REXCVAR_DEFINE_BOOL(native_nearest_lights, true, "SBX",
                    "Our own C++ for Lights::FindNearest (the course lights nearest a car) "
                    "instead of the recompiled function");

namespace {

using game::CourseLight;
using game::DataFile;
using game::LightGroup;
using game::LightHead;
using game::LightSetOwner;
using game::NearLight;
using game::VisibleArea;
using guest::At;

constexpr int32_t kMostLights = 32;
constexpr float kNearLightW = 1.0f;  // 0x82000A20, read-only.

// A light kept: its first 32 bytes, its position and its squared distance.
struct Kept {
  LightHead head;
  float x, y, z;
  float distance;
};

struct Query {
  float px, py, pz;
  __m128 dx, dy, dz;    // The direction, each component in every lane.
  float merge_squared;  // A light this near one kept is merged with it.
  __m128 farthest;      // The farthest squared distance.
  __m128 least_dot;     // The least dot product with the direction.
  int32_t most;
};

struct Search {
  Kept kept[kMostLights + 1];  // The last one is where a light goes when it's full.
  int32_t count = 0;
};

// The lights of a group that passed both tests, in order: a group scanned
// again passes the same ones (the tests don't depend on what's kept), so
// only these are taken again.
struct Passed {
  struct Light {
    const CourseLight* light;
    float x, y, z, distance;
  };
  Light lights[64];
  uint32_t count = 0;
  bool overflowed = false;
};

// A light's position (and the word after it), byte-swapped (pshufb).
__attribute__((target("ssse3"))) __m128 LoadPosition(const CourseLight* light) {
  const __m128i kReverse = _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
  __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&light->head));
  return _mm_castsi128_ps(_mm_shuffle_epi8(v, kReverse));
}

// One light that passed both tests: dropped or merged into a kept light
// within the merge distance (the nearest such, searching from the last
// kept), otherwise sorted in by distance; once full, the farthest drops off
// the end.
void Keep(const Query& query, Search& search, const CourseLight* light, float x, float y, float z,
          float distance) {
  Kept& added = search.kept[search.count];
  added.head = light->head;
  added.x = x;
  added.y = y;
  added.z = z;
  added.distance = distance;
  for (int32_t i = search.count - 1; i >= 0; i--) {
    Kept& other = search.kept[i];
    const float ex = x - other.x, ey = y - other.y, ez = z - other.z;
    const float apart = ez * ez + (ey * ey + ex * ex);
    if (!(apart > query.merge_squared)) {
      if (distance < other.distance) {
        other = added;
      }
      return;
    }
  }
  for (int32_t i = search.count; i > 0; i--) {
    if (!(search.kept[i - 1].distance > search.kept[i].distance)) {
      break;
    }
    std::swap(search.kept[i - 1], search.kept[i]);
  }
  if (search.count < query.most) {
    search.count++;
  }
}

// Tests a group's lights four at a time. SSE4.1 (_mm_blendv_ps) is enabled
// for these functions alone: the SDK reaches SIMD through simde, so the
// build doesn't turn it on everywhere.
__attribute__((target("ssse3,sse4.1"))) void ScanGroup(const Query& query, Search& search,
                                                        LightGroup* group, Passed* passed_lights) {
  const uint32_t count = group->light_count;
  const __m128 px = _mm_set1_ps(query.px), py = _mm_set1_ps(query.py),
               pz = _mm_set1_ps(query.pz);
  const __m128 zero = _mm_setzero_ps(), one = _mm_set1_ps(1.0f), half = _mm_set1_ps(0.5f);
  const __m128 sign = _mm_castsi128_ps(_mm_set1_epi32(int(0x80000000)));
  for (uint32_t first = 0; first < count; first += 4) {
    const uint32_t in_group = count - first < 4 ? count - first : 4;
    // Lanes past the group's end repeat its last light, and are ignored.
    __m128 l0 = LoadPosition(group->Light(first));
    __m128 l1 = LoadPosition(group->Light(first + (in_group > 1 ? 1 : 0)));
    __m128 l2 = LoadPosition(group->Light(first + (in_group > 2 ? 2 : 0)));
    __m128 l3 = LoadPosition(group->Light(first + (in_group > 3 ? 3 : 0)));
    _MM_TRANSPOSE4_PS(l0, l1, l2, l3);  // Now x, y, z, w of each light.
    const __m128 dx = _mm_sub_ps(l0, px), dy = _mm_sub_ps(l1, py), dz = _mm_sub_ps(l2, pz);
    const __m128 distance =
        _mm_add_ps(_mm_mul_ps(dz, dz), _mm_add_ps(_mm_mul_ps(dy, dy), _mm_mul_ps(dx, dx)));
    const int valid = (1 << in_group) - 1;
    const int near = _mm_movemask_ps(_mm_cmpngt_ps(distance, query.farthest)) & valid;
    if (near == 0) {
      continue;
    }
    // The direction to each light (0 for one at the point), as the original:
    // 1/sqrt, then r + r * -((d / 2) * r² - 1/2).
    __m128 r = _mm_div_ps(one, _mm_sqrt_ps(distance));
    const __m128 step = _mm_xor_ps(
        _mm_sub_ps(_mm_mul_ps(_mm_mul_ps(distance, half), _mm_mul_ps(r, r)), half), sign);
    r = _mm_add_ps(_mm_mul_ps(r, step), r);
    const __m128 at_point = _mm_cmpeq_ps(distance, zero);
    const __m128 nx = _mm_blendv_ps(_mm_mul_ps(dx, r), distance, at_point);
    const __m128 ny = _mm_blendv_ps(_mm_mul_ps(dy, r), distance, at_point);
    const __m128 nz = _mm_blendv_ps(_mm_mul_ps(dz, r), distance, at_point);
    const __m128 dot = _mm_add_ps(_mm_add_ps(zero, _mm_mul_ps(nz, query.dz)),
                                  _mm_add_ps(_mm_mul_ps(ny, query.dy), _mm_mul_ps(nx, query.dx)));
    int passed = near & _mm_movemask_ps(_mm_cmpnlt_ps(dot, query.least_dot));
    if (passed == 0) {
      continue;
    }
    alignas(16) float xs[4], ys[4], zs[4], distances[4];
    _mm_store_ps(xs, l0);
    _mm_store_ps(ys, l1);
    _mm_store_ps(zs, l2);
    _mm_store_ps(distances, distance);
    for (; passed; passed &= passed - 1) {
      const int lane = __builtin_ctz(passed);
      const CourseLight* light = group->Light(first + lane);
      Keep(query, search, light, xs[lane], ys[lane], zs[lane], distances[lane]);
      if (passed_lights) {
        if (passed_lights->count < std::size(passed_lights->lights)) {
          passed_lights->lights[passed_lights->count++] = {light, xs[lane], ys[lane], zs[lane],
                                                           distances[lane]};
        } else {
          passed_lights->overflowed = true;
        }
      }
    }
  }
}

// The search as the original walks it: each visible area's light set, each
// group of the set, and the group's lights as many times as its scan count
// says (the scans after the first take the lights the first passed, unless
// there were too many to keep).
void FindNearest(LightSetOwner* owner, const Query& query, Search& search) {
  for (uint32_t i = 0; i < owner->area_count; i++) {
    DataFile* set = At<DataFile>(owner->Area(i)->lights->light_set);
    if (set == nullptr) {
      continue;
    }
    for (uint32_t j = 0; j < set->count; j++) {
      LightGroup* group = reinterpret_cast<LightGroup*>(set->Entry(j, false));
      if (group->light_count == 0) {
        continue;
      }
      const uint32_t scans = group->scans;
      if (scans == 1) {
        ScanGroup(query, search, group, nullptr);
        continue;
      }
      Passed passed;
      for (uint32_t scan = 0; scan < scans; scan++) {
        if (scan == 0 || passed.overflowed) {
          ScanGroup(query, search, group, scan == 0 ? &passed : nullptr);
          continue;
        }
        for (uint32_t k = 0; k < passed.count; k++) {
          const Passed::Light& light = passed.lights[k];
          Keep(query, search, light.light, light.x, light.y, light.z, light.distance);
        }
      }
    }
  }
}

// The kept lights written out: each position with w = 1.0, then its 16
// bytes.
void Write(const Search& search, NearLight* out) {
  for (int32_t i = 0; i < search.count; i++) {
    const LightHead& head = search.kept[i].head;
    out[i].x = head.x;
    out[i].y = head.y;
    out[i].z = head.z;
    out[i].w = kNearLightW;
    std::memcpy(out[i].unknown_16, head.unknown_16, sizeof(out[i].unknown_16));
  }
}

bool IsFloat(double value) { return double(float(value)) == value; }

}  // namespace

// Lights::FindNearest: r3 the scenery (the light sets' owner), r4 the
// output, r5 the point, r6 the most lights (at most 32), r9 the direction,
// f1 the merge distance, f2 the farthest squared distance, f3 the least dot
// product with the direction; returns the lights written. The only caller
// reads only r3 after the call.
extern "C" REX_FUNC(sub_820F3FF8) {
  REX_FUNC_PROLOGUE();
  ctx.fpscr.disableFlushMode();
  // The original compares in double what's made here in float: the caller
  // passes floats, but anything else goes to the original.
  if (!REXCVAR_GET(native_nearest_lights) || !IsFloat(ctx.f2.f64) || !IsFloat(ctx.f3.f64))
      [[unlikely]] {
    __imp__sub_820F3FF8(ctx, base);
    return;
  }
  guest::Guest g(ctx, base);
  LightSetOwner* owner = At<LightSetOwner>(ctx.r3.u32);
  const int32_t most = ctx.r6.s32;
  if (owner->area_count == 0 || most > kMostLights) {
    ctx.r3.u64 = 0;
    return;
  }
  const game::Vector* point = At<game::Vector>(ctx.r5.u32);
  const game::Vector* direction = At<game::Vector>(ctx.r9.u32 & ~0xFu);  // Aligned, as lvx.
  Query query;
  query.px = point->x;
  query.py = point->y;
  query.pz = point->z;
  query.dx = _mm_set1_ps(direction->x);
  query.dy = _mm_set1_ps(direction->y);
  query.dz = _mm_set1_ps(direction->z);
  query.merge_squared = float(ctx.f1.f64 * ctx.f1.f64);
  query.farthest = _mm_set1_ps(float(ctx.f2.f64));
  query.least_dot = _mm_set1_ps(float(ctx.f3.f64));
  query.most = most;
  Search search;
  FindNearest(owner, query, search);
  Write(search, At<NearLight>(ctx.r4.u32));
  ctx.r3.u64 = uint64_t(int64_t(search.count));
}
