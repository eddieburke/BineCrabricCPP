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



in vec4 texcoord;
in vec3 lightVector;


in float timeSunriseSunset;
in float timeNoon;
in float timeMidnight;

in vec3 colorSunlight;
in vec3 colorSkylight;

in float avgSkyBrightness;



const float overlap = 0.2;

const float rgOverlap = 0.1 * overlap;
const float rbOverlap = 0.01 * overlap;
const float gbOverlap = 0.04 * overlap;

const mat3 coneOverlap = mat3(1.0, 			rgOverlap, 	rbOverlap,
							  rgOverlap, 	1.0, 		gbOverlap,
							  rbOverlap, 	rgOverlap, 	1.0);

const mat3 coneOverlapInverse = mat3(	1.0 + (rgOverlap + rbOverlap), 			-rgOverlap, 	-rbOverlap,
									  	-rgOverlap, 		1.0 + (rgOverlap + gbOverlap), 		-gbOverlap,
									  	-rbOverlap, 		-rgOverlap, 	1.0 + (rbOverlap + rgOverlap));

// ACES
const mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

const mat3 ACESOutputMat = mat3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 Uncharted2Tonemap(vec3 x)
{
	x *= 3.0;

	// float A = 0.15;
	// float B = 0.50;
	// float C = 0.10;
	// float D = 0.20;
	// float E = 0.02;
	// float F = 0.30;

	float A = 0.9;
	float B = 0.8;
	float C = 0.1;
	float D = 1.0;
	float E = 0.02;
	float F = 0.30;

	x = x * coneOverlap;

	x = ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;

	x = x * coneOverlapInverse;

    return x;
}

float almostIdentity( float x, float m, float n )
{
    if( x>m ) return x;

    float a = 2.0*n - m;
    float b = 2.0*m - 3.0*n;
    float t = x/m;

    return (a*t + b)*t*t + n;
}

vec3 almostIdentity(vec3 x, vec3 m, vec3 n)
{
	return vec3(
		almostIdentity(x.x, m.x, n.x),
		almostIdentity(x.y, m.y, n.y),
		almostIdentity(x.z, m.z, n.z)
		);
}

vec3 BlackDepth(vec3 color, vec3 blackDepth)
{
	vec3 m = blackDepth;
	vec3 n = blackDepth * 0.5;
	return (almostIdentity(color, m, n) - n);// * (vec3(1.0) - n);
}

vec3 BurgessTonemap(vec3 col)
{
	col *= 0.9;
	col = col * coneOverlap;

	vec3 maxCol = col;


    const float p = 1.0;
    maxCol = pow(maxCol, vec3(p));

    vec3 retCol = (maxCol * (6.2 * maxCol + 0.05)) / (maxCol * (6.2 * maxCol + 2.3) + 0.06);
	retCol = pow(retCol, vec3(1.0 / p));

	retCol = retCol * coneOverlapInverse;

    return retCol;
}

vec3 SEUSTonemap(vec3 color)
{
	const float p = TONEMAP_CURVE;

	// vec3 ncolor = normalize(color + 0.00001);
	// float colorSaturation = abs(ncolor.r - ncolor.g) + abs(ncolor.r - ncolor.b) + abs(ncolor.g - ncolor.b);

	// color = mix(color, vec3(dot(color, vec3(0.3333))), vec3(-colorSaturation * 0.1));

	color = pow(color, vec3(p));
	color = color / (1.0 + color);
	color = pow(color, vec3((1.0 / GAMMA) / p));




	color = mix(color, color * color * (3.0 - 2.0 * color), vec3(0.2));

	//color = pow(color, vec3(1.0 / 2.0));
	//color = mix(color, color * color * (3.0 - 2.0 * color), vec3(0.1));
	//color = pow(color, vec3(2.0));

	//color = color * 0.5 + 0.5;
	//color = mix(color, color * color * (3.0 - 2.0 * color), vec3(0.8));
	//color = saturate(color * 2.0 - 1.0);




	return color;
}

vec3 ReinhardJodie(vec3 v)
{
	v = pow(v, vec3(TONEMAP_CURVE));
    float l = Luminance(v);
    vec3 tv = v / (1.0f + v);

    vec3 tonemapped = mix(v / (1.0f + l), tv, tv);
	tonemapped = pow(tonemapped, vec3(1.0 / TONEMAP_CURVE));

	return tonemapped;
}



/////////////////////////////////////////////////////////////////////////////////
//	ACES Fitting by Stephen Hill
vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (1.0f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESTonemap2(vec3 color)
{
	color *= 1.5;
	color = color * ACESInputMat;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);


    // Clamp to [0, 1]
	color = color * ACESOutputMat;
    color = saturate(color);

    return color;
}
/////////////////////////////////////////////////////////////////////////////////





vec3 ACESTonemap(vec3 color)
{
	color *= 0.7;

		color = color * coneOverlap;



		vec3 crosstalk = vec3(0.05, 0.2, 0.05) * 2.9;

		// float avgColor = (color.r + color.g + color.b) * 0.33333;
		float avgColor = Luminance(color.rgb);

		// color = mix(color, vec3(avgColor), crosstalk);


	const float p = 1.0;
	color = pow(color, vec3(p));
	color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
	// color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.1);
	color = pow(color, vec3(1.0 / p));

		color = color * coneOverlapInverse;


		// float avgColorTonemapped = (color.r + color.g + color.b) * 0.33333;
		float avgColorTonemapped = Luminance(color.rgb);

		// color = mix(color, vec3(avgColorTonemapped), -crosstalk * 1.0);


	color = saturate(color);

	color = pow(color, vec3(0.9));

	// color = mix(color, vec3(avgColorTonemapped), vec3(-saturate(avgColor * 0.25 + 0.0)));


	return color;
}



vec3 	CalculateNoisePattern1(vec2 offset, float size) 
{
	vec2 coord = texcoord.st;

	coord *= vec2(viewWidth, viewHeight);
	coord = mod(coord + offset, vec2(size));
	coord /= 64.0;

	return texture2D(noisetex, coord).xyz;
}

vec4 cubic(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w;
    w.x =   -x3 + 3*x2 - 3*x + 1;
    w.y =  3*x3 - 6*x2       + 4;
    w.z = -3*x3 + 3*x2 + 3*x + 1;
    w.w =  x3;
    return w / 6.f;
}

vec4 BicubicTexture(in sampler2D tex, in vec2 coord)
{
	vec2 resolution = vec2(viewWidth, viewHeight);

	coord *= resolution;

	float fx = fract(coord.x);
    float fy = fract(coord.y);
    coord.x -= fx;
    coord.y -= fy;

    fx -= 0.5;
    fy -= 0.5;

    vec4 xcubic = cubic(fx);
    vec4 ycubic = cubic(fy);

    vec4 c = vec4(coord.x - 0.5, coord.x + 1.5, coord.y - 0.5, coord.y + 1.5);
    vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
    vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

    vec4 sample0 = texture2D(tex, vec2(offset.x, offset.z) / resolution);
    vec4 sample1 = texture2D(tex, vec2(offset.y, offset.z) / resolution);
    vec4 sample2 = texture2D(tex, vec2(offset.x, offset.w) / resolution);
    vec4 sample3 = texture2D(tex, vec2(offset.y, offset.w) / resolution);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix( mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

vec3 GetBloomTap(vec2 coord, const float octave, const vec2 offset)
{
	float scale = exp2(octave);

	coord /= scale;
	coord -= offset;

	// return GammaToLinear(BicubicTexture(colortex2, coord).rgb);
	return GammaToLinear(texture2D(colortex2, coord).rgb);
}

vec2 CalcOffset(float octave)
{
    vec2 offset = vec2(0.0);
    
    vec2 padding = vec2(30.0) / vec2(viewWidth, viewHeight);
    
    offset.x = -min(1.0, floor(octave / 3.0)) * (0.25 + padding.x);
    
    offset.y = -(1.0 - (1.0 / exp2(octave))) - padding.y * octave;

	offset.y += min(1.0, floor(octave / 3.0)) * 0.35;
    
 	return offset;   
}



vec3 GetBloom(vec2 coord)
{
	vec3 bloom = vec3(0.0);

	bloom += GetBloomTap(coord, 1.0, CalcOffset(0.0)) * 2.0;
	bloom += GetBloomTap(coord, 2.0, CalcOffset(1.0)) * 1.5;
	bloom += GetBloomTap(coord, 3.0, CalcOffset(2.0)) * 1.2;
	bloom += GetBloomTap(coord, 4.0, CalcOffset(3.0)) * 1.3;
	bloom += GetBloomTap(coord, 5.0, CalcOffset(4.0)) * 1.4;
	bloom += GetBloomTap(coord, 6.0, CalcOffset(5.0)) * 1.3;
	bloom += GetBloomTap(coord, 7.0, CalcOffset(6.0)) * 1.2;
	bloom += GetBloomTap(coord, 8.0, CalcOffset(7.0)) * 1.1;
	bloom += GetBloomTap(coord, 9.0, CalcOffset(8.0)) * 0.0;

	bloom /= 12.6;


	//bloom = mix(bloom, vec3(dot(bloom, vec3(0.3333))), vec3(-0.1));
	//bloom = mix(bloom, vec3(dot(bloom, vec3(0.3333))), vec3(0.1));

	//bloom = length(bloom) * pow(normalize(bloom + 0.00001), vec3(1.5));

	return bloom;
}

void CalculateExposureEyeBrightness(inout vec3 color) 
{
	float exposureMax = 1.55f;
		  //exposureMax *= mix(1.0f, 0.25f, timeSunriseSunset);
		  //exposureMax *= mix(1.0f, 0.0f, timeMidnight);
		  //exposureMax *= mix(1.0f, 0.25f, rainStrength);
		  exposureMax *= avgSkyBrightness * 2.0;
	float exposureMin = 0.07f;
	float exposure = pow(eyeBrightnessSmooth.y / 240.0f, 6.0f) * exposureMax + exposureMin;

	//exposure = 1.0f;

	color.rgb /= vec3(exposure);
	color.rgb *= 350.0;
}

void AverageExposure(inout vec3 color)
{
	// float avglod = int(log2(min(viewWidth, viewHeight))) - 0;
	// color /= pow(Luminance(texture2DLod(colortex3, vec2(0.65, 0.65), avglod).rgb), 1.5) * 3.9 + 0.00015;

	float avgLum = texture2DLod(colortex7, vec2(0.0, 0.0), 0).a * 0.01;

	// color /= avgLum * 3.9 + 0.00015;
	color /= avgLum * 23.9 + 0.0008;
}

void MicroBloom(inout vec3 color)
{
	/*
	vec2 texel = vec2(1.0 / viewWidth, 1.0 / viewHeight);
	vec3 sum = GetColorTexture(texcoord.st + vec2(0.5, -0.5) * texel * 0.9).rgb;
		 sum += GetColorTexture(texcoord.st + vec2(0.5, 0.5) * texel * 0.9).rgb;
		 sum += GetColorTexture(texcoord.st + vec2(-0.5, 0.5) * texel * 0.9).rgb;
		 sum += GetColorTexture(texcoord.st + vec2(-0.5, -0.5) * texel * 0.9).rgb;

	vec3 sum2 = GetColorTexture(texcoord.st + vec2(0.5, -0.5) * texel * 1.9).rgb;
		 sum2 += GetColorTexture(texcoord.st + vec2(0.5, 0.5) * texel * 1.9).rgb;
		 sum2 += GetColorTexture(texcoord.st + vec2(-0.5, 0.5) * texel * 1.9).rgb;
		 sum2 += GetColorTexture(texcoord.st + vec2(-0.5, -0.5) * texel * 1.9).rgb;

	color += sum * 0.1 * 0.75;
	color += sum2 * 0.05 * 0.75;
	*/

	vec3 bloom = vec3(0.0);
	float allWeights = 0.0f;

	for (int i = 0; i < 4; i++) 
	{
		for (int j = 0; j < 4; j++) 
		{
			float weight = 1.0f - distance(vec2(i, j), vec2(2.5f)) / 2.5;
				  weight = clamp(weight, 0.0f, 1.0f);
				  weight = 1.0f - cos(weight * 3.1415 / 2.0f);
				  weight = pow(weight, 2.0f);
			vec2 coord = vec2(i - 2.5, j - 2.5);
				 coord.x /= viewWidth;
				 coord.y /= viewHeight;
				 //coord *= 0.0f;

				 //coord.x -= 0.5f / viewWidth;
				 //coord.y -= 0.5f / viewHeight;

			vec2 finalCoord = (texcoord.st + coord.st * 1.0);

			if (weight > 0.0f)
			{
				bloom += pow(clamp(texture2DLod(colortex3, finalCoord, 0).rgb, vec3(0.0f), vec3(1.0f)), vec3(2.2f)) * weight;
				allWeights += 1.0f * weight;
			}
		}
	}
	bloom /= allWeights;

	color = mix(color, bloom, vec3(0.4));
}

void 	Vignette(inout vec3 color) {
	float dist = distance(texcoord.st, vec2(0.5f)) * 2.0f;
		  dist /= 1.5142f;

		  //dist = pow(dist, 1.1f);

	color.rgb *= 1.0f - dist * 0.5;

}

void DoNightEye(inout vec3 color)
{
	float lum = Luminance(color * vec3(1.0, 1.0, 1.0));
	float mixSize = 1250000.0;
	float mixFactor = 0.01 / (pow(lum * mixSize, 2.0) + 0.01);


	vec3 nightColor = mix(color, vec3(lum), vec3(0.9)) * vec3(0.25, 0.5, 1.0) * 2.0;

	color = mix(color, nightColor, mixFactor);
}

void Overlay(inout vec3 color, vec3 overlayColor)
{
	vec3 overlay = vec3(0.0);

	for (int i = 0; i < 3; i++)
	{
		if (color[i] > 0.5)
		{
			float valueUnit = (1.0 - color[i]) / 0.5;
			float minValue = color[i] - (1.0 - color[i]);
			overlay[i] = (overlayColor[i] * valueUnit) + minValue;
		}
		else
		{
			float valueUnit = color[i] / 0.5;
			overlay[i] = overlayColor[i] * valueUnit;
		}
	}

	color = overlay;
}

vec3 BlueNoise(vec2 coord)
{
	vec2 noiseCoord = vec2(coord.st * vec2(viewWidth, viewHeight)) / 64.0;
	//noiseCoord += vec2(frameCounter, frameCounter);
	//noiseCoord += mod(frameCounter, 16.0) / 16.0;
	//noiseCoord += rand(vec2(mod(frameCounter, 16.0) / 16.0, mod(frameCounter, 16.0) / 16.0) + 0.5).xy;
	noiseCoord += vec2(sin(frameCounter * 0.75), cos(frameCounter * 0.75));

	noiseCoord = (floor(noiseCoord * 64.0) + 0.5) / 64.0;

	return texture2D(noisetex, noiseCoord).rgb;
}

/////////////////////////MAIN//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////MAIN//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main() {

	vec3 color = 	(texture2D(colortex3, texcoord.st).rgb);


	// Sharpen
	#if POST_SHARPENING > 0
	{
		vec2 texel = 1.0 / vec2(viewWidth, viewHeight);
		vec3 cs = 	(texture2D(colortex3, texcoord.st + vec2(texel.x, texel.y) * 0.5).rgb);
		cs += 		(texture2D(colortex3, texcoord.st + vec2(texel.x, -texel.y) * 0.5).rgb);
		cs += 		(texture2D(colortex3, texcoord.st + vec2(-texel.x, texel.y) * 0.5).rgb);
		cs += 		(texture2D(colortex3, texcoord.st + vec2(-texel.x, -texel.y) * 0.5).rgb);
		cs -= color;
		cs /= 3.0;

		// color += clamp(dot(color - cs, vec3(0.333333)), -0.001, 0.001) * 12.3 * pow(Luminance(color), 0.5);
		color += clamp(dot(color - cs, vec3(0.333333)), -0.001, 0.001) * 12.3 * float(POST_SHARPENING) * pow(Luminance(color), 0.5) * normalize(color.rgb + 0.00000001);
	}
	#endif

	color = GammaToLinear(color);



	float linDepth = ExpToLinearDepth(texture2D(depthtex0, texcoord.st).x);

	vec3 bloom = GetBloom(texcoord.st);
	color = mix(color, bloom, vec3(0.035 * BLOOM_AMOUNT + isEyeInWater * 0.5));
	// color = mix(color, bloom, vec3(1.0 - exp(-linDepth * 0.03)));
	color = max(vec3(0.0), color);

	Vignette(color);

	color = BlackDepth(color, vec3(0.000015 * BLACK_DEPTH * BLACK_DEPTH));


	AverageExposure(color);
	// color *= 51.0;


	// color *= mix(BlueNoise(texcoord.st * 1.0).xxx + 1.0, vec3(1.0), vec3(0.3));


	// const float blackRolloff = 0.005 * BLACK_DEPTH;
	// const float blackClip = 0.0;

 //    color = vec3(
 //    	almostIdentity(color.x, blackRolloff, blackClip),
 //    	almostIdentity(color.y, blackRolloff, blackClip),
 //    	almostIdentity(color.z, blackRolloff, blackClip)
 //    	) - blackClip;




	color *= 9.6 * EXPOSURE; 


	color = saturate(TONEMAP_OPERATOR(color));


	// color = texture2DLod(shadowcolor1, texcoord.st * 0.5 + 0.5, 0).rrr;



	color = pow(color, vec3(1.0 / 2.2 + (1.0 - GAMMA)));


	color = (mix(color, vec3(Luminance(color)), vec3(1.0 - SATURATION)));




	color += rand(texcoord.st) * (1.0 / 255.0);



	// color = vec3(texture2DLod(colortex7, texcoord.st, 0).a);



	gl_FragColor = vec4(color.rgb, 1.0f);

}
