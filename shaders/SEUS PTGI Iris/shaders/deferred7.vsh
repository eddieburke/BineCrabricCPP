#version 330 compatibility


#include "lib/Uniforms.inc"


out vec4 texcoord;


void main() 
{
	gl_Position = ftransform();
	texcoord = gl_MultiTexCoord0;
}
