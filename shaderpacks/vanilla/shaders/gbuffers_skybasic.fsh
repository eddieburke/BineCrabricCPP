#version 430 core
in vec4 color;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 // The sky is its own light source: never lightmapped, never fogged.
 outColor = color;
}
