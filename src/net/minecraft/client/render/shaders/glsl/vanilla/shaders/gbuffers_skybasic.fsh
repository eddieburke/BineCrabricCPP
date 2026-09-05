#version 430 core
#include "/lib/common.glsl"
in vec4 color;
in vec3 viewPosition;
uniform int renderStage;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 float viewDistance = fogCoord(viewPosition);
 bool foggedSky = renderStage == MC_RENDER_STAGE_SKY || renderStage == MC_RENDER_STAGE_VOID;
 outColor = vec4(foggedSky ? applyFog(color.rgb, viewDistance) : color.rgb, color.a);
}
