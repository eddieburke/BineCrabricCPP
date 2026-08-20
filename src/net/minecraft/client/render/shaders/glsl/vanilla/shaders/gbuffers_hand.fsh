#version 430 core
#include "/lib/common.glsl"
in vec2 texcoord;
in vec2 lmcoord;
in vec4 color;
in vec3 normal;
in float viewDistance;
uniform sampler2D gtexture;
uniform sampler2D lightmap;
uniform vec3 sunDirectionView;
uniform vec3 fillDirectionView;
uniform vec3 ambientColor;
uniform float sunIntensity;
uniform float fillIntensity;
uniform int lightingEnabled;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
float entityDiffuse(vec3 n) {
 float key = max(0.0, dot(normalize(sunDirectionView), n));
 float fill = max(0.0, dot(normalize(fillDirectionView), n));
 return min(1.0, key * sunIntensity + fill * fillIntensity + ambientColor.r);
}
void main() {
 vec4 tex = texture(gtexture, texcoord);
 vec4 surface = vec4(tex.rgb * tintAndAo(color), tex.a);
 if(surface.a < alphaTestRef) discard;
 if(lightingEnabled != 0) surface.rgb *= entityDiffuse(normalize(normal));
 surface.rgb *= texture(lightmap, lmcoord).rgb;
 outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
}
