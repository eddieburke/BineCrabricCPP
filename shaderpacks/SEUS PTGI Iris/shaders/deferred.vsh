#version 330 compatibility










out vec4 texcoord;

out float timeMidnight;

out vec3 colorSunlight;
out vec3 colorSkylight;
out vec3 colorSkyUp;
out vec3 colorTorchlight;

out vec4 skySHR;
out vec4 skySHG;
out vec4 skySHB;


out vec3 worldLightVector;
out vec3 worldSunVector;


out mat4 gbufferPreviousModelViewInverse;
out mat4 gbufferPreviousProjectionInverse;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

void main() 
{
	gl_Position = ftransform();
	texcoord = gl_MultiTexCoord0;

	// Get light and sun vectors
	worldLightVector = normalize((shadowModelViewInverse * vec4(0.0, 0.0, 1.0, 0.0)).xyz);
	worldSunVector = worldLightVector * -sign(sunAngle * 2.0 - 1.0);

	// Get diffuse light colors and data
	colorSunlight = GetColorSunlight(worldSunVector, rainStrength);
	GetSkylightData(worldSunVector, rainStrength,
		skySHR, skySHG, skySHB,
		colorSkylight, colorSkyUp);
	colorTorchlight = GetColorTorchlight();

	// Time values
	timeMidnight = GetTimeMidnight(worldSunVector);

	// Inverse previous matrices for reprojection
	gbufferPreviousModelViewInverse = inverse(gbufferPreviousModelView);
	gbufferPreviousProjectionInverse = inverse(gbufferPreviousProjection);
}
