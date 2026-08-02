in vec3 vaPosition;
in vec2 vaUV0;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
out vec2 texcoord;
out vec4 irs_texCoords[3];
out vec4 irs_Color;
void main() {
 gl_Position = projectionMatrix * modelViewMatrix * vec4(vaPosition, 1.0);
 texcoord = vaUV0;
 irs_texCoords[0] = vec4(vaUV0, 0.0, 1.0);
 irs_texCoords[1] = vec4(0.0, 0.0, 0.0, 1.0);
 irs_texCoords[2] = vec4(0.0, 0.0, 0.0, 1.0);
 irs_Color = vec4(1.0);
}
