in vec2 vUV;
uniform sampler2D depthtex0;
uniform sampler2D shadowtex0;
uniform sampler3D voxeltex0;
uniform int uShadowAvailable;
uniform int uVoxelAvailable;
uniform vec3 uShadowCameraWorld;
uniform vec3 uShadowViewRight;
uniform vec3 uShadowViewUp;
uniform vec3 uShadowViewForward;
uniform vec2 uShadowOrthoHalf;
uniform vec2 uShadowNearFar;
uniform vec3 uCameraWorld;
uniform vec3 uViewRight;
uniform vec3 uViewUp;
uniform vec3 uViewForward;
uniform vec3 uSunDirectionView;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform vec2 uProjectionScale;
uniform vec2 uNearFar;
uniform float uFogEnd;
uniform vec3 uVoxelOrigin;
uniform float uVoxelSize;
out vec4 fragColor;

float linearViewZ(float depth) {
  float z = depth * 2.0 - 1.0;
  return -(2.0 * uNearFar.x * uNearFar.y) /
      max(uNearFar.y + uNearFar.x - z * (uNearFar.y - uNearFar.x), 0.0001);
}

float interleavedNoise(vec2 pixel) {
  return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

#if VL_QUALITY == 1
const int VL_STEPS = 12;
#elif VL_QUALITY == 2
const int VL_STEPS = 20;
#else
const int VL_STEPS = 32;
#endif

void main() {
  float depth = texture(depthtex0, vUV).r;
  float fogReach = max(uFogEnd, 1.0);
  vec3 dirCam = vec3((vUV * 2.0 - 1.0) / max(uProjectionScale, vec2(0.0001)), -1.0);
  float dirLen = length(dirCam);
  vec3 rayDirView = dirCam / dirLen;
  bool hasSurface = depth < 0.99999;
  float rayLength = hasSurface ? min(-linearViewZ(depth) * dirLen, fogReach) : fogReach;
#if VOLUMETRIC
  if(uShadowAvailable == 0 || uSunIntensity <= 0.0) {
    fragColor = vec4(0.0, 0.0, 0.0, rayLength);
    return;
  }
  vec3 rayWorld = uViewRight * rayDirView.x + uViewUp * rayDirView.y - uViewForward * rayDirView.z;
  float stepLength = rayLength / float(VL_STEPS);
  float dither = interleavedNoise(gl_FragCoord.xy);
  vec3 originWorld = uCameraWorld + rayWorld * (stepLength * dither);
  vec3 stepWorld = rayWorld * stepLength;
  vec3 relative = originWorld - uShadowCameraWorld;
  vec3 shadowPos = vec3(dot(relative, uShadowViewRight), dot(relative, uShadowViewUp),
                        -dot(relative, uShadowViewForward));
  vec3 shadowStep = vec3(dot(stepWorld, uShadowViewRight), dot(stepWorld, uShadowViewUp),
                         -dot(stepWorld, uShadowViewForward));
  vec2 invOrtho = 1.0 / max(uShadowOrthoHalf, vec2(0.001));
  float invRange = 1.0 / max(uShadowNearFar.y - uShadowNearFar.x, 0.001);
  bool useVoxel = uVoxelAvailable != 0;
  vec3 uvw = (originWorld - uVoxelOrigin) / uVoxelSize;
  vec3 uvwStep = stepWorld / uVoxelSize;
  float lit = 0.0;
  for(int i = 0; i < VL_STEPS; ++i) {
    vec2 projected = shadowPos.xy * invOrtho;
    float currentDepth = (-shadowPos.z - uShadowNearFar.x) * invRange;
    float visibility = 1.0;
    if(currentDepth > 0.0 && currentDepth < 1.0 && all(lessThan(abs(projected), vec2(1.0)))) {
      visibility = currentDepth - 0.0015 <= texture(shadowtex0, projected * 0.5 + 0.5).r ? 1.0 : 0.0;
    }
    if(useVoxel && all(greaterThanEqual(uvw, vec3(0.0))) && all(lessThanEqual(uvw, vec3(1.0)))) {
      visibility *= 1.0 - texture(voxeltex0, uvw).r;
    }
    lit += visibility;
    shadowPos += shadowStep;
    uvw += uvwStep;
  }
  lit /= float(VL_STEPS);
  float mu = dot(rayDirView, normalize(uSunDirectionView));
  float g = 0.62;
  float phase = 0.25 * (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5);
  float density = 1.0 - exp(-rayLength * 0.012);
  fragColor = vec4(uSunColor * (uSunIntensity * lit * phase * density), rayLength);
#else
  fragColor = vec4(0.0, 0.0, 0.0, rayLength);
#endif
}
