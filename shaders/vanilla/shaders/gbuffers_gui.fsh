#version 430 core
#include "/lib/common.glsl"
// Untextured GUI: white texel * colour. No lightmap, no fog.
in vec2 texcoord;
in vec4 color;
uniform sampler2D gtexture;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 vec4 surface = texture(gtexture, texcoord) * color;
 if(surface.a < alphaTestRef) discard;
 outColor = surface;
}
