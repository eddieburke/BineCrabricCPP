#version 430 core
#include "/lib/common.glsl"
in vec2 texcoord;
in vec2 lmcoord;
in vec4 color;
in vec3 normal;
in float viewDistance;
uniform sampler2D gtexture;
uniform sampler2D lightmap;
uniform vec4 entityColor;
uniform mat4 gbufferModelViewInverse;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 vec4 tex = texture(gtexture, texcoord);
 // Entity shadows put opacity in vaColor.a; terrain separateAo must not steal it.
 // Alpha = tex.a * color.a so soft shadow quads stay translucent, not pitch-black discs.
 vec4 surface = vec4(tex.rgb * color.rgb, tex.a * color.a);
 if(surface.a < alphaTestRef) discard;
 surface.rgb = mix(surface.rgb, entityColor.rgb, entityColor.a);
 vec3 worldNormal = normalize(mat3(gbufferModelViewInverse) * normal);
 surface.rgb *= faceShade(worldNormal);
 surface.rgb *= texture(lightmap, lmcoord).rgb;
 outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
}
