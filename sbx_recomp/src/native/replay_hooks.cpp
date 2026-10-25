// The render command handlers (the table at 0x8264EB40; see
// docs/gpu-functions.md), wrapped to pass each command on before it runs.

#include "draw_packets.h"
#include "guest.h"
#include "renderer.h"
#include "state.h"

#include "base/log.h"

#include <set>
#include <utility>

// Command handlers, wrapped so the native renderer sees every command.
// Ids 0, 36 and 37 aren't wrapped: they do nothing (ITC's sub_82304258, a
// two-instruction stub that isn't a function of its own in SBX).
// Addresses are SBX's (docs/native-renderer-port.md); command 7, set shader
// constants, is wrapped here too (ITC wraps it in its interpolation code).
#define REPLAY_COUNTED_HANDLER(address, id)                                         \
  extern "C" REX_FUNC(sub_##address) {                                              \
    REX_FUNC_PROLOGUE();                                                            \
    uint32_t payload = ctx.r3.u32, arg1 = ctx.r4.u32, arg2 = ctx.r5.u32;           \
    native::Renderer_OnCommand(base, id, payload, arg1, arg2);                      \
    uint32_t outer_command = native::g_current_command;                             \
    native::g_current_command = id;                                                 \
    native::g_command_serial++;                                                     \
    __imp__sub_##address(ctx, base);                                                \
    native::g_current_command = outer_command;                                      \
    native::Renderer_AfterCommand(base, id, payload, arg1, arg2);                   \
  }

// The library's vertex shader patcher (see "Vertex declaration" in
// docs/gpu-functions.md): counted, so the native renderer can tell whether a
// draw's library call patched the shader's microcode.
namespace native {
uint32_t g_vertex_shader_patches = 0;
uint32_t g_last_patch[3] = {};
LoadedVertexShader g_loaded_vertex_shader;
uint32_t g_current_command = 0;
uint32_t g_command_serial = 0;
}

// When the GPU may still be using a vertex shader's microcode, the library
// can't patch it in place: sub_822AB600 copies it into the command buffer
// (IM_LOAD_IMMEDIATE) and patches the copy, leaving the object's microcode
// (+48) and the id it's patched for (+24) as they were. The GPU then runs
// the copy, and keeps running it while the shader isn't loaded again. So
// the copy, once patched, is kept here for the native renderer.
extern "C" REX_FUNC(sub_822AB028) {
  REX_FUNC_PROLOGUE();
  uint32_t object = ctx.r3.u32, microcode = ctx.r4.u32, declaration = ctx.r5.u32;
  bool copy = microcode != guest::LoadU32(base, object + 48);
  native::g_vertex_shader_patches++;
  native::g_last_patch[0] = object;
  native::g_last_patch[1] = declaration;
  native::g_last_patch[2] = declaration ? guest::LoadU32(base, declaration + 48) : 0;
  __imp__sub_822AB028(ctx, base);
  if (copy) {
    native::LoadedVertexShader& loaded = native::g_loaded_vertex_shader;
    loaded.object = object;
    loaded.declaration_id = declaration ? guest::LoadU32(base, declaration + 48) : 0;
    uint32_t size = guest::LoadU32(base, object + 600);
    loaded.microcode.assign(base + microcode, base + microcode + size);
  } else {
    native::g_loaded_vertex_shader.object = 0;  // the object's own, patched in place
  }
}

// Inline vertex draws (DrawPrimitiveUP, command 19, and the helper object's
// draws, command 15; see "Inline vertex draws" in docs/gpu-functions.md):
// BeginVertices (sub_825A31A0: device, primitive type, vertex count, stride
// in bytes) returns where in the command buffer the caller copies the
// vertices; End (sub_825A3AE0) follows the copy, so the native renderer
// draws them then.
namespace {
struct PendingInline {
  bool pending = false;
  uint32_t type, count, stride, data, caller;
} g_pending_inline;
// Library calls that draw or resolve made outside a replayed command (e.g.
// the front buffer resolve), logged once per function and caller.
std::set<std::pair<uint32_t, uint32_t>> g_logged_outside;
void LogOutside(const char* what, uint32_t function, uint32_t caller, uint32_t command) {
  if (!g_logged_outside.insert({function, caller}).second) {
    return;
  }
  ITC_LOG_INFO("native renderer: {} ({:08X}) called from {:08X} in command {}{}", what, function,
               caller, command, command ? "" : " (outside the command list)");
}
}  // namespace

extern "C" REX_FUNC(sub_825A31A0) {
  REX_FUNC_PROLOGUE();
  uint32_t type = ctx.r4.u32, count = ctx.r5.u32, stride = ctx.r6.u32;
  uint32_t caller = uint32_t(ctx.lr);
  __imp__sub_825A31A0(ctx, base);
  g_pending_inline = {ctx.r3.u32 != 0, type, count, stride, ctx.r3.u32, caller};
}

extern "C" REX_FUNC(sub_825A3AE0) {
  REX_FUNC_PROLOGUE();
  PendingInline draw = g_pending_inline;
  g_pending_inline.pending = false;
  if (draw.pending) {
    uint32_t command = native::g_current_command;
    if (command == 15 || command == 19) {
      native::Renderer_InlineDraw(base, draw.type, draw.count, draw.stride, draw.data);
    } else {
      LogOutside("an inline vertex draw, not drawn, BeginVertices", 0x825A31A0, draw.caller,
                 command);
    }
  }
  __imp__sub_825A3AE0(ctx, base);
}

REPLAY_COUNTED_HANDLER(82575740, 1)
REPLAY_COUNTED_HANDLER(825757A0, 2)
REPLAY_COUNTED_HANDLER(82575800, 3)
REPLAY_COUNTED_HANDLER(82575850, 4)
REPLAY_COUNTED_HANDLER(82574608, 5)
REPLAY_COUNTED_HANDLER(82574630, 6)
REPLAY_COUNTED_HANDLER(825758A0, 7)
REPLAY_COUNTED_HANDLER(82575A58, 8)
REPLAY_COUNTED_HANDLER(82575AA8, 9)
REPLAY_COUNTED_HANDLER(82575AF8, 10)
REPLAY_COUNTED_HANDLER(82575B48, 11)
REPLAY_COUNTED_HANDLER(82575BA0, 12)
REPLAY_COUNTED_HANDLER(82574668, 13)  // also 14: one handler
REPLAY_COUNTED_HANDLER(82575BF0, 15)
REPLAY_COUNTED_HANDLER(82575C98, 16)
REPLAY_COUNTED_HANDLER(82575D50, 17)
REPLAY_COUNTED_HANDLER(82575DC8, 18)
REPLAY_COUNTED_HANDLER(82575E48, 19)
REPLAY_COUNTED_HANDLER(82575EC0, 20)
REPLAY_COUNTED_HANDLER(825746E0, 21)
REPLAY_COUNTED_HANDLER(82577018, 22)
REPLAY_COUNTED_HANDLER(82575F10, 23)
REPLAY_COUNTED_HANDLER(82575F60, 24)
REPLAY_COUNTED_HANDLER(82575FB0, 25)
REPLAY_COUNTED_HANDLER(82576108, 26)
REPLAY_COUNTED_HANDLER(82576180, 27)
REPLAY_COUNTED_HANDLER(825746F0, 28)
REPLAY_COUNTED_HANDLER(82576250, 29)
REPLAY_COUNTED_HANDLER(82576348, 30)
REPLAY_COUNTED_HANDLER(825763C8, 31)
REPLAY_COUNTED_HANDLER(82576450, 32)
REPLAY_COUNTED_HANDLER(825764D0, 33)
REPLAY_COUNTED_HANDLER(82576550, 34)
REPLAY_COUNTED_HANDLER(825765B8, 35)


// The chain walker (sub_82574AA8) also has items of single commands, a
// slot each ([id, arguments]), dispatched by sub_82574238 through a second
// table at 0x8264EAA8 with the same ids. In a race only 5, 7, 13 and 15
// come this way (a few a frame: the minimap's player triangle). Its 15,
// the helper object's draw, is marked as command 15 so the native renderer
// draws its inline vertices; the others reach the device's shadow, which
// the renderer reads anyway.
extern "C" REX_FUNC(sub_82575148) {
  REX_FUNC_PROLOGUE();
  uint32_t outer_command = native::g_current_command;
  native::g_current_command = 15;
  __imp__sub_82575148(ctx, base);
  native::g_current_command = outer_command;
}

// The frame's end (see "The end of a frame" in docs/gpu-functions.md):
// sub_82573538(video output) resolves render target 0 into one of two front
// buffer textures (video output + 4 * (68 + *(+268))) with the library's
// Resolve, outside command 25; the native renderer shows that target.
extern "C" REX_FUNC(sub_82573538) {
  REX_FUNC_PROLOGUE();
  uint32_t video_output = ctx.r3.u32;
  uint32_t front_buffer =
      guest::LoadU32(base, video_output + 4 * (68 + guest::LoadU32(base, video_output + 268)));
  native::Renderer_FrontBufferResolve(base, guest::LoadU32(base, video_output + 140), front_buffer);
  __imp__sub_82573538(ctx, base);
}

// The library's other draws and Resolve: logged when called outside the
// commands that are known to make them (18, 17, 27, 25).
#define LOG_OUTSIDE_WRAPPER(address, what, expected)                                  \
  extern "C" REX_FUNC(sub_##address) {                                                \
    REX_FUNC_PROLOGUE();                                                              \
    if (native::g_current_command != (expected)) {                                    \
      LogOutside(what, 0x##address, uint32_t(ctx.lr), native::g_current_command);     \
    }                                                                                 \
    __imp__sub_##address(ctx, base);                                                  \
  }
LOG_OUTSIDE_WRAPPER(825A3B00, "DrawVertices", 17)
LOG_OUTSIDE_WRAPPER(825A35F0, "BeginIndexedVertices", 27)

// DrawIndexedVertices: in exclusive mode without its packets (see
// draw_packets.cpp).
extern "C" REX_FUNC(sub_825A3ED0) {
  REX_FUNC_PROLOGUE();
  if (native::g_current_command != 18) {
    LogOutside("DrawIndexedVertices", 0x825A3ED0, uint32_t(ctx.lr), native::g_current_command);
  }
  native::DrawPackets_IndexedDraw(ctx, base);
}

// The library's Resolve (r3 device, r4 flags, r5 source rect, r6
// destination texture, r7 destination point, r8 level, r9 slice, r10 clear
// color as 4 floats, f1 clear Z). Command 25's are handled from its items;
// command 33 (EndTiling, see "Predicated tiling" in docs/gpu-functions.md)
// makes one per tile (the tile count at device +12488), passed on here.
extern "C" REX_FUNC(sub_822AC858) {
  REX_FUNC_PROLOGUE();
  static uint32_t serial = 0, tile = 0;
  uint32_t command = native::g_current_command;
  if (command == 33) {
    if (serial != native::g_command_serial) {
      serial = native::g_command_serial;
      tile = 0;
    }
    uint32_t device = ctx.r3.u32;
    native::Renderer_TiledResolve(base, device, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32,
                                  ctx.r10.u32, float(ctx.f1.f64), tile++,
                                  guest::LoadU32(base, device + 12772) /* SBX (ITC +12488) */);
  } else if (command != 25) {
    LogOutside("Resolve", 0x822AC858, uint32_t(ctx.lr), command);
  }
  __imp__sub_822AC858(ctx, base);
}
