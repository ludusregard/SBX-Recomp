// The part models' meshes and materials, as far as they're worked out.

#pragma once

#include "game/guest_types.h"
#include "game/math.h"

namespace game {

using guest::f32_be;
using guest::ptr;
using guest::u32_be;

struct Material {
  static constexpr uint32_t kKind1002 = 1002;

  uint8_t unknown_0[12];
  f32_be alpha;
  uint8_t unknown_16[376];
  u32_be kind;
  uint8_t unknown_396[116];
};
GUEST_SIZE(Material, 512);
GUEST_OFFSET(Material, kind, 392);

// A mesh's materials: the count, then the materials from +16.
struct MaterialList {
  u32_be count;
  uint8_t unknown_4[12];
  Material materials[1];
};
GUEST_OFFSET(MaterialList, materials, 16);

// A model's mesh: two vertex streams (each a size and its data) and its
// materials.
struct Mesh {
  uint8_t unknown_0[268];
  u32_be stream_size;
  uint8_t unknown_272[12];
  u32_be stream2_size;
  uint8_t unknown_288[100];
  u32_be stream_data;
  u32_be stream2_data;
  ptr<MaterialList> materials;
  uint8_t unknown_400[144];
};
GUEST_SIZE(Mesh, 544);
GUEST_OFFSET(Mesh, stream_size, 268);
GUEST_OFFSET(Mesh, stream2_size, 284);
GUEST_OFFSET(Mesh, stream_data, 388);
GUEST_OFFSET(Mesh, materials, 396);

// An XPR (the game's packed resources) a model or holder keeps.
struct XPR {
  u32_be words[6];
};
GUEST_SIZE(XPR, 24);

// A model's list of XPR objects (XPRList::XPRList, sub_8257A150; vtable
// 0x82069A94).
struct XPRList {
  u32_be vtable;
  uint8_t unknown_4[16];
};
GUEST_SIZE(XPRList, 20);

// A set of animation tracks (game/model_animation.cpp).
struct TrackSet {
  u32_be unknown_0[3];
  f32_be time;
  u32_be unknown_16;
};
GUEST_SIZE(TrackSet, 20);

struct Model {
  u32_be vtable;
  uint8_t unknown_4[12];
  u32_be unknown_16[2];
  u32_be shader_pairs;  // The address of its ShaderPairEntry array.
  u32_be unknown_28;
  uint8_t unknown_32[8];
  u32_be meshes;  // The address of its Mesh array.
  uint8_t unknown_44[4];
  u32_be unknown_48;
  uint8_t unknown_52[4];
  XPR xpr;
  uint8_t member_80[32];   // Built by sub_8257A150.
  uint8_t skeleton[192];   // A Skeleton.
  TrackSet tracks[5];
  u32_be unknown_404[4];
  u32_be shader_pair_count;

  Mesh* MeshAt(uint32_t index) { return guest::Element<Mesh>(meshes, index); }
};
GUEST_OFFSET(Model, meshes, 40);
GUEST_OFFSET(Model, xpr, 56);
GUEST_OFFSET(Model, skeleton, 112);
GUEST_OFFSET(Model, tracks, 304);
GUEST_OFFSET(Model, unknown_404, 404);
GUEST_OFFSET(Model, shader_pairs, 24);
GUEST_OFFSET(Model, shader_pair_count, 420);

// A car part's model.
struct PartModel {
  Model model;
  uint8_t unknown_424[8];
  u32_be unknown_432;
  guest::i32_be unknown_436;
  u32_be unknown_440;
  u32_be vinyl_textures[2];
  u32_be unknown_452[3];
};
GUEST_OFFSET(PartModel, unknown_432, 432);
GUEST_OFFSET(PartModel, vinyl_textures, 444);

// A row of the paint table (0x82003660).
struct PaintRecord {
  uint8_t unknown_0[48];
};
GUEST_SIZE(PaintRecord, 48);

}  // namespace game
