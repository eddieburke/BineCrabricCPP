#version 430 core
#include "/lib/common.glsl"
// Iris: block outline + fishing line (fallback = gbuffers_basic).
// Unlit colour geometry — no lightmap multiply (stale vaUV2 was making
// outlines pitch-black depending on the last lit draw).
in vec3 vaPosition;
in vec4 vaColor;
out vec4 color;
out float viewDistance;
void main() {
 vec4 viewPosition = modelViewMatrix * vec4(vaPosition + chunkOffset, 1.0);
 gl_Position = projectionMatrix * viewPosition;
 color = vaColor;
 viewDistance = fogCoord(viewPosition.xyz);
}
