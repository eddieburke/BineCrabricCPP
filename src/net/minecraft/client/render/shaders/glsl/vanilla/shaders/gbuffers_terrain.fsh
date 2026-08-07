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
 // tintAndAo: separateAo puts AO in alpha; otherwise a=1 and rgb already has AO.
 vec4 surface = vec4(tex.rgb * tintAndAo(color), tex.a);
 if(surface.a < alphaTestRef) discard;
 vec3 worldNormal = normalize(mat3(gbufferModelViewInverse) * normal);
 surface.rgb *= faceShade(worldNormal);
 surface.rgb *= texture(lightmap, lmcoord).rgb;
 outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
}
