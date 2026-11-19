#version 430 core
#include "/lib/common.glsl"
#include "/lib/lod_transition.glsl"
in vec3 vertexLight;
in vec4 color;
in float viewDistance;
in vec2 horizontalPosition;
in vec3 nativeViewPosition;
in float directionalShade;
uniform sampler2D depthtex1;
uniform mat4 gbufferProjection;
uniform float clipDistance;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 if(length(horizontalPosition) < clipDistance) discard;
 if(lodCoverage(horizontalPosition).g > 0.5 && lodDither() >= lodBlend(horizontalPosition)) discard;
 vec4 nativeClip = gbufferProjection * vec4(nativeViewPosition, 1.0);
 float nativeDepth = nativeClip.z / nativeClip.w * 0.5 + 0.5;
 if(nativeDepth >= texelFetch(depthtex1, ivec2(gl_FragCoord.xy), 0).r) discard;
 float softening = betaLodEnabled ? smoothstep(betaLodTransition.y, betaLodTransition.y + 256.0,
                                               length(horizontalPosition)) : 0.0;
 vec3 surface = color.rgb * vertexLight * (1.0 + 0.5 * softening * (1.0 / directionalShade - 1.0));
 outColor = vec4(applyFog(surface, viewDistance), color.a);
}
