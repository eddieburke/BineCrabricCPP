in vec2 vUV;
uniform sampler2D colortex0;
uniform vec2 uViewport;
out vec4 fragColor;

void main() {
  vec2 texel = 1.0 / max(uViewport, vec2(1.0));
  vec3 sum = vec3(0.0);
  float weight = 0.0;
  for(int y = -1; y <= 1; ++y) {
    for(int x = -1; x <= 1; ++x) {
      float sampleWeight = 1.0 / (1.0 + float(x * x + y * y) * 2.0);
      vec3 sampleColor = texture(colortex0, vUV + vec2(x, y) * texel * 3.0).rgb;
      float luminance = dot(sampleColor, vec3(0.2126, 0.7152, 0.0722));
      float brightness = smoothstep(0.62, 1.05, luminance);
      float warmEmissive = max(0.0, sampleColor.r - max(sampleColor.g, sampleColor.b) * 1.05) *
                           smoothstep(0.30, 0.50, sampleColor.r);
      float coolEmissive = max(0.0, sampleColor.b * 0.6 + sampleColor.r * 0.4 - sampleColor.g * 1.05) *
                           smoothstep(0.26, 0.44, sampleColor.b);
      float factor = max(brightness, max(warmEmissive * 2.0, coolEmissive * 2.0));
      sum += sampleColor * factor * sampleWeight;
      weight += sampleWeight;
    }
  }
  fragColor = vec4(sum / max(weight, 0.0001), 1.0);
}
