#version 330 compatibility

out vec4 color;
out vec4 texcoord;

uniform vec3 sunPosition;
uniform vec3 moonPosition;

void main() {
	gl_Position = ftransform();
	
	color = gl_Color;
	
	texcoord = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	gl_FogFragCoord = gl_Position.z;
}
