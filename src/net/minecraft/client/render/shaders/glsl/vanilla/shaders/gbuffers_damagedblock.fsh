#version 430 core
#include "/lib/common.glsl"
in vec2 texcoord;
in vec2 lmcoord;
in vec4 color;
in vec3 normal;
in float viewDistance;
uniform sampler2D gtexture;
uniform sampler2D lightmap;
uniform mat4 gbufferModelViewInverse;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 vec4 tex = texture(gtexture, texcoord);
 if(tex.a < alphaTestRef) discard;
 outColor = tex;
}
