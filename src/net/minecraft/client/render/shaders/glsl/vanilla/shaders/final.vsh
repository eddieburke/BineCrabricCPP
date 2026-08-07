#version 430 core
// Iris final — backbuffer present of colortex0.
// https://shaders.properties/current/reference/programs/final/
in vec3 vaPosition;
in vec2 vaUV0;
out vec2 texcoord;
void main() {
 texcoord = vaUV0;
 gl_Position = vec4(vaPosition.xy, 0.0, 1.0);
}
