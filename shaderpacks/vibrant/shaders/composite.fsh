in vec2 vUV;
uniform sampler2D colortex0;
uniform sampler2D depthtex0;
uniform sampler2D bloomtex;
uniform sampler2D vltex;
uniform vec3 uShadowCameraWorld;
uniform vec3 uShadowViewRight;
uniform vec3 uShadowViewUp;
uniform vec3 uShadowViewForward;
uniform vec3 uCameraWorld;
uniform vec2 uShadowOrthoHalf;
uniform vec2 uShadowNearFar;
uniform float uShadowTexelSize;
uniform int uShadowAvailable;
uniform vec3 uViewRight;
uniform vec3 uViewUp;
uniform vec3 uViewForward;
uniform vec3 uSunDirectionView;
uniform vec3 uSunColor;
uniform vec2 uProjectionScale;
uniform vec2 uNearFar;
uniform vec2 uViewport;
uniform float uSunIntensity;
uniform float uFogEnd;
out vec4 fragColor;

float linearViewZ(float depth) {
  float z = depth * 2.0 - 1.0;
  return -(2.0 * uNearFar.x * uNearFar.y) /
      max(uNearFar.y + uNearFar.x - z * (uNearFar.y - uNearFar.x), 0.0001);
}

vec3 viewPosition(vec2 uv, float depth) {
  float z = linearViewZ(depth);
  return vec3((uv * 2.0 - 1.0) * (-z) / max(uProjectionScale, vec2(0.0001)), z);
}

vec3 reconstructNormal(vec3 position) {
  return normalize(cross(dFdx(position), dFdy(position)));
}

vec3 viewToWorldRelative(vec3 point) {
  return uViewRight * point.x + uViewUp * point.y - uViewForward * point.z;
}

float sunShadow(vec3 point, vec3 normal) {
#if SHADOW_QUALITY == 0
  return 1.0;
#else
  if(uShadowAvailable == 0) {
    return 1.0;
  }
  float facing = dot(normal, normalize(uSunDirectionView));
  if(facing <= 0.0) {
    return smoothstep(-0.18, 0.04, facing);
  }
  vec3 worldPosition = uCameraWorld + viewToWorldRelative(point);
  vec3 relative = worldPosition - uShadowCameraWorld;
  vec3 shadowPosition = vec3(dot(relative, uShadowViewRight), dot(relative, uShadowViewUp),
                             -dot(relative, uShadowViewForward));
  vec2 projected = shadowPosition.xy / max(uShadowOrthoHalf, vec2(0.001));
  float currentDepth = (-shadowPosition.z - uShadowNearFar.x) /
                       max(uShadowNearFar.y - uShadowNearFar.x, 0.001);
  if(currentDepth <= 0.0 || currentDepth >= 1.0 || any(greaterThanEqual(abs(projected), vec2(1.0)))) {
    return 1.0;
  }
  vec2 uv = projected * 0.5 + 0.5;
  float bias = mix(0.0025, 0.0006, facing);
#if SHADOW_QUALITY == 1
  float visibility = 1.0;
#else
  float visibility = 0.0;
  float samples = 0.0;
  for(int y = -2; y <= 2; ++y) {
    for(int x = -2; x <= 2; ++x) {
#if SHADOW_QUALITY == 2
      if(abs(x) > 1 || abs(y) > 1) {
        continue;
      }
#endif
      float storedDepth = 1.0;
      visibility += currentDepth - bias <= storedDepth ? 1.0 : 0.0;
      samples += 1.0;
    }
  }
  visibility /= max(samples, 1.0);
#endif
  float edgeFade = smoothstep(0.82, 0.98, max(abs(projected.x), abs(projected.y)));
  return mix(visibility, 1.0, edgeFade);
#endif
}

#if VOLUMETRIC
vec3 volumetricUpsample(float rayLength, float fogReach) {
  vec2 texel = 2.0 / max(uViewport, vec2(1.0));
  vec3 sum = vec3(0.0);
  float weightSum = 0.0;
  for(int i = 0; i < 4; ++i) {
    vec2 offset = vec2(i == 0 || i == 3 ? -0.5 : 0.5, i < 2 ? -0.5 : 0.5) * texel;
    vec4 sampleValue = texture(vltex, vUV + offset);
    float weight = 1.0 / (0.02 + abs(sampleValue.a - rayLength) / fogReach);
    sum += sampleValue.rgb * weight;
    weightSum += weight;
  }
  return sum / max(weightSum, 0.0001);
}
#endif

vec3 tonemap(vec3 value) {
  value = max(value, vec3(0.0));
  vec3 shoulder = vec3(0.88) + vec3(0.12) * (vec3(1.0) - exp(-max(value - vec3(0.88), vec3(0.0)) * 2.2));
  return clamp(mix(value, shoulder, step(vec3(0.88), value)), 0.0, 1.0);
}

void main() {
  vec4 source = texture(colortex0, vUV);
  float depth = texture(depthtex0, vUV).r;
  bool hasSurface = depth < 0.99999;
  vec3 radiance = source.rgb * EXPOSURE;

  float fogReach = max(uFogEnd, 1.0);
  float surfaceBlend = hasSurface
      ? 1.0 - smoothstep(fogReach * 0.55, fogReach * 0.98, -linearViewZ(depth))
      : 0.0;

  vec3 rayDirView = normalize(vec3((vUV * 2.0 - 1.0) / max(uProjectionScale, vec2(0.0001)), -1.0));
  vec3 position = hasSurface ? viewPosition(vUV, depth) : vec3(0.0);
  float rayLength = hasSurface ? min(length(position), fogReach) : fogReach;

  if(hasSurface && uSunIntensity > 0.0) {
    vec3 normal = reconstructNormal(position);
    float facing = max(dot(normal, normalize(uSunDirectionView)), 0.0);
    float shadow = sunShadow(position, normal);
    vec3 warmth = mix(vec3(1.0), normalize(uSunColor + vec3(0.001)) * 1.732, 0.35);
    float lit = smoothstep(0.05, 0.65, facing) * uSunIntensity * shadow;
    vec3 grade = mix(vec3(0.78), warmth * 1.08, lit);
    radiance *= mix(vec3(1.0), grade, SUN_GRADE * surfaceBlend);
    float occlusion = mix(1.0 - SHADOW_DARKNESS, 1.0, shadow);
    radiance *= mix(1.0, occlusion, uSunIntensity * surfaceBlend);
  }

  if(hasSurface && HAZE > 0.0) {
    float surfaceDistance = length(position);
    float hazeAmount = HAZE * (1.0 - exp(-max(surfaceDistance - fogReach * 0.12, 0.0) * (2.4 / fogReach)));
    float mu = max(dot(rayDirView, normalize(uSunDirectionView)), 0.0);
    vec3 hazeTint = mix(vec3(0.62, 0.72, 0.88), normalize(uSunColor + vec3(0.001)) * 1.732,
                        pow(mu, 6.0) * 0.6 * uSunIntensity);
    vec3 hazeColor = hazeTint * (0.25 + 0.75 * uSunIntensity) * EXPOSURE;
    radiance = mix(radiance, hazeColor, clamp(hazeAmount, 0.0, 0.9));
  }

#if VOLUMETRIC
  radiance += volumetricUpsample(rayLength, fogReach) * VL_STRENGTH;
#endif

#if BLOOM
  radiance += texture(bloomtex, vUV).rgb * BLOOM_STRENGTH;
#endif

  float luminance = dot(radiance, vec3(0.2126, 0.7152, 0.0722));
  radiance = mix(vec3(luminance), radiance, SATURATION);
  radiance = (radiance - vec3(0.5)) * CONTRAST + vec3(0.5);

  vec2 centered = vUV - vec2(0.5);
  float vignette = 1.0 - VIGNETTE * smoothstep(0.25, 0.75, dot(centered, centered) * 2.0);
  radiance *= vignette;

  fragColor = vec4(tonemap(radiance), source.a);
}
