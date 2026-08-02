#version 430 core
#include "/lib/common.glsl"
// Unlit textured: albedo * vertexColour * fog. No lightmap.
in vec2 texcoord;
in vec4 color;
in float viewDistance;
uniform sampler2D gtexture;
uniform vec4 entityColor;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 vec4 surface = texture(gtexture, texcoord) * color;
 if(surface.a < alphaTestRef) discard;
 surface.rgb = mix(surface.rgb, entityColor.rgb, entityColor.a);
 outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
}
