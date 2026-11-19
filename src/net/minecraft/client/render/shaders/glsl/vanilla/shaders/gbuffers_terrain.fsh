#version 430 core
#include "/lib/common.glsl"
#include "/lib/lod_transition.glsl"
in vec2 texcoord;
in vec3 vertexLight;
in vec4 color;
in vec3 normal;
in float viewDistance;
in vec2 horizontalPosition;
uniform sampler2D gtexture;
uniform mat4 gbufferModelViewInverse;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 if(lodCoverage(horizontalPosition).r > 0.5 && lodDither() < lodBlend(horizontalPosition)) discard;
 vec4 tex = texture(gtexture, texcoord);
 // tintAndAo: separateAo puts AO in alpha; otherwise a=1 and rgb already has AO.
 vec4 surface = vec4(tex.rgb * tintAndAo(color), tex.a);
 if(surface.a < alphaTestRef) discard;
 vec3 worldNormal = normalize(mat3(gbufferModelViewInverse) * normal);
 surface.rgb *= faceShade(worldNormal);
 surface.rgb *= vertexLight;
 outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
}
