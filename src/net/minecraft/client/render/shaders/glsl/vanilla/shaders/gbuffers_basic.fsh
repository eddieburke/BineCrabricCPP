#version 430 core
#include "/lib/common.glsl"
// Pack-owned b1.7.3 lighting:
//   albedo * tintAndAo(vaColor) * faceShade(vaNormal) * lightmap * fog
in vec2 texcoord;
in vec3 vertexLight;
in vec4 color;
in vec3 normal;
in float viewDistance;
uniform sampler2D gtexture;
uniform vec4 entityColor;
uniform mat4 gbufferModelViewInverse;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 vec4 surface = texture(gtexture, texcoord) * color;
 if(surface.a < alphaTestRef) discard;
 surface.rgb = mix(surface.rgb, entityColor.rgb, entityColor.a);
 vec3 worldNormal = normalize(mat3(gbufferModelViewInverse) * normal);
 surface.rgb *= faceShade(worldNormal);
 surface.rgb *= vertexLight;
 outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
}
