#version 330 compatibility

in vec4 color;

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;

void main() {
	vec3 skyColor = color.rgb;

	skyColor.rgb *= 0.0;

	float saturation = abs(color.r - color.g) + abs(color.r - color.b) + abs(color.g - color.b);

	if (saturation <= 0.01 && length(color.rgb) > 0.5)
	{
		skyColor.rgb = vec3(0.4);
	}

	outColor0 = vec4(skyColor.rgb, 1.0);
	outColor1 = vec4(0.0, 0.0, 0.0, 1.0);
}

/* RENDERTARGETS: 0,1 */
