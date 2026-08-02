#version 430 core
#include "/lib/common.glsl"
// GUI 3D items: albedo * tint * two-light diffuse (enableGUIStandardItemLighting).
in vec2 texcoord;
in vec4 color;
in vec3 normal;
uniform sampler2D gtexture;
uniform vec4 entityColor;
uniform vec3 sunDirectionView;
uniform vec3 fillDirectionView;
uniform vec3 ambientColor;
uniform float sunIntensity;
uniform float fillIntensity;
uniform int lightingEnabled;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
float itemDiffuse(vec3 n) {
 vec3 l0 = normalize(sunDirectionView);
 vec3 l1 = normalize(fillDirectionView);
 float light0 = max(0.0, dot(l0, n));
 float light1 = max(0.0, dot(l1, n));
 return min(1.0, light0 * sunIntensity + light1 * fillIntensity + ambientColor.r);
}
void main() {
 vec4 surface = texture(gtexture, texcoord) * color;
 if(surface.a < alphaTestRef) discard;
 surface.rgb = mix(surface.rgb, entityColor.rgb, entityColor.a);
 if(lightingEnabled != 0) {
  vec3 n = length(normal) > 0.001 ? normalize(normal) : vec3(0.0, 1.0, 0.0);
  surface.rgb *= itemDiffuse(n);
 }
 outColor = surface;
}
