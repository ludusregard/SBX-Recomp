// The game's HLSL compiler (D3DXCompileShader, sub_822B3678; ITC
// sub_822B8F10, exact) answered with the shaders it makes at boot, stored in
// src/graphics/compiled_shaders.h, instead of compiling them (ported from
// ITC-Recomp's src/hooks/shader_compiles.cpp, 2026-10-25). The compiler is a
// whole tree of the Xbox's D3DX library (~640 functions at ITC) that only
// runs for a dozen tiny shaders: the font's (Font::CreateShaders,
// sub_821089D8), the primitive drawer's (sub_82586B20) and sub_8209BFA8's.
// With this it's out of play, and out of the code to translate later.
//
// The answer is the compiler's own output, byte for byte: a buffer made
// with the game's D3DXCreateBuffer (sub_822B8C18; ITC sub_822BE450, exact),
// the microcode copied into it (its data pointer at +12). The callers read it
// through the buffer's slot 3 and release it through slot 2, as before. No
// error buffer or constant table (none is asked for but the primitive
// drawer's errors). A compile not in the table (or with defines, an include
// handler or flags) goes to the real compiler and is logged.
//
// The table is SBX's own (its sources are at other addresses than ITC's),
// made from the real compiler's output for these shaders.

#include "graphics/compiled_shaders.h"
#include "guest.h"

#include <rex/logging.h>

#include <string>
#include <string_view>

namespace {

std::string LoadString(uint8_t* base, uint32_t address, size_t limit = 256) {
  std::string s;
  if (address == 0) {
    return s;
  }
  for (uint8_t c; s.size() < limit && (c = guest::LoadU8(base, address)) != 0; address++) {
    s.push_back(static_cast<char>(c));
  }
  return s;
}

const graphics::compiled_shaders::Entry* Find(uint32_t source, uint32_t length,
                                              std::string_view profile) {
  for (const auto& shader : graphics::compiled_shaders::kShaders) {
    if (shader.data && shader.source == source && shader.length == length &&
        profile == shader.profile) {
      return &shader;
    }
  }
  return nullptr;
}

// D3DXCompileShader: r3 the source, r4 its length, r5 the defines, r6 the
// include handler, r7 the entry point, r8 the profile, r9 the flags, r10
// where the shader buffer goes; the caller's stack words +84 and +92 where
// the errors and the constant table go. Returns 0, or D3DXCreateBuffer's
// failure.
extern "C" REX_FUNC(sub_822B3678) {
  REX_FUNC_PROLOGUE();
  const auto* shader = Find(ctx.r3.u32, ctx.r4.u32, LoadString(base, ctx.r8.u32, 16));
  if (!shader || ctx.r5.u32 != 0 || ctx.r6.u32 != 0 || ctx.r9.u32 != 0) {
    REXLOG_INFO("hardcoded shaders: no stored shader for source {:08X} length {}, compiled",
                ctx.r3.u32, ctx.r4.u32);
    __imp__sub_822B3678(ctx, base);
    return;
  }
  const uint32_t shader_out = ctx.r10.u32;
  const uint32_t errors_out = guest::LoadU32(base, ctx.r1.u32 + 84);
  const uint32_t table_out = guest::LoadU32(base, ctx.r1.u32 + 92);
  ctx.r3.u64 = shader->size;
  ctx.r4.u64 = shader_out;
  sub_822B8C18(ctx, base);  // D3DXCreateBuffer(size, &buffer)
  if (ctx.r3.s32 < 0) {
    return;
  }
  const uint32_t data = guest::LoadU32(base, guest::LoadU32(base, shader_out) + 12);
  for (uint32_t i = 0; i < shader->size; i++) {
    guest::StoreU8(base, data + i, shader->data[i]);
  }
  if (errors_out) {
    guest::StoreU32(base, errors_out, 0);
  }
  if (table_out) {
    guest::StoreU32(base, table_out, 0);
  }
  ctx.r3.u64 = 0;
}
