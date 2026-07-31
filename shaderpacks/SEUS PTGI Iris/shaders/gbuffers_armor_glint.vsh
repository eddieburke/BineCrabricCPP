#version 330 compatibility

out vec4 color;
out vec4 texcoord;
out vec3 worldPos;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"




void main() {

	texcoord = gl_MultiTexCoord0;


	// gl_Position = ftransform();

	worldPos = (gbufferModelViewInverse * gl_ModelViewMatrix * gl_Vertex).xyz;


	vec4 vp = gl_ModelViewMatrix * gl_Vertex;

	//Temporal jitter
	gl_Position = gl_ProjectionMatrix * vp;
	gl_Position.xyz /= gl_Position.w;
	TemporalJitterProjPos(gl_Position);
	gl_Position.xyz *= gl_Position.w;



	color = gl_Color;

	gl_FogFragCoord = gl_Position.z;
}
