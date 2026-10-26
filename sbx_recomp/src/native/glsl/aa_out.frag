#version 450
// Anti-aliasing's result back into every sample of the view: aa_in.frag's
// tonemap undone, at the same exposure.

layout(set = 0, binding = 0) uniform sampler2D source;

layout(push_constant) uniform Constants {
  float exposure;
} constants;

layout(location = 0) out vec4 color;

vec3 Untonemap(vec3 t) {
  t = clamp(t, vec3(0.0), vec3(0.999));
  return t / ((1.0 - max(max(t.r, t.g), t.b)) * constants.exposure);
}

void main() {
  vec4 t = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
  color = vec4(Untonemap(t.rgb), t.a);
}
