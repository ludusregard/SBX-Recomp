#version 450
// Anti-aliasing's result back into every sample of the main view: aa_in.frag's
// tonemap undone.

layout(set = 0, binding = 0) uniform sampler2D source;

layout(location = 0) out vec4 color;

const float kExposure = 0.125;

vec3 Untonemap(vec3 t) {
  t = clamp(t, vec3(0.0), vec3(0.999));
  return t / ((1.0 - max(max(t.r, t.g), t.b)) * kExposure);
}

void main() {
  vec4 t = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
  color = vec4(Untonemap(t.rgb), t.a);
}
