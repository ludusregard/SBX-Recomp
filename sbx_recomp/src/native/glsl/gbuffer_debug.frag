#version 450
// --gbuffer_debug (passes.cpp, DrawGBufferDebug): one of the G-buffer's
// buffers drawn over the whole frame, before it's shown.
//   1: view distance, white near to black at depth_range.
//   2: velocity: red right, green down (UV per frame times velocity_scale),
//      gray where nothing moves.
//   3: the game's own half-size velocity buffer, its two channels as red and
//      green (its encoding isn't worked out yet).
//   4: the depth as drawn (0..1), stretched near 1 (depths cluster there).

layout(set = 0, binding = 0) uniform sampler2D depth_buffer;
layout(set = 0, binding = 1) uniform sampler2D velocity_buffer;
layout(set = 0, binding = 2) uniform sampler2D game_velocity;

layout(push_constant) uniform Constants {
  vec2 extent;            // the frame's drawn part, in pixels
  vec2 game_uv_scale;     // the game's velocity buffer: its drawn part in UV
  int mode;
  float depth_range;
  float velocity_scale;
} constants;

layout(location = 0) out vec4 color;

void main() {
  vec2 uv = gl_FragCoord.xy / constants.extent;
  if (constants.mode == 1) {
    float view_distance = texture(depth_buffer, uv).g;
    color = vec4(vec3(1.0 - clamp(view_distance / constants.depth_range, 0.0, 1.0)), 1.0);
  } else if (constants.mode == 2) {
    vec2 velocity = texture(velocity_buffer, uv).rg * constants.velocity_scale;
    color = vec4(clamp(0.5 + velocity, 0.0, 1.0), 0.5, 1.0);
  } else if (constants.mode == 3) {
    vec2 game = texture(game_velocity, uv * constants.game_uv_scale).rg;
    color = vec4(game, 0.0, 1.0);
  } else {
    float depth = texture(depth_buffer, uv).r;
    color = vec4(vec3(pow(clamp(depth, 0.0, 1.0), 64.0)), 1.0);
  }
}
