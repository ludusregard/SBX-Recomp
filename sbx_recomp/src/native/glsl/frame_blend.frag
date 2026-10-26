#version 450
// Frame blending motion blur (--blur_frame_blend; passes.cpp, ApplyFrameBlend):
// a camera shutter approximated by averaging the rendered frames of the 3D
// view from the last shutter (the game's 30 Hz tick, 1/30 s), as bxt.rs's
// Half-Life recorder blends sub-frames but sliding, one output per rendered
// frame. As Halo 3's blur, it's kept off the centre of the screen: the
// current frame there, the average towards the edges. Needs frames between
// the game's ticks (--subtick) at a rate above 30 (--max_fps): with fewer
// frames in the shutter there's less to blend.
// `frames` holds the frames in a ring, `current` the newest's layer; the
// `count` frames of the shutter are the newest and the ones before it.

layout(set = 0, binding = 0) uniform sampler2DArray frames;

layout(push_constant) uniform Constants {
  int current;     // the newest frame's layer
  int count;       // frames in the shutter, the newest included (1: none to blend)
  float inner;     // radius (0 the centre, 1 the corners) kept sharp
  float outer;     // radius from where the blur is full
  float strength;  // the full blur's share (0-1)
} constants;

layout(location = 0) out vec4 color;

void main() {
  ivec2 position = ivec2(gl_FragCoord.xy);
  vec4 now = texelFetch(frames, ivec3(position, constants.current), 0);
  vec2 size = vec2(textureSize(frames, 0).xy);
  float aspect = size.x / size.y;
  vec2 offset = (gl_FragCoord.xy / size - 0.5) * vec2(aspect, 1.0);
  float radius = length(offset) / length(vec2(0.5 * aspect, 0.5));
  float amount = smoothstep(constants.inner, max(constants.outer, constants.inner + 1e-3),
                            radius) * constants.strength;
  if (amount <= 0.0 || constants.count <= 1) {
    color = now;
    return;
  }
  // The layers of the shutter, oldest to newest, wrapping round the ring.
  int layers = textureSize(frames, 0).z;
  vec3 sum = vec3(0.0);
  for (int i = 0; i < constants.count; i++) {
    int layer = (constants.current - constants.count + 1 + i + layers) % layers;
    sum += texelFetch(frames, ivec3(position, layer), 0).rgb;
  }
  color = vec4(mix(now.rgb, sum / float(constants.count), amount), now.a);
}
