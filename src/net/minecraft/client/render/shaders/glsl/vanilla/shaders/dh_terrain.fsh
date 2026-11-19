#version 430 core
#include "/lib/common.glsl"
#include "/lib/lod_transition.glsl"
in vec3 vertexLight;
in vec4 color;
in float viewDistance;
in vec2 horizontalPosition;
in float directionalShade;
uniform float clipDistance;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 if(length(horizontalPosition) < clipDistance) discard;
 float softening = betaLodEnabled ? smoothstep(betaLodTransition.y, betaLodTransition.y + 256.0,
                                               length(horizontalPosition)) : 0.0;
 vec3 surface = color.rgb * vertexLight * (1.0 + 0.5 * softening * (1.0 / directionalShade - 1.0));
 outColor = vec4(applyFog(surface, viewDistance), 1.0);
}
