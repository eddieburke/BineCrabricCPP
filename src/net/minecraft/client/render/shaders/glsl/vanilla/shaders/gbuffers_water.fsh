#version 430 core
#include "/lib/common.glsl"
#include "/lib/lod_transition.glsl"
in vec2 texcoord;
in vec3 vertexLight;
in vec4 color;
in vec3 normal;
in float viewDistance;
in vec2 horizontalPosition;
in vec3 nativeViewPosition;
uniform sampler2D gtexture;
uniform sampler2D dhDepthTex1;
uniform mat4 dhProjection;
uniform mat4 gbufferModelViewInverse;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 if(lodCoverage(horizontalPosition).r > 0.5 && lodDither() < lodBlend(horizontalPosition)) discard;
 if(betaLodEnabled) {
  vec4 distantClip = dhProjection * vec4(nativeViewPosition, 1.0);
  float distantDepth = distantClip.z / distantClip.w * 0.5 + 0.5;
  if(distantDepth > texelFetch(dhDepthTex1, ivec2(gl_FragCoord.xy), 0).r) discard;
 }
 vec4 tex = texture(gtexture, texcoord);
 vec4 surface = vec4(tex.rgb * tintAndAo(color), tex.a);
 if(surface.a < alphaTestRef) discard;
 vec3 worldNormal = normalize(mat3(gbufferModelViewInverse) * normal);
 surface.rgb *= faceShade(worldNormal);
 surface.rgb *= vertexLight;
 outColor = vec4(applyFog(surface.rgb, viewDistance), surface.a);
}
