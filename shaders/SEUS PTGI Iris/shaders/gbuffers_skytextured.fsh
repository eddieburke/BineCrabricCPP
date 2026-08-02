#version 330 compatibility

//sun and moon

uniform sampler2D gtexture;
#define texture gtexture

in vec4 color;
in vec4 texcoord;

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;

void main() {
	vec4 tex = texture2D(texture, texcoord.st);

	outColor0 = tex * color;
	outColor1 = vec4(0.0, 0.0, 0.0, 1.0);
}

/* RENDERTARGETS: 0,1 */
