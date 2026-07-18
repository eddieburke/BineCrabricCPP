in vec2 vUV;
uniform sampler2D colortex0;
uniform sampler2D depthtex0;
uniform sampler2D lightData0;
uniform sampler2D lightData1;
uniform usampler2D lightTileGrid;
uniform usampler2D lightTileIndices;
uniform sampler2D shadowtex0;
uniform vec3 uViewRight;
uniform vec3 uViewUp;
uniform vec3 uViewForward;
uniform vec3 uCameraWorld;
uniform vec3 uShadowCameraWorld;
uniform vec3 uShadowViewRight;
uniform vec3 uShadowViewUp;
uniform vec3 uShadowViewForward;
uniform vec3 uSunDirectionView;
uniform vec3 uSunColor;
uniform vec2 uProjectionScale;
uniform vec2 uNearFar;
uniform vec2 uViewport;
uniform vec2 uLightDataSize;
uniform vec2 uLightTileCount;
uniform vec2 uLightClusterParams;
uniform vec2 uLightIndexSize;
uniform vec2 uShadowOrthoHalf;
uniform vec2 uShadowNearFar;
uniform float uLightTileSize;
uniform int uLightClusterSlices;
uniform int uShadowAvailable;
uniform float uSunIntensity;
uniform float uWorldTime;
uniform int lightCount;
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

vec4 lightTexel(sampler2D source, int index) {
  int width = max(1, int(uLightDataSize.x));
  return texelFetch(source, ivec2(index % width, index / width), 0);
}

int tileLightIndex(int scalarIndex) {
  int texelIndex = scalarIndex / 4;
  int width = max(1, int(uLightIndexSize.x));
  uvec4 packedIndices = texelFetch(lightTileIndices, ivec2(texelIndex % width, texelIndex / width), 0);
  return int(packedIndices[scalarIndex % 4]);
}

vec3 lightViewPosition(vec3 relativePosition) {
  return vec3(dot(relativePosition, uViewRight), dot(relativePosition, uViewUp),
              -dot(relativePosition, uViewForward));
}

float attenuation(vec3 point, vec3 lightPosition, float radius) {
  float ratio = length(lightPosition - point) / max(radius, 0.001);
  float edge = max(0.0, 1.0 - ratio * ratio);
  return edge * edge;
}

vec3 viewToWorldRelative(vec3 point) {
  return uViewRight * point.x + uViewUp * point.y - uViewForward * point.z;
}

float sunShadow(vec3 point, vec3 normal) {
#if SHADOW_QUALITY == 0
  return 1.0;
#else
  if(uShadowAvailable == 0) {
    vec3 sunDirection = normalize(uSunDirectionView);
    if(dot(normal, sunDirection) <= 0.02) {
      return 1.0;
    }
    float visibility = 1.0;
    float maxDistance = SHADOW_QUALITY == 1 ? 7.0 : 14.0;
    int steps = SHADOW_QUALITY == 1 ? 5 : 9;
    vec3 origin = point + normal * 0.08;
    for(int i = 1; i <= 9; ++i) {
      if(i > steps) {
        break;
      }
      float t = (float(i) / float(steps)) * maxDistance;
      vec3 samplePosition = origin + sunDirection * t;
      float sampleViewDepth = -samplePosition.z;
      if(sampleViewDepth <= uNearFar.x) {
        break;
      }
      vec2 sampleUv = (samplePosition.xy * uProjectionScale / sampleViewDepth) * 0.5 + 0.5;
      if(any(lessThanEqual(sampleUv, vec2(0.001))) || any(greaterThanEqual(sampleUv, vec2(0.999)))) {
        break;
      }
      float sceneDepth = texture(depthtex0, sampleUv).r;
      if(sceneDepth >= 0.99999) {
        continue;
      }
      float sceneViewDepth = -linearViewZ(sceneDepth);
      float separation = sampleViewDepth - sceneViewDepth;
      float thickness = 0.22 + t * 0.025;
      if(separation > 0.035 && separation < thickness) {
        visibility = 0.28;
        break;
      }
    }
    return visibility;
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
  float facing = max(dot(normal, normalize(uSunDirectionView)), 0.0);
  float bias = mix(0.0016, 0.00035, facing);
#if SHADOW_QUALITY == 1
  float visibility = currentDepth - bias <= texture(shadowtex0, uv).r ? 1.0 : 0.0;
#else
  vec2 texel = 1.0 / vec2(textureSize(shadowtex0, 0));
  float visibility = 0.0;
  float samples = 0.0;
  for(int y = -2; y <= 2; ++y) {
    for(int x = -2; x <= 2; ++x) {
#if SHADOW_QUALITY == 2
      if(abs(x) > 1 || abs(y) > 1) {
        continue;
      }
#endif
      float storedDepth = texture(shadowtex0, uv + vec2(x, y) * texel).r;
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

void unifiedLighting(vec3 point, vec3 normal, bool hasSurface, out vec3 direct, out vec3 volume) {
  direct = vec3(0.0);
  volume = vec3(0.0);
  ivec2 tileCount = max(ivec2(1), ivec2(uLightTileCount));
  ivec2 tile = clamp(ivec2(gl_FragCoord.xy / max(uLightTileSize, 1.0)), ivec2(0), tileCount - 1);
  float viewDepth = max(-point.z, uNearFar.x);
  int clusterSlice = clamp(int(log2(viewDepth) * uLightClusterParams.x + uLightClusterParams.y),
                           0, max(0, uLightClusterSlices - 1));
  tile.y += clusterSlice * tileCount.y;
  uvec4 range = texelFetch(lightTileGrid, tile, 0);
  int offset = int(range.x);
  int count = int(range.y);
  float rayLength = min(length(point), uNearFar.y);
  vec3 rayDirection = point / max(length(point), 0.0001);
#if LOCAL_LIGHTING
  for(int localIndex = 0; localIndex < count; ++localIndex) {
    int index = tileLightIndex(offset + localIndex);
    if(index < 0 || index >= lightCount) {
      continue;
    }
    vec4 encodedPosition = lightTexel(lightData0, index);
    vec4 encodedColor = lightTexel(lightData1, index);
    vec3 lightPosition = lightViewPosition(encodedPosition.xyz);
    float surfaceAttenuation = attenuation(point, lightPosition, encodedPosition.w);
    if(hasSurface && surfaceAttenuation > 0.0) {
      vec3 toLight = normalize(lightPosition - point);
      float diffuse = max(dot(normal, toLight), 0.0);
      direct += encodedColor.rgb * encodedColor.a * surfaceAttenuation * (0.14 + diffuse * 0.86);
    }
#if VOLUMETRIC_QUALITY > 0
    float closestT = clamp(dot(lightPosition, rayDirection), 0.0, rayLength);
    float closestDistance = length(lightPosition - rayDirection * closestT);
    float radius = max(encodedPosition.w, 0.001);
    float chord = 2.0 * sqrt(max(radius * radius - closestDistance * closestDistance, 0.0));
    float density = attenuation(rayDirection * closestT, lightPosition, radius);
    volume += encodedColor.rgb * encodedColor.a * density * chord / radius;
#endif
  }
#endif
#if VOLUMETRIC_QUALITY > 0
  float sunPhase = pow(max(dot(rayDirection, normalize(uSunDirectionView)), 0.0), 24.0);
  volume += uSunColor * uSunIntensity * sunPhase * min(rayLength / 96.0, 1.0) * 0.12;
  volume *= 0.006 * float(VOLUMETRIC_QUALITY);
#endif
}

vec3 bloom(vec2 uv) {
#if BLOOM
  vec2 texel = 1.0 / max(uViewport, vec2(1.0));
  vec3 sum = vec3(0.0);
  float weight = 0.0;
  for(int y = -2; y <= 2; ++y) {
    for(int x = -2; x <= 2; ++x) {
      float sampleWeight = 1.0 / (1.0 + float(x * x + y * y));
      vec3 sampleColor = texture(colortex0, uv + vec2(x, y) * texel * 3.0).rgb;
      float sampleLuminance = dot(sampleColor, vec3(0.2126, 0.7152, 0.0722));
      float brightness = max(sampleLuminance - 1.0, 0.0) / max(sampleLuminance, 0.0001);
      sum += sampleColor * brightness * sampleWeight;
      weight += sampleWeight;
    }
  }
  return sum / max(weight, 0.0001);
#else
  return vec3(0.0);
#endif
}

vec3 displayMap(vec3 value) {
  value = max(value, vec3(0.0));
  vec3 shoulder = vec3(0.9) + vec3(0.1) * (vec3(1.0) - exp(-max(value - vec3(0.9), vec3(0.0)) * 2.0));
  return clamp(mix(value, shoulder, step(vec3(0.9), value)), 0.0, 1.0);
}

void main() {
  vec4 source = texture(colortex0, vUV);
  float depth = texture(depthtex0, vUV).r;
  vec3 position = viewPosition(vUV, depth);
  vec3 normal = normalize(cross(dFdx(position), dFdy(position)));
  if(normal.z < 0.0) {
    normal = -normal;
  }
  vec3 direct;
  vec3 volume;
  bool hasSurface = depth < 0.99999;
  unifiedLighting(position, normal, hasSurface, direct, volume);
  float shadow = hasSurface ? sunShadow(position, normal) : 1.0;
  float sunFacing = max(dot(normal, normalize(uSunDirectionView)), 0.0);
  float sunWeight = uSunIntensity * smoothstep(0.02, 0.45, sunFacing);
  float sunGrade = mix(1.0, mix(0.72, 1.02, shadow), sunWeight);
  vec3 base = source.rgb * sunGrade;
  vec3 localHeadroom = max(vec3(1.15) - base, vec3(0.0));
  vec3 radiance = base + direct * localHeadroom * 0.08 + volume + bloom(vUV) * 0.10;
  fragColor = vec4(displayMap(radiance * EXPOSURE), source.a);
}
