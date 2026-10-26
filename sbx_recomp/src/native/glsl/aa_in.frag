#version 450
// Anti-aliasing's input (--cmaa2, --fxaa; passes.cpp, ApplyCmaa2 and
// ApplyFxaa): the view's samples (however many it has; MULTISAMPLE, or
// one with --msaa=false), each tonemapped into 0..1 and then averaged. The
// race's main view is HDR stored at 8x (its resolves' exponent bias -3;
// exposure 1/8): blended as they are, an edge against a bright pixel stays
// as bright as that pixel, so blends (the AA's, and the MSAA's own) don't
// show. aa_out.frag (and fxaa.frag) undo the tonemap.

#ifdef MULTISAMPLE
layout(set = 0, binding = 0) uniform sampler2DMS source;
#else
layout(set = 0, binding = 0) uniform sampler2D source;
#endif

layout(push_constant) uniform Constants {
  float exposure;  // the view's values times this
} constants;

layout(location = 0) out vec4 color;

vec3 Tonemap(vec3 c) {
  c = max(c, vec3(0.0)) * constants.exposure;
  return c / (1.0 + max(max(c.r, c.g), c.b));
}

void main() {
  ivec2 position = ivec2(gl_FragCoord.xy);
#ifdef MULTISAMPLE
  int samples = textureSamples(source);
  vec3 rgb = vec3(0.0);
  float alpha = 0.0;
  for (int i = 0; i < samples; i++) {
    vec4 c = texelFetch(source, position, i);
    rgb += Tonemap(c.rgb);
    alpha += c.a;
  }
  color = vec4(rgb, alpha) / float(samples);
#else
  vec4 c = texelFetch(source, position, 0);
  color = vec4(Tonemap(c.rgb), c.a);
#endif
}
