#version 330 compatibility

in vec4 color;

uniform int fogMode;

layout(location = 0) out vec4 outColor0;

/* RENDERTARGETS: 0 */

void main() {
	vec4 albedo = color;
	albedo.a = 1.0;
	outColor0 = albedo;
}
