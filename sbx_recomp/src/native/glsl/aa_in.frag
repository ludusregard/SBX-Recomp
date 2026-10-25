#version 450
// Anti-aliasing's input (--cmaa2, --fxaa; passes.cpp, ApplyCmaa2 and
// ApplyFxaa): the main view's two samples, each tonemapped into 0..1 and
// then averaged. The main view is HDR, stored at 8x (the resolves'
// exponent bias -3): blended as it is, an edge against a bright pixel stays
// as bright as that pixel, so blends (the AA's, and the 2x MSAA's own) don't
// show. aa_out.frag (and fxaa.frag) undo the tonemap.

layout(set = 0, binding = 0) uniform sampler2DMS source;

layout(location = 0) out vec4 color;

const float kExposure = 0.125;

vec3 Tonemap(vec3 c) {
  c = max(c, vec3(0.0)) * kExposure;
  return c / (1.0 + max(max(c.r, c.g), c.b));
}

void main() {
  ivec2 position = ivec2(gl_FragCoord.xy);
  vec4 a = texelFetch(source, position, 0);
  vec4 b = texelFetch(source, position, 1);
  color = vec4(0.5 * (Tonemap(a.rgb) + Tonemap(b.rgb)), 0.5 * (a.a + b.a));
}
