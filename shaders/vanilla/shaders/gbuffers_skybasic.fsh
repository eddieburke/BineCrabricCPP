#version 430 core
#include "/lib/common.glsl"
in vec4 color;
in float viewDistance;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 // The sky is its own light source (never lightmapped), but vanilla draws the
 // sky dome with the world fog enabled so the horizon fades into the fog colour
 // and meets the fogged terrain seamlessly.
 outColor = vec4(applyFog(color.rgb, viewDistance), color.a);
}
