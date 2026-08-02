#version 330 compatibility




in vec4 mc_Entity;

out vec4 color;
out vec4 texcoord;
out vec4 lmcoord;
out vec3 worldPosition;
out vec4 vertexPos;
out vec4 viewPosition;

out vec3 normal;
out vec3 globalNormal;
out vec3 tangent;
out vec3 binormal;
out vec3 viewVector;
out vec3 viewVector2;
out float distance;


out float isWater;
out float isIce;
out float isStainedGlass;
out float isSlime;
out vec2 blockLight;

out vec3 worldNormal;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"


void main() {

	isWater = 0.0;
	isIce = 0.0;
	isStainedGlass = 0.0;
	isSlime = 0.0;

	if(mc_Entity.x == 8)
	{
		isWater = 1.0;
	}

	if (mc_Entity.x == 79) {
		// isIce = 1.0;
		isStainedGlass = 1.0;
	}
	
		 vertexPos = gl_Vertex;

	// if (mc_Entity.x == 1971.0f)
	// {
	// 	isWater = 1.0f;
	// }
	
	// if (mc_Entity.x == 8 || mc_Entity.x == 9) {
	// 	isWater = 1.0f;
	// }

	if (mc_Entity.x == 95 || mc_Entity.x == 160 || mc_Entity.x == 90)
	{
		isStainedGlass = 1.0;
	}

	if (mc_Entity.x == 165)
	{
		isSlime = 1.0;
	}


	
		
	vec4 viewPos = gbufferModelViewInverse * gl_ModelViewMatrix * gl_Vertex;
	vec4 position = viewPos;

	worldPosition.xyz = viewPos.xyz + cameraPosition.xyz;
	viewPosition = gbufferModelView * position;

	vec4 localPosition = gl_ModelViewMatrix * gl_Vertex;

	distance = length(localPosition.xyz);

	gl_Position = gl_ProjectionMatrix * (gbufferModelView * position);



	//Temporal jitter
	gl_Position.xyz /= gl_Position.w;
	TemporalJitterProjPos(gl_Position);
	gl_Position.xyz *= gl_Position.w;

	gl_Position.z -= 0.0001;

	
	color = gl_Color;
	
	texcoord = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	lmcoord = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	
	blockLight.x = clamp((lmcoord.x * 33.05f / 32.0f) - 1.05f / 32.0f, 0.0f, 1.0f);
	blockLight.y = clamp((lmcoord.y * 33.75f / 32.0f) - 1.05f / 32.0f, 0.0f, 1.0f);

	gl_FogFragCoord = gl_Position.z;


	
	
	normal = normalize(gl_NormalMatrix * gl_Normal);
	globalNormal = normalize(gl_Normal);

	if (gl_Normal.x > 0.5) {
		//  1.0,  0.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 0.0,  0.0, -1.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	} else if (gl_Normal.x < -0.5) {
		// -1.0,  0.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 0.0,  0.0,  1.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	} else if (gl_Normal.y > 0.5) {
		//  0.0,  1.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0,  0.0,  1.0));
	} else if (gl_Normal.y < -0.5) {
		//  0.0, -1.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0,  0.0,  1.0));
	} else if (gl_Normal.z > 0.5) {
		//  0.0,  0.0,  1.0
		tangent  = normalize(gl_NormalMatrix * vec3( 1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	} else if (gl_Normal.z < -0.5) {
		//  0.0,  0.0, -1.0
		tangent  = normalize(gl_NormalMatrix * vec3(-1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	}
	
	mat3 tbnMatrix = mat3(tangent.x, binormal.x, normal.x,
                          tangent.y, binormal.y, normal.y,
                          tangent.z, binormal.z, normal.z);

	viewVector = (gl_ModelViewMatrix * gl_Vertex).xyz;
	viewVector2 = normalize(viewVector);
	viewVector = normalize(tbnMatrix * viewVector);


	worldNormal = gl_Normal.xyz;

	
}
