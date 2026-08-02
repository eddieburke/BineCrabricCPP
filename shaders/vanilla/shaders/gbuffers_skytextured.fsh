#version 430 core
in vec2 texcoord;
in vec4 color;
uniform sampler2D gtexture;
uniform float alphaTestRef;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 // Sun and moon emit their own light, so no lightmap and no fog here.
 vec4 surface = texture(gtexture, texcoord) * color;
 if(surface.a < alphaTestRef) discard;
 outColor = surface;
}
