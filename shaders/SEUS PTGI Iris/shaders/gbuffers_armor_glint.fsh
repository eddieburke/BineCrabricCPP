#version 330 compatibility

in vec4 color;
in vec4 texcoord;
in vec3 worldPos;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

layout(location = 0) out vec4 outColor0;

void main()
{
	vec4 albedo = color;
	albedo.a = 1.0;

	// albedo *= pow(texture(colortex0, texcoord.xy * 1.0 + FRAME_TIME * 0.3), vec4(2.2));
	albedo *= texture(colortex0, texcoord.xy * 1.0 + FRAME_TIME * 0.03);

	albedo.rgb = pow(albedo.rgb, vec3(2.2));

	outColor0 = albedo;
}
/* RENDERTARGETS: 0 */
