#version 430 core
#include "/lib/common.glsl"
in vec3 vaPosition;
in vec2 vaUV0;
in vec2 vaUV2;
in vec4 vaColor;
in vec3 vaNormal;
out vec2 texcoord;
out vec2 lmcoord;
out vec4 color;
out vec3 normal;
out float viewDistance;
void main() {
 vec4 viewPosition = modelViewMatrix * vec4(vaPosition + chunkOffset, 1.0);
 gl_Position = projectionMatrix * viewPosition;
 texcoord = vaUV0;
 lmcoord = (vaUV2 + 8.0) / 256.0;
 color = vaColor;
 vec3 viewNormal = mat3(modelViewMatrix) * vaNormal;
 normal = length(viewNormal) > 0.001 ? normalize(viewNormal) : vec3(0.0, 1.0, 0.0);
 viewDistance = length(viewPosition.xyz);
}
