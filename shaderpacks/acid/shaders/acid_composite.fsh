in vec2 vUV;
uniform sampler2D colortex0;
uniform sampler2D depthtex0;
uniform float uWorldTime;
uniform float uPartialTicks;
uniform vec2 uViewport;
uniform int uDepthAvailable;
out vec4 fragColor;

vec3 hueRotate(vec3 color, float angle) {
  float c = cos(angle);
  float s = sin(angle);
  mat3 rotation = mat3(
    0.299 + 0.701 * c + 0.168 * s, 0.587 - 0.587 * c + 0.330 * s, 0.114 - 0.114 * c - 0.497 * s,
    0.299 - 0.299 * c - 0.328 * s, 0.587 + 0.413 * c + 0.035 * s, 0.114 - 0.114 * c + 0.292 * s,
    0.299 - 0.300 * c + 1.250 * s, 0.587 - 0.588 * c - 1.050 * s, 0.114 + 0.886 * c - 0.203 * s);
  return rotation * color;
}

void main() {
  float phase = (uWorldTime + uPartialTicks) * ACID_SPEED * 0.035;
  float shift = ACID_ABERRATION * 0.0015 * sin(phase * 1.7 + vUV.y * 18.0);
  vec2 offset = vec2(shift / max(uViewport.x / max(uViewport.y, 1.0), 1.0), 0.0);
  vec3 color;
  color.r = texture(colortex0, vUV + offset).r;
  color.g = texture(colortex0, vUV).g;
  color.b = texture(colortex0, vUV - offset).b;
  float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
  vec3 rotated = hueRotate(color, phase + luminance * 2.0);
  float rotatedLuminance = max(dot(rotated, vec3(0.2126, 0.7152, 0.0722)), 1e-4);
  rotated *= luminance / rotatedLuminance;
  vec3 saturated = vec3(dot(rotated, vec3(0.2126, 0.7152, 0.0722)));
  rotated = mix(saturated, rotated, ACID_SATURATION);
  float depth = uDepthAvailable == 1 ? texture(depthtex0, vUV).r : 1.0;
  float distanceTint = (1.0 - depth) * ACID_INTENSITY;
  rotated = mix(rotated, hueRotate(rotated, 1.4), distanceTint);
#if ACID_SCANLINES
  rotated *= 0.94 + 0.06 * sin(vUV.y * uViewport.y * 3.14159265);
#endif
  fragColor = vec4(mix(color, rotated, ACID_INTENSITY), 1.0);
}
