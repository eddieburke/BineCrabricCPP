#version 430 core
#include "/lib/common.glsl"
// GUI / inventory 3D items. View-space normals feed two-light diffuse in the FSH.
in vec3 vaPosition;
in vec2 vaUV0;
in vec4 vaColor;
in vec3 vaNormal;
out vec2 texcoord;
out vec4 color;
out vec3 normal;
void main() {
 gl_Position = projectionMatrix * modelViewMatrix * vec4(vaPosition + chunkOffset, 1.0);
 texcoord = vaUV0;
 color = vaColor;
 vec3 viewNormal = mat3(modelViewMatrix) * vaNormal;
 normal = length(viewNormal) > 0.001 ? normalize(viewNormal) : vec3(0.0, 1.0, 0.0);
}
