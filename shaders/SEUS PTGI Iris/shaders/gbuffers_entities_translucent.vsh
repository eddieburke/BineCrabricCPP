#version 330 compatibility


#define OLD_LIGHTING_FIX		//In newest versions of the shaders mod/optifine, old lighting isn't removed properly. If OldLighting is On and this is enabled, you'll get proper results in any shaders mod/minecraft version.

#define GLOWING_REDSTONE_BLOCK // If enabled, redstone blocks are treated as light sources for GI
#define GLOWING_LAPIS_LAZULI_BLOCK // If enabled, lapis lazuli blocks are treated as light sources for GI


#define GENERAL_GRASS_FIX

#include "lib/Uniforms.inc"
#include "lib/Common.inc"


in vec4 mc_Entity;
in vec4 at_tangent;
in vec4 mc_midTexCoord;



out vec4 color;
out vec4 texcoord;
out vec4 lmcoord;
out vec3 worldPosition;
out vec3 viewPos;

out vec3 worldNormal;

out vec2 blockLight;

out float materialIDs;






void main() {

	color = gl_Color;

	texcoord = gl_MultiTexCoord0;
	lmcoord = gl_TextureMatrix[1] * gl_MultiTexCoord1;

	blockLight.x = clamp((lmcoord.x * 33.05f / 32.0f) - 1.05f / 32.0f, 0.0f, 1.0f);
	blockLight.y = clamp((lmcoord.y * 33.75f / 32.0f) - 1.05f / 32.0f, 0.0f, 1.0f);

	worldNormal = gl_Normal;

	
	vec4 localWorldPos = gbufferModelViewInverse * gl_ModelViewMatrix * gl_Vertex;

	worldPosition = localWorldPos.xyz + cameraPosition.xyz;
	viewPos = (gl_ModelViewMatrix * gl_Vertex).xyz;



	


	//Gather materials
	materialIDs = 1.0f;
	








	vec4 vp = gl_ModelViewMatrix * gl_Vertex;

	//Temporal jitter
	gl_Position = gl_ProjectionMatrix * vp;
	gl_Position.xyz /= gl_Position.w;
	TemporalJitterProjPos(gl_Position);
	gl_Position.xyz *= gl_Position.w;

}
