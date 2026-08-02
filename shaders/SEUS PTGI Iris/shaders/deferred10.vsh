#version 330 compatibility






out vec4 texcoord;

out vec3 lightVector;
out vec3 worldLightVector;
out vec3 worldSunVector;

out float timeMidnight;

out vec3 colorSunlight;
out vec3 colorSkylight;
out vec3 colorSkyUp;
out vec3 colorTorchlight;

out vec4 skySHR;
out vec4 skySHG;
out vec4 skySHB;



#include "lib/Uniforms.inc"
#include "lib/Common.inc"



void main() 
{
	gl_Position = ftransform();
	texcoord = gl_MultiTexCoord0;

	// Get light and sun vectors
	worldLightVector = normalize((shadowModelViewInverse * vec4(0.0, 0.0, 1.0, 0.0)).xyz);
	worldSunVector = worldLightVector * -sign(sunAngle * 2.0 - 1.0);

	lightVector = normalize((gbufferModelView * vec4(worldLightVector.xyz, 0.0)).xyz); 

	// Get diffuse light colors and data
	colorSunlight = GetColorSunlight(worldSunVector, rainStrength);
	GetSkylightData(worldSunVector, rainStrength,
		skySHR, skySHG, skySHB,
		colorSkylight, colorSkyUp);
	colorTorchlight = GetColorTorchlight();

	// Time values
	timeMidnight = GetTimeMidnight(worldSunVector);
}
