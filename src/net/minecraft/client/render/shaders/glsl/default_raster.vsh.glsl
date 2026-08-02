in vec3 vaPosition;
in vec2 vaUV0;
in vec2 vaUV2;
in vec4 vaColor;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 textureMatrix;
uniform vec3 chunkOffset;
const mat4 iris_lightmapTextureMatrix = mat4(vec4(0.00390625, 0.0, 0.0, 0.0), vec4(0.0, 0.00390625, 0.0, 0.0), vec4(0.0, 0.0, 0.00390625, 0.0), vec4(0.03125, 0.03125, 0.03125, 1.0));
out vec4 irs_texCoords[3];
out vec4 irs_Color;
void main() {
 gl_Position = projectionMatrix * modelViewMatrix * vec4(vaPosition + chunkOffset, 1.0);
 irs_texCoords[0] = textureMatrix * vec4(vaUV0, 0.0, 1.0);
 irs_texCoords[1] = iris_lightmapTextureMatrix * vec4(vaUV2, 0.0, 1.0);
 irs_texCoords[2] = iris_lightmapTextureMatrix * vec4(vaUV2, 0.0, 1.0);
 irs_Color = vaColor;
}
