#version 430 core
#include "/lib/common.glsl"
in vec3 vaPosition;
in vec4 vaColor;
out vec4 color;
out vec3 viewPosition;
void main() {
 vec4 position = modelViewMatrix * vec4(vaPosition, 1.0);
 gl_Position = projectionMatrix * position;
 color = vaColor;
 viewPosition = position.xyz;
}
