#version 430 core
#include "/lib/common.glsl"
// Unlit textured world draws (weather, clouds, etc.). No lightmap coords.
in vec3 vaPosition;
in vec2 vaUV0;
in vec4 vaColor;
out vec2 texcoord;
out vec4 color;
out float viewDistance;
void main() {
 vec4 viewPosition = modelViewMatrix * vec4(vaPosition + chunkOffset, 1.0);
 gl_Position = projectionMatrix * viewPosition;
 texcoord = vaUV0;
 color = vaColor;
 viewDistance = fogCoord(viewPosition.xyz);
}
