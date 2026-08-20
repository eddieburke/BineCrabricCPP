#version 430 core
#include "/lib/common.glsl"
in vec3 vaPosition;
in vec4 vaColor;
out vec4 color;
out float viewDistance;
void main() {
 vec4 viewPosition = modelViewMatrix * vec4(vaPosition, 1.0);
 gl_Position = projectionMatrix * viewPosition;
 color = vaColor;
 viewDistance = fogCoord(viewPosition.xyz);
}
