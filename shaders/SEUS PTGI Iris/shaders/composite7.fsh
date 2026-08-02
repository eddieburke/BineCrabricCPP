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



#include "lib/Uniforms.inc"
#include "lib/Common.inc"


/////////////////////////CONFIGURABLE VARIABLES////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////CONFIGURABLE VARIABLES////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////END OF CONFIGURABLE VARIABLES/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////END OF CONFIGURABLE VARIABLES/////////////////////////////////////////////////////////////////////////////////////////////////////////////


const bool colortex3MipmapEnabled = false;

in vec4 texcoord;
in vec3 lightVector;

in float timeSunriseSunset;
in float timeNoon;
in float timeMidnight;

in vec3 colorSunlight;
in vec3 colorSkylight;


vec3 GetColorTexture(vec2 coord)
{
	return pow(texture2DLod(colortex3, coord, 0).rgb, vec3(2.2));
}

vec2 GetNearFragment(vec2 coord, float depth)
{
	vec2 texel = 1.0 / vec2(viewWidth, viewHeight);
	vec4 depthSamples;
	depthSamples.x = texture2D(depthtex1, coord + texel * vec2(1.0, 1.0)).x;
	depthSamples.y = texture2D(depthtex1, coord + texel * vec2(1.0, -1.0)).x;
	depthSamples.z = texture2D(depthtex1, coord + texel * vec2(-1.0, 1.0)).x;
	depthSamples.w = texture2D(depthtex1, coord + texel * vec2(-1.0, -1.0)).x;

	vec2 targetFragment = vec2(0.0, 0.0);

	if (depthSamples.x < depth)
		targetFragment = vec2(1.0, 1.0);
	if (depthSamples.y < depth)
		targetFragment = vec2(1.0, -1.0);
	if (depthSamples.z < depth)
		targetFragment = vec2(-1.0, 1.0);
	if (depthSamples.w < depth)
		targetFragment = vec2(-1.0, -1.0);

	return coord + texel * targetFragment;
}

void 	MotionBlur(inout vec3 color) {
	float depth = GetDepth(texcoord.st);

	//vec2 nearFragment = GetNearFragment(texcoord.st, depth);
	//depth = GetDepth(nearFragment);



	vec4 currentPosition = vec4(texcoord.x * 2.0f - 1.0f, texcoord.y * 2.0f - 1.0f, 2.0f * depth - 1.0f, 1.0f);

	vec4 fragposition = gbufferProjectionInverse * currentPosition;
	fragposition = gbufferModelViewInverse * fragposition;
	fragposition /= fragposition.w;
	fragposition.xyz += cameraPosition;

	vec4 previousPosition = fragposition;
	previousPosition.xyz -= previousCameraPosition;
	previousPosition = gbufferPreviousModelView * previousPosition;
	previousPosition = gbufferPreviousProjection * previousPosition;
	previousPosition /= previousPosition.w;

	vec2 velocity = (currentPosition - previousPosition).st * 0.1f * (1.0 / frameTime) * 0.012 * MOTION_BLUR_INTENSITY;
	float maxVelocity = 0.05f;
		 velocity = clamp(velocity, vec2(-maxVelocity), vec2(maxVelocity));


	if (depth < 0.7)
	{
		velocity *= 0.0;
	}

	//bool isHand = GetMaterialMask(texcoord.st, 5);
	//velocity *= 1.0f - float(isHand);

	int samples = 0;

	#ifdef MOTION_BLUR_HQ
	float dither = 0.0;

	color.rgb = vec3(0.0f);

	for (int i = -2; i <= 2; ++i) {
		vec2 coord = texcoord.st + velocity * (float(i + dither) / 2.0);
			 //coord += vec2(dither) * 1.0f * velocity;

		if (coord.x > 0.0f && coord.x < 1.0f && coord.y > 0.0f && coord.y < 1.0f) {

			color += GetColorTexture(coord).rgb;
			samples += 1;

		}
	}

	color.rgb /= samples;
	#else
	float dither = rand(texcoord.st).x;

	color.rgb = vec3(0.0f);

	for (int i = -2; i <= 2; ++i) {
		vec2 coord = texcoord.st + velocity * (float(i + dither) / 2.0);
			 //coord += vec2(dither) * 1.0f * velocity;

		if (coord.x > 0.0f && coord.x < 1.0f && coord.y > 0.0f && coord.y < 1.0f) {

			color += GetColorTexture(coord).rgb;
			samples += 1;

		}
	}

	color.rgb /= samples;
	#endif


}

/////////////////////////MAIN//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////MAIN//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main() {

	vec3 color = vec3(0.0);

	#ifdef MOTION_BLUR
		MotionBlur(color);
	#else
		color = GetColorTexture(texcoord.st);
	#endif




	color = pow(color, vec3(1.0 / 2.2));

	gl_FragData[0] = vec4(color, 1.0);
	//Write color for previous frame here
	gl_FragData[1] = vec4(texture2D(colortex3, texcoord.st).rgba);

}

/* RENDERTARGETS: 3,7 */
