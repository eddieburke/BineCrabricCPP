#version 430 core
#include "/lib/common.glsl"
in vec2 lmcoord;
in vec4 color;
in float viewDistance;
in float horizontalDistance;
uniform sampler2D lightmap;
uniform float clipDistance;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 if(horizontalDistance < clipDistance) discard;
 vec3 surface = color.rgb * texture(lightmap, lmcoord).rgb;
 outColor = vec4(applyFog(surface, viewDistance), color.a);
}
