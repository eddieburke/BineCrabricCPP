#version 430 core
#include "/lib/common.glsl"
// Iris gbuffers_line: outline / fishing line. Colour + fog; never lightmapped.
// Block selection uses MC_RENDER_STAGE_OUTLINE (engine sets renderStage=14).
in vec4 color;
in float viewDistance;
uniform vec4 entityColor;
uniform int renderStage;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 vec4 surface = color;
 if(surface.a < alphaTestRef) discard;
 surface.rgb = mix(surface.rgb, entityColor.rgb, entityColor.a);
 // Selection outline stays the authored colour; fishing line still fog-blends.
 if(renderStage == 14) {
  outColor = surface;
 } else {
  outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
 }
}
