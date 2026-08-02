#version 330 compatibility

/*
 _______ _________ _______  _______  _ 
(  ____ \\__   __/(  ___  )(  ____ )( )
| (    \/   ) (   | (   ) || (    )|| |
| (_____    | |   | |   | || (____)|| |
(_____  )   | |   | |   | ||  _____)| |
      ) |   | |   | |   | || (      (_)
/\____) |   | |   | (___) || )       _ 
\_______)   )_(   (_______)|/       (_)

Do not modify this code until you have read the LICENSE.txt contained in the root directory of this shaderpack!

*/

















in vec4 color;
in vec4 texcoord;
in vec4 lmcoord;
in vec3 worldPosition;
in vec3 viewPos;

in vec3 worldNormal;

in vec2 blockLight;

in float materialIDs;






#include "lib/Uniforms.inc"
#include "lib/Common.inc"
#include "lib/GBufferData.inc"
#include "lib/GBuffersCommon.inc"

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;
layout(location = 2) out vec4 outColor2;




void main() 
{	
	float lodOffset = 0.0;

	vec4 albedo = texture2D(texture, texcoord.st, lodOffset);
	albedo *= color;

	vec2 mcLightmap = blockLight;



	float wetnessModulator = 1.0;

	vec3 rainNormal = vec3(0.0, 0.0, 0.0);
	#ifdef RAIN_SPLASH_EFFECT
	rainNormal = GetRainSplashNormal(worldPosition, worldNormal, wetnessModulator);
	#endif

	wetnessModulator *= saturate(worldNormal.y * 10.5 + 0.7);
	wetnessModulator *= saturate(abs(2.0 - materialIDs));
	wetnessModulator *= clamp(blockLight.y * 1.05 - 0.7, 0.0, 0.3) / 0.3;
	wetnessModulator *= saturate(wetness * 1.1 - 0.1);



	vec3 N;
	mat3 tbn;
	CalculateNormalAndTBN(viewPos.xyz, texcoord.st, N, tbn);




	vec4 specTex = texture2D(specular, texcoord.st, lodOffset);
		 #ifdef SPEC_SMOOTHNESS_AS_ROUGHNESS
specTex.SPEC_CHANNEL_SMOOTHNESS = 1.0 - specTex.SPEC_CHANNEL_SMOOTHNESS;
#endif
specTex.SPEC_CHANNEL_SMOOTHNESS = specTex.SPEC_CHANNEL_SMOOTHNESS * 0.992; 								// Fix weird specular issue
	vec4 normalTex = texture2D(normals, texcoord.st, lodOffset) * 2.0 - 1.0;

	float normalMapStrength = 2.0;
	#ifdef FORCE_WET_EFFECT
	normalMapStrength = mix(normalMapStrength, 0.1, wetnessModulator * wetnessModulator * wetnessModulator * wetnessModulator);
	#endif

	vec3 viewNormal = tbn * normalize(normalTex.xyz * vec3(normalMapStrength, normalMapStrength, 1.0) + rainNormal * wetnessModulator);

	
	// Get specular data from specular texture
	float smoothness = specTex.SPEC_CHANNEL_SMOOTHNESS;
	float metallic = specTex.SPEC_CHANNEL_METALNESS;
	float emissive = specTex.b;

	#ifdef FORCE_WET_EFFECT
	smoothness = mix(smoothness, 1.0, saturate(wetnessModulator * 1.0 * saturate(1.0 - metallic)));
	#endif

	// Darker albedo when wet
	albedo.rgb = pow(albedo.rgb, vec3(1.0 + wetnessModulator * (1.0 - metallic) * 0.3));




	// Fix impossible normal angles
	vec3 viewDir = -normalize(viewPos.xyz);
	vec3 relfectDir = reflect(-viewDir, viewNormal);
	// make outright impossible
	viewNormal.xyz = normalize(viewNormal.xyz + (N / (pow(saturate(dot(viewNormal, viewDir)) + 0.001, 0.5)) * 1.0));





	// vec3 analyticNormal = normalize(cross(dFdx(viewPos.xyz), dFdy(viewPos.xyz)));

	// albedo.rgb = analyticNormal.xyz * 0.5 + 0.5;


	albedo.rgb = mix(albedo.rgb, entityColor.rgb, vec3(entityColor.a));




	GBufferData gbuffer;
	gbuffer.albedo = albedo;
	gbuffer.normal = viewNormal.xyz;
	gbuffer.mcLightmap = mcLightmap;
	gbuffer.smoothness = smoothness;
	gbuffer.metalness = metallic;
	gbuffer.materialID = (materialIDs + 0.1) / 255.0;
	gbuffer.emissive = 0.0;
	gbuffer.geoNormal = N.xyz;
	gbuffer.totalTexGrad = length(fwidth(texcoord.st)) * (256.0 / 8.0);


	vec4 frag0, frag1, frag2, frag3;

	OutputGBufferDataSolid(gbuffer, frag0, frag1, frag2, frag3);

	outColor0 = frag0;
	outColor1 = frag1;
	outColor2 = frag2;

}

/* RENDERTARGETS: 0,1,2 */
