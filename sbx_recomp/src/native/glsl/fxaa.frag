#version 450
// FXAA (after Timothy Lottes' FXAA 3.11 "quality" algorithm, written for
// this renderer) on the 3D view (--fxaa, ApplyFxaa in passes.cpp): `source`
// is the main view's samples tonemapped into 0..1 and averaged
// (aa_in.frag); the result is drawn back into every sample with the tonemap
// undone (as aa_out.frag). The alpha is kept.

layout(set = 0, binding = 0) uniform sampler2D source;  // linear, clamped

layout(push_constant) uniform Constants {
  vec2 texel;  // 1 / the source's size
  int debug;   // --aa_debug: the pixels FXAA blends drawn red
  float exposure;  // as aa_in.frag's: the view's values times this tonemapped
} constants;

layout(location = 0) out vec4 color;

// Edge thresholds (FXAA 3.11's "high quality" defaults) and how much of the
// sub-pixel aliasing is taken out.
const float kEdgeThreshold = 0.125;
const float kEdgeThresholdMin = 0.0312;
const float kSubpixel = 0.75;
const int kSteps = 12;
const float kStepSizes[kSteps] = float[](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0,
                                         8.0);

// Luma of the tonemapped colour, roughly gamma-compressed (as FXAA's).
float Luma(vec3 rgb) { return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114))); }

vec3 Untonemap(vec3 t) {
  t = clamp(t, vec3(0.0), vec3(0.999));
  return t / ((1.0 - max(max(t.r, t.g), t.b)) * constants.exposure);
}

float LumaAt(vec2 uv) { return Luma(texture(source, uv).rgb); }

void main() {
  vec2 texel = constants.texel;
  vec2 uv = gl_FragCoord.xy * texel;
  vec4 center = texture(source, uv);
  float m = Luma(center.rgb);
  float n = Luma(textureOffset(source, uv, ivec2(0, -1)).rgb);
  float s = Luma(textureOffset(source, uv, ivec2(0, 1)).rgb);
  float w = Luma(textureOffset(source, uv, ivec2(-1, 0)).rgb);
  float e = Luma(textureOffset(source, uv, ivec2(1, 0)).rgb);
  float high = max(m, max(max(n, s), max(w, e)));
  float low = min(m, min(min(n, s), min(w, e)));
  float range = high - low;
  if (range < max(kEdgeThresholdMin, high * kEdgeThreshold)) {
    color = vec4(Untonemap(center.rgb), center.a);
    return;
  }
  float nw = Luma(textureOffset(source, uv, ivec2(-1, -1)).rgb);
  float ne = Luma(textureOffset(source, uv, ivec2(1, -1)).rgb);
  float sw = Luma(textureOffset(source, uv, ivec2(-1, 1)).rgb);
  float se = Luma(textureOffset(source, uv, ivec2(1, 1)).rgb);

  // Sub-pixel blend: how far the centre is from its neighbourhood's average.
  float average = (2.0 * (n + s + w + e) + (nw + ne + sw + se)) / 12.0;
  float subpixel = clamp(abs(average - m) / range, 0.0, 1.0);
  subpixel = smoothstep(0.0, 1.0, subpixel);
  subpixel = subpixel * subpixel * kSubpixel;

  // The edge's direction, and which side of the centre it runs on.
  float horizontal = abs(nw + ne - 2.0 * n) + 2.0 * abs(w + e - 2.0 * m) + abs(sw + se - 2.0 * s);
  float vertical = abs(nw + sw - 2.0 * w) + 2.0 * abs(n + s - 2.0 * m) + abs(ne + se - 2.0 * e);
  bool is_horizontal = horizontal >= vertical;
  float side1 = is_horizontal ? n : w;
  float side2 = is_horizontal ? s : e;
  float gradient1 = abs(side1 - m), gradient2 = abs(side2 - m);
  float step_length = is_horizontal ? texel.y : texel.x;
  float local;
  if (gradient1 >= gradient2) {
    step_length = -step_length;
    local = 0.5 * (side1 + m);
  } else {
    local = 0.5 * (side2 + m);
  }
  float gradient = 0.25 * max(gradient1, gradient2);

  // Along the edge (half a pixel over, between the two rows) both ways
  // until the luma leaves it.
  vec2 edge = uv;
  vec2 along;
  if (is_horizontal) {
    edge.y += 0.5 * step_length;
    along = vec2(texel.x, 0.0);
  } else {
    edge.x += 0.5 * step_length;
    along = vec2(0.0, texel.y);
  }
  vec2 uv1 = edge - along, uv2 = edge + along;
  float end1 = LumaAt(uv1) - local, end2 = LumaAt(uv2) - local;
  bool done1 = abs(end1) >= gradient, done2 = abs(end2) >= gradient;
  for (int i = 1; i < kSteps && !(done1 && done2); i++) {
    if (!done1) {
      uv1 -= along * kStepSizes[i];
      end1 = LumaAt(uv1) - local;
      done1 = abs(end1) >= gradient;
    }
    if (!done2) {
      uv2 += along * kStepSizes[i];
      end2 = LumaAt(uv2) - local;
      done2 = abs(end2) >= gradient;
    }
  }
  float distance1 = is_horizontal ? uv.x - uv1.x : uv.y - uv1.y;
  float distance2 = is_horizontal ? uv2.x - uv.x : uv2.y - uv.y;
  bool nearer1 = distance1 < distance2;
  float distance = min(distance1, distance2);
  float span = distance1 + distance2;
  // Blend only if the nearer end's luma moves the way the centre's does.
  bool centre_lower = m - local < 0.0;
  bool good = ((nearer1 ? end1 : end2) < 0.0) != centre_lower;
  float offset = good ? 0.5 - distance / span : 0.0;
  offset = max(offset, subpixel);
  vec2 final_uv = uv;
  if (is_horizontal) {
    final_uv.y += offset * step_length;
  } else {
    final_uv.x += offset * step_length;
  }
  color = vec4(Untonemap(texture(source, final_uv).rgb), center.a);
  if (constants.debug != 0) {
    color = vec4(1.0 / constants.exposure, 0.0, 0.0, center.a);  // red, at the view's scale
  }
}
