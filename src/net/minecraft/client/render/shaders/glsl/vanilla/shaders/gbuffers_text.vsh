#version 430 core
#include "/lib/common.glsl"
// Glyph atlas text. No lightmap.
in vec3 vaPosition;
in vec2 vaUV0;
in vec4 vaColor;
out vec2 texcoord;
out vec4 color;
void main() {
 gl_Position = projectionMatrix * modelViewMatrix * vec4(vaPosition + chunkOffset, 1.0);
 texcoord = vaUV0;
 color = vaColor;
}
