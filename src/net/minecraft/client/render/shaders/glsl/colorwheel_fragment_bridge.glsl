#ifndef _CLRWL_MERGE_F
#define _CLRWL_MERGE_F
struct FlwMaterial {
 bool blur;
 bool mipmap;
 bool backfaceCulling;
 bool polygonOffset;
 uint depthTest;
 uint transparency;
 uint writeMask;
 bool useOverlay;
 bool useLight;
 uint cardinalLightingMode;
 bool ambientOcclusion;
};
in ClrwlVertexData {
 vec4 flw_vertexPos;
 vec4 flw_vertexColor;
 vec2 flw_vertexTexCoord;
 flat ivec2 flw_vertexOverlay;
 vec2 flw_vertexLight;
 vec3 flw_vertexNormal;
 vec4 clrwl_vertexTangent;
};
#define clrwl_vertexPos flw_vertexPos
#define clrwl_vertexColor flw_vertexColor
#define clrwl_vertexTexCoord flw_vertexTexCoord
#define clrwl_vertexOverlay flw_vertexOverlay
#define clrwl_vertexLight flw_vertexLight
#define clrwl_vertexNormal flw_vertexNormal
vec4 flw_sampleColor;
vec4 flw_fragColor;
ivec2 flw_fragOverlay;
vec2 flw_fragLight;
FlwMaterial flw_material;
vec4 clrwl_overlayColor = vec4(0.0);
uniform sampler2D flw_diffuseTex;
uniform sampler2D flw_overlayTex;
void flw_materialFragment() {}
void flw_shaderLight() {}
CLRWL_DISCARD_FUNCTIONS
void clrwl_getDebugColor(inout vec4 color) {}
void _clrwl_materialFragment_hook() {
 flw_materialFragment();
 if (flw_material.useOverlay) {
  clrwl_overlayColor = texelFetch(flw_overlayTex, flw_fragOverlay, 0);
  clrwl_overlayColor.a = 1.0 - clrwl_overlayColor.a;
 }
}
void _clrwl_shaderLight_hook() { flw_shaderLight(); }
void clrwl_computeFragment(vec4 sampleColor, out vec4 fragColor, out vec2 fragLight, out float ao, out vec4 fragOverlay) {
 flw_material.blur = false;
 flw_material.mipmap = true;
 flw_material.backfaceCulling = true;
 flw_material.polygonOffset = false;
 flw_material.depthTest = 4u;
 flw_material.transparency = 0u;
 flw_material.writeMask = 0u;
 flw_material.useOverlay = false;
 flw_material.useLight = true;
 flw_material.cardinalLightingMode = 0u;
 flw_material.ambientOcclusion = true;
 flw_sampleColor = sampleColor;
 flw_fragColor = flw_sampleColor * flw_vertexColor;
 flw_fragLight = flw_vertexLight;
 flw_fragOverlay = flw_vertexOverlay;
 _clrwl_materialFragment_hook();
 vec4 fragColorBfLight = flw_fragColor;
 _clrwl_shaderLight_hook();
 if (flw_material.ambientOcclusion) {
  vec3 fragColorLightRatio = flw_fragColor.rgb / fragColorBfLight.rgb;
  ao = clamp(max(max(fragColorLightRatio.r, fragColorLightRatio.g), fragColorLightRatio.b), 0.0, 1.0);
 } else {
  ao = 1.0;
 }
 clrwl_computeDiscard(flw_fragColor);
 clrwl_getDebugColor(flw_fragColor);
 fragColor = flw_fragColor;
 fragLight = flw_fragLight + 1.0 / 32.0;
 fragOverlay = clrwl_overlayColor;
}
#endif
