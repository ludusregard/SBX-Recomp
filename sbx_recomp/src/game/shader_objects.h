// The graphics library's shader objects: a shader file (its header, then
// its microcode), the vertex and pixel shader objects made from one (the
// file's header copied after their fields), and the tables of shader pairs
// the models and the shared pairs keep.

#pragma once

#include "game/guest_types.h"

namespace game {

using guest::i32_be;
using guest::u32_be;

// A shader file: its header is its first `header_bytes` bytes, the
// microcode right after. A vertex shader's magic has some of its low 7
// bits set.
struct ShaderFile {
  u32_be magic;
  u32_be header_bytes;
  u32_be code_bytes;
};

// ShaderFile::Parse's record.
struct ShaderRecord {
  u32_be file;
  u32_be object_bytes;  // The object's fields and the file's header.
  u32_be code;
  u32_be code_bytes;
};
GUEST_SIZE(ShaderRecord, 16);

// A vertex shader's file header, as copied after its object: +20 the
// offset of its info.
struct VertexShaderHeader {
  uint8_t unknown_0[20];
  u32_be info_offset;
};

// Its info: where its fetch words are (in words, after 10) and how many.
struct VertexShaderInfo {
  uint8_t unknown_0[24];
  u32_be fetch_offset;
  u32_be fetch_count;
};

struct FetchConstant {
  u32_be words[3];
};

struct VertexShader {
  static constexpr uint32_t kFlags = 6;

  u32_be flags;  // | 0x100000 once VertexShader::Create made it.
  u32_be unknown_4;
  uint8_t unknown_8[12];
  u32_be unknown_20;
  i32_be unknown_24;
  uint8_t unknown_28[20];
  u32_be code;
  uint8_t unknown_52[8];
  u32_be fetch_count;
  uint8_t unknown_64[4];
  u32_be fetches[32];
  FetchConstant constants[33];  // Each fetch's, from the microcode.
  // The file's header follows.
};
GUEST_SIZE(VertexShader, 592);
GUEST_OFFSET(VertexShader, code, 48);
GUEST_OFFSET(VertexShader, fetch_count, 60);
GUEST_OFFSET(VertexShader, fetches, 68);
GUEST_OFFSET(VertexShader, constants, 196);

struct PixelShader {
  static constexpr uint32_t kFlags = 7;

  u32_be flags;
  u32_be unknown_4;
  uint8_t unknown_8[12];
  u32_be unknown_20;
  u32_be code;
  uint8_t unknown_28[28];
  // The file's header follows.
};
GUEST_SIZE(PixelShader, 56);
GUEST_OFFSET(PixelShader, code, 24);

// A model's shader pair: the two objects and their microcodes.
struct ShaderPairEntry {
  u32_be vertex;
  u32_be pixel;
  u32_be vertex_code;
  u32_be pixel_code;
};
GUEST_SIZE(ShaderPairEntry, 16);

// A pair in a model's shader section: the vertex file at +32, the pixel
// file after it, the next pair after that.
struct ShaderSectionPair {
  uint8_t unknown_0[4];
  u32_be vertex_bytes;
  u32_be pixel_bytes;
  uint8_t unknown_12[20];
};
GUEST_SIZE(ShaderSectionPair, 32);

struct ShaderSection {
  u32_be pair_count;
  uint8_t unknown_4[28];
  // The pairs follow.
};
GUEST_SIZE(ShaderSection, 32);

// Shaders::OnPairLoaded's context.
struct ShaderPairLoad {
  u32_be vertex_file;
  u32_be pixel_file;
  u32_be pair;
};
GUEST_SIZE(ShaderPairLoad, 12);

}  // namespace game
