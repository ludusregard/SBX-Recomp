#version 450
// G-buffer lite (--gbuffer; passes.cpp, BuildGBuffer): from the main view's
// depth, at its opaque resolve, our own buffers that last past the frame:
//   0: depth (R32G32F): the depth as drawn (0..1), and the view distance (the
//      point's clip w, in the game's units).
//   1: velocity (R16G16F): where the point is now minus where it was last
//      frame, in UV (0..1 across the view): the camera's motion only.
// The point's position comes back from the depth through this frame's
// inverse view-projection, then goes through last frame's view-projection;
// the two are pushed multiplied together (reprojection), with the inverse's
// last row for the distance, to stay within 128 bytes of push constants.
// The matrices are the game's (camera pass 0, row vectors), pushed as they
// are: read as GLSL's column-major mat4 they're transposed, so matrix *
// column vector is the game's row vector * matrix.

#ifdef MULTISAMPLE
layout(set = 0, binding = 0) uniform sampler2DMS depth_source;
#else
layout(set = 0, binding = 0) uniform sampler2D depth_source;
#endif

layout(push_constant) uniform Constants {
  mat4 reprojection;        // last frame's view-projection * this frame's inverse
  vec4 inverse_w;           // this frame's inverse view-projection's w row
  vec2 extent;              // the view's drawn part, in pixels
  int has_previous;         // 0: no last frame (velocity 0)
} constants;

layout(location = 0) out vec2 depth_out;
layout(location = 1) out vec2 velocity_out;

void main() {
  ivec2 position = ivec2(gl_FragCoord.xy);
  // Sample 0: the samples of one pixel are at one depth but at edges.
  float depth = texelFetch(depth_source, position, 0).r;
  vec2 uv = gl_FragCoord.xy / constants.extent;
  // The game's screen: y up in normalized device coordinates, down in pixels.
  vec4 ndc = vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
  // The point, homogeneous, has w = 1 / its clip w.
  float w = dot(constants.inverse_w, ndc);
  float view_distance = w != 0.0 ? 1.0 / w : 0.0;
  depth_out = vec2(depth, view_distance);

  vec2 velocity = vec2(0.0);
  if (constants.has_previous != 0 && w != 0.0) {
    // Last frame's clip position, times w (which cancels below).
    vec4 previous = constants.reprojection * ndc;
    if (previous.w / w > 1e-6) {
      vec2 previous_ndc = previous.xy / previous.w;
      vec2 previous_uv = vec2(previous_ndc.x * 0.5 + 0.5, 0.5 - previous_ndc.y * 0.5);
      velocity = uv - previous_uv;
    }
  }
  velocity_out = velocity;
}
