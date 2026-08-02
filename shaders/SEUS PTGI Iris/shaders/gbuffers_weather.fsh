#version 330 compatibility

uniform sampler2D gtexture;
#define texture gtexture
uniform sampler2D lightmap;

in vec4 color;
in vec4 texcoord;
in vec4 lmcoord;

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;
layout(location = 2) out vec4 outColor2;
layout(location = 3) out vec4 outColor3;

/* RENDERTARGETS: 0,1,2,3 */

void main() {
	discard;
	outColor0 = vec4(texture2D(texture, texcoord.st).rgb, texture2D(texture, texcoord.st).a * 1.0) * color;
	outColor1 = vec4(0.0);
	outColor2 = vec4(0.0);
	outColor3 = vec4(0.0);
}
