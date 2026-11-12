// The course's lights as Lights::FindNearest reads them: the scenery's
// visible areas, each area's light set (a container of light groups), each
// group's lights.

#pragma once

#include "game/course_scenery.h"
#include "game/guest_types.h"

namespace game {

using guest::f32_be;
using guest::ptr;
using guest::u32_be;

// A light's first 32 bytes: its position, then 16 bytes the nearest-lights
// list copies out with it.
struct LightHead {
  f32_be x, y, z;
  uint8_t unknown_12[4];
  uint8_t unknown_16[16];
};
GUEST_SIZE(LightHead, 32);

struct CourseLight {
  LightHead head;
  uint8_t unknown_32[16];
};
GUEST_SIZE(CourseLight, 48);

// One entry of an area's light set: how many times its lights are scanned
// (the same lights each time), and the lights.
struct LightGroup {
  u32_be scans;
  uint8_t unknown_4[56];
  u32_be light_count;
  uint8_t unknown_64[12];
  CourseLight lights[1];  // The first of `light_count`.

  CourseLight* Light(uint32_t index) {
    return guest::Element<CourseLight>(guest::AddressOf(lights), index);
  }
};
GUEST_OFFSET(LightGroup, light_count, 60);
GUEST_OFFSET(LightGroup, lights, 76);

// One of the scenery's visible areas (272 bytes).
struct VisibleArea {
  uint8_t unknown_0[12];
  ptr<AreaLights> lights;
  uint8_t unknown_16[256];
};
GUEST_SIZE(VisibleArea, 272);

// What Lights::FindNearest searches: the course scenery manager, as far as
// its visible areas go.
struct LightSetOwner {
  uint8_t unknown_0[216];
  u32_be area_count;
  uint8_t unknown_220[4];
  u32_be areas;  // The address of its VisibleArea array.

  VisibleArea* Area(uint32_t index) { return guest::Element<VisibleArea>(areas, index); }
};
GUEST_OFFSET(LightSetOwner, area_count, 216);
GUEST_OFFSET(LightSetOwner, areas, 224);

// A light as the nearest-lights list holds it: the position with w = 1.0,
// then the light's 16 bytes.
struct NearLight {
  f32_be x, y, z, w;
  uint8_t unknown_16[16];
};
GUEST_SIZE(NearLight, 32);

}  // namespace game
