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
	float angledPlane = 0.0f;

	float facingEast = abs(normalize(gl_Normal.xz).x);
	float facingUp = abs(gl_Normal.y);

	//Grass
	if  (  mc_Entity.x == 31.0

		|| mc_Entity.x == 38.0f 	//Rose
		|| mc_Entity.x == 37.0f 	//Flower
		|| mc_Entity.x == 1925.0f 	//Biomes O Plenty: Medium Grass
		|| mc_Entity.x == 1920.0f 	//Biomes O Plenty: Thorns, barley
		|| mc_Entity.x == 1921.0f 	//Biomes O Plenty: Sunflower
		//|| mc_Entity.x >= 312.0f 	//Biomes O Plenty: Lavender
		|| mc_Entity.x == 2.0 && gl_Normal.y < 0.5 && facingEast > 0.01 && facingEast < 0.99 && facingUp < 0.9

		)
	{
		materialIDs = max(materialIDs, 2.0f);
		angledPlane = 1.0f;
	}





	if (  mc_Entity.x == 175.0f)
	{
		materialIDs = max(materialIDs, 2.0f);
	}
	
	//Wheat
	if (mc_Entity.x == 59.0) {
		materialIDs = max(materialIDs, 2.0f);
		angledPlane = 1.0f;
	}	
	
	//Leaves
	if   ( mc_Entity.x == 18.0 
		|| mc_Entity.x == 161.0f
		 ) 
	{
		if (color.r > 0.999 && color.g > 0.999 && color.b > 0.999)
		{

		}
		else
		{
			materialIDs = max(materialIDs, 3.0f);
		}

		if (abs(color.r - color.g) > 0.001 || abs(color.r - color.b) > 0.001 || abs(color.g - color.b) > 0.001)
		{
			materialIDs = max(materialIDs, 3.0f);
		}
	}	

	
	//Gold block
	if (mc_Entity.x == 41) {
		materialIDs = max(materialIDs, 20.0f);
	}
	
	//Iron block
	if (mc_Entity.x == 42) {
		materialIDs = max(materialIDs, 21.0f);
	}
	
	//Diamond Block
	if (mc_Entity.x == 57) {
		materialIDs = max(materialIDs, 22.0f);
	}
	
	//Emerald Block
	if (mc_Entity.x == -123) {
		materialIDs = max(materialIDs, 23.0f);
	}
	
	
	
	//sand
	if (mc_Entity.x == 12) {
		materialIDs = max(materialIDs, 24.0f);
	}

	//sandstone
	if (mc_Entity.x == 24 || mc_Entity.x == -128) {
		materialIDs = max(materialIDs, 25.0f);
	}
	
	//stone
	if (mc_Entity.x == 1) {
		materialIDs = max(materialIDs, 26.0f);
	}
	
	//cobblestone
	if (mc_Entity.x == 4) {
		materialIDs = max(materialIDs, 27.0f);
	}
	
	//wool
	if (mc_Entity.x == 35) {
		materialIDs = max(materialIDs, 28.0f);
	}


	//torch	
	if (mc_Entity.x == 50) {
		materialIDs = max(materialIDs, 30.0f);
	}

	//lava
	if (mc_Entity.x == 10 || mc_Entity.x == 11) {
		materialIDs = max(materialIDs, 31.0f);
	}

	//glowstone and lamp, and lapis, and redstone and sea lantern and jack o lantern
	if (
		mc_Entity.x == 89 || mc_Entity.x == 124 || mc_Entity.x == 169 || mc_Entity.x == 91
#ifdef GLOWING_REDSTONE_BLOCK
		|| mc_Entity.x == 152
#endif
#ifdef GLOWING_LAPIS_LAZULI_BLOCK
		|| mc_Entity.x == 22
#endif
		) 
	{
		materialIDs = max(materialIDs, 32.0f);
	}

	//fire
	if (mc_Entity.x == 51) {
		materialIDs = max(materialIDs, 33.0f);
	}






	


	float fixOldLighting = 1.0;

	if (color.r == 1.0 && color.g == 1.0 && color.b == 1.0)
	{
		fixOldLighting = 0.0;
	}


	#ifdef OLD_LIGHTING_FIX
	if (angledPlane < 0.1 && fixOldLighting > 0.5)
	{
		if (worldNormal.x > 0.85)
		{
			color.rgb *= 1.0 / 0.6;
		}
		if (worldNormal.x < -0.85)
		{
			color.rgb *= 1.0 / 0.6;
		}
		if (worldNormal.z > 0.85)
		{
			color.rgb *= 1.0 / 0.8;
		}
		if (worldNormal.z < -0.85)
		{
			color.rgb *= 1.0 / 0.8;
		}
		if (worldNormal.y < -0.85)
		{
			color.rgb *= 1.0 / 0.5;
		}
	}
	#endif













	vec4 vp = gl_ModelViewMatrix * gl_Vertex;

	//Temporal jitter
	gl_Position = gl_ProjectionMatrix * vp;
	gl_Position.xyz /= gl_Position.w;
	TemporalJitterProjPos(gl_Position);
	gl_Position.xyz *= gl_Position.w;


}
