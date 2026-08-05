#version 330 compatibility

out vec4 color;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

void main() {
	gl_Position = ftransform();

	//Temporal jitter
	gl_Position.xyz /= gl_Position.w;
	TemporalJitterProjPos(gl_Position);
	gl_Position.xyz *= gl_Position.w;
	
	color = gl_Color;

	gl_FogFragCoord = gl_Position.z;
}
