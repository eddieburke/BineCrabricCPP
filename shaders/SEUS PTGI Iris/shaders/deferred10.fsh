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

/////////////////////////CONFIGURABLE VARIABLES////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////CONFIGURABLE VARIABLES////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////END OF CONFIGURABLE VARIABLES/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////END OF CONFIGURABLE VARIABLES/////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int 		shadowMapResolution 	= 4096;
const float 	shadowDistance 			= 120.0; // Shadow distance. Set lower if you prefer nicer close shadows. Set higher if you prefer nicer distant shadows. [80.0 120.0 180.0 240.0]
// Iris: keep an unculled sphere for GI voxelization while far shadow geometry stays culled.
// Must be <= shadowDistance. ~half of SEUS shadow-volume side (~161) centered on the camera.
const float 	voxelDistance 			= 80.0;
const float 	shadowIntervalSize 		= 1.0f;
const bool 		shadowHardwareFiltering0 = true;

const bool 		shadowtexMipmap = true;
const bool 		shadowtex1Mipmap = false;
const bool 		shadowtex1Nearest = false;
const bool 		shadowcolor0Mipmap = false;
const bool 		shadowcolor0Nearest = false;
const bool 		shadowcolor1Mipmap = false;
const bool 		shadowcolor1Nearest = false;

const float shadowDistanceRenderMul = 1.0f;

const int 		RGB8 					= 0;
const int 		RGBA8 					= 0;
const int 		RGBA16 					= 0;
const int 		RGBA16F 				= 0;
const int 		RGBA32F 				= 0;
const int 		RG16 					= 0;
const int 		RGB16 					= 0;
const int 		R11F_G11F_B10F 			= 0;
const int 		colortex0Format 			= RGB8;
const int 		colortex1Format 			= RGBA16;
const int 		colortex2Format 			= RGBA16;
const int 		colortex3Format 			= RGBA16;
const int 		colortex4Format 			= RGBA16F;
const int 		colortex5Format 			= RGBA32F;
const int 		colortex6Format 			= RGBA16F;
const int 		colortex7Format 			= RGBA16;

const int 		superSamplingLevel 		= 0;

const float		sunPathRotation 		= -40.0f;

const int 		noiseTextureResolution  = 64;

const float 	ambientOcclusionLevel 	= 0.06f;

const bool colortex7Clear = false;

const float wetnessHalflife = 100.0;
const float drynessHalflife = 100.0;

in vec4 texcoord;

in vec3 lightVector;
in vec3 worldLightVector;
in vec3 worldSunVector;

in float timeMidnight;

in vec3 colorSunlight;
in vec3 colorSkylight;
in vec3 colorSkyUp;
in vec3 colorTorchlight;

in vec4 skySHR;
in vec4 skySHG;
in vec4 skySHB;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"
#include "lib/Materials.inc"

/////////////////////////FUNCTIONS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////FUNCTIONS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// vec4 GetViewPosition(in vec2 coord, in float depth) 
// {	
// 	vec2 tcoord = coord;
// 	TemporalJitterProjPosInv01(tcoord);

// 	vec4 fragposition = gbufferProjectionInverse * vec4(tcoord.s * 2.0f - 1.0f, tcoord.t * 2.0f - 1.0f, 2.0f * depth - 1.0f, 1.0f);
// 		 fragposition /= fragposition.w;

	
// 	return fragposition;
// }

/////////////////////////STRUCTS///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////STRUCTS///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "lib/GBufferData.inc"

/////////////////////////STRUCT FUNCTIONS//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////STRUCT FUNCTIONS//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

vec3 WorldPosToShadowProjPosBias(vec3 worldPos, vec3 worldNormal, out float dist, out float distortFactor)
{
	vec3 sn = normalize((shadowModelView * vec4(worldNormal.xyz, 0.0)).xyz) * vec3(1, 1, -1);

	vec4 sp = (shadowModelView * vec4(worldPos, 1.0));
	sp = shadowProjection * sp;
	sp /= sp.w;

	dist = sqrt(sp.x * sp.x + sp.y * sp.y);
	distortFactor = (1.0f - SHADOW_MAP_BIAS) + dist * SHADOW_MAP_BIAS;

	sp.xyz += sn * 0.002 * distortFactor;
	sp.xy *= 0.95f / distortFactor;
	sp.z = mix(sp.z, 0.5, 0.8);
	sp = sp * 0.5f + 0.5f;		//Transform from shadow space to shadow map coordinates

	//move to quadrant
	sp.xy *= 0.5;
	sp.xy += 0.5;

	return sp.xyz;
}

vec3 CalculateSunlightVisibility(vec4 screenSpacePosition, MaterialMask materialMask, vec3 worldGeoNormal) {				//Calculates shadows
	if (rainStrength >= 0.99f)
		return vec3(1.0f);

	//if (shadingStruct.directionect > 0.0f) {
		float distance = sqrt(  screenSpacePosition.x * screenSpacePosition.x 	//Get surface distance in meters
							  + screenSpacePosition.y * screenSpacePosition.y
							  + screenSpacePosition.z * screenSpacePosition.z);

		vec4 ssp = screenSpacePosition;

		// if (isEyeInWater > 0.5)
		// {
		// 	ssp.xy *= 0.82;
		// }

		vec3 worldPos = (gbufferModelViewInverse * ssp).xyz;

		// worldPos += worldGeoNormal * 0.04;

		if (materialMask.grass > 0.5)
		{
			worldGeoNormal.xyz = vec3(0, 1, 0);
		}

		float dist;
		float distortFactor;
		vec3 shadowProjPos = WorldPosToShadowProjPosBias(worldPos.xyz, worldGeoNormal, dist, distortFactor);

		// float fademult = 0.15f;
			// shadowMult = clamp((shadowDistance * 1.4f * fademult) - (distance * fademult), 0.0f, 1.0f);	//Calculate shadowMult to fade shadows out

		float shadowMult = 1.0;

		float shading = 0.0;
		vec3 result = vec3(0.0);

		if (shadowMult > 0.0) 
		{

			float diffthresh = dist * 1.0f + 0.10f;
				  diffthresh *= 2.0f / (shadowMapResolution / 2048.0f);
			// diffthresh = 0.0;
				  //diffthresh /= shadingStruct.directionect + 0.1f;

			// shadowProjPos.xyz += shadowNormal * 0.0004 * (dist + 0.5);

			float vpsSpread = 0.105 / distortFactor;

			float avgDepth = 0.0;
			float minDepth = 11.0;
			int c;

			for (int i = -1; i <= 1; i++)
			{
				for (int j = -1; j <= 1; j++)
				{
					vec2 lookupCoord = shadowProjPos.xy + (vec2(i, j) / shadowMapResolution) * 8.0 * vpsSpread;
					//avgDepth += pow(texture2DLod(shadowtex1, lookupCoord, 2).x, 4.1);
					float depthSample = texture2DLod(shadowtex1, lookupCoord, 2).x;
					minDepth = min(minDepth, depthSample);
					avgDepth += pow(min(max(0.0, shadowProjPos.z - depthSample) * 1.0, 0.025), 2.0);
					c++;
				}
			}

			avgDepth /= c;
			avgDepth = pow(avgDepth, 1.0 / 2.0);

			// float penumbraSize = min(abs(shadowProjPos.z - minDepth), 0.15);
			float penumbraSize = avgDepth;

			//if (materialMask.leaves > 0.5)
			//{
				//penumbraSize = 0.02;
			//}

			int count = 0;
			float spread = penumbraSize * 0.055 * vpsSpread + 0.55 / shadowMapResolution;

			vec3 noise = BlueNoiseTemporal(texcoord.st);

			diffthresh *= 0.5 + avgDepth * 50.0;

			const int latSamples = 5;
			const int lonSamples = 5;

			// shadowProjPos.xyz += shadowNormal * diffthresh * 0.001;
			// shadowProjPos.xyz += shadowNormal * diffthresh * 0.001;

			for (int i = 0; i < 25; i++)
			{
				float fi = float(i + noise.x) / 10.0;
				float r = float(i + noise.x) * 3.14159265 * 2.0 * 1.61;

				vec2 radialPos = vec2(cos(r), sin(r));
				vec2 coordOffset = radialPos * spread * sqrt(fi) * 2.0;

				// shading += shadow2DLod(shadowtex0, vec3(shadowProjPos.st + coordOffset, shadowProjPos.z - 0.0012f * diffthresh - (noise.z * 0.00005)), 0).x;
				shading += shadow2DLod(shadowtex0, vec3(shadowProjPos.st + coordOffset, shadowProjPos.z - 0.0012 * dist - (noise.z * 0.00005)), 0).x;
				count += 1;
			}
			shading /= count;

			result = vec3(shading);

			// stained glass shadow
			{
				float stainedGlassShadow = shadow2DLod(shadowtex0, vec3(shadowProjPos.st - vec2(0.5, 0.0), shadowProjPos.z - 0.0012 * diffthresh), 2).x;
				vec3 stainedGlassColor = texture2DLod(shadowcolor, vec2(shadowProjPos.st - vec2(0.5, 0.0)), 2).rgb;
				stainedGlassColor *= stainedGlassColor;
				result = mix(result, result * stainedGlassColor, vec3(1.0 - stainedGlassShadow));

				// result = mix(result, vec3(0.0), vec3(1.0 - stainedGlassShadow));
			}

			// CAUSTICS
			// water shadow (caustics)
			{
				// float waterDepth = abs(texture2DLod(shadowcolor1, shadowProjPos.st - vec2(0.0, 0.5), 4).x * 256.0 - (worldPos.y + cameraPosition.y));
				float waterDepth = abs(texture2DLod(shadowcolor1, shadowProjPos.st - vec2(0.0, 0.5), 3).x * 256.0 - (worldPos.y + cameraPosition.y));

				// float caustics = GetCausticsDeferred(worldPos, waterDepth);
				vec3 caustics = vec3(0.0);
				caustics.r = GetCausticsDeferred(worldPos, 										worldLightVector, waterDepth);
				// caustics.g = GetCausticsDeferred(worldPos + vec3(0.003 * waterDepth, 0.0, 0.0), worldLightVector, waterDepth);
				// caustics.b = GetCausticsDeferred(worldPos + vec3(0.006 * waterDepth, 0.0, 0.0), worldLightVector, waterDepth);
				caustics.g = caustics.r;
				caustics.b = caustics.r;

				float waterShadow = shadow2DLod(shadowtex0, vec3(shadowProjPos.st - vec2(0.0, 0.5), shadowProjPos.z - 0.0012 * diffthresh - noise.z * 0.0001), 3).x;
				result = mix(result, 
					// result * caustics * exp(-GetWaterAbsorption() * waterDepth), 
					result * caustics, 
					vec3(1.0 - waterShadow));
			}
		}

		result = mix(vec3(1.0), result, shadowMult);

		return result;
	//} else {
	//	return vec3(0.0f);
	//}
}

vec3 SubsurfaceScatteringSunlight(vec3 worldNormal, vec3 worldPos, vec3 albedo)
{
	vec4 shadowProjPos = shadowModelView * vec4(worldPos.xyz, 1.0);	//Transform from world space to shadow space
	shadowProjPos = shadowProjection * shadowProjPos;
	shadowProjPos /= shadowProjPos.w;

	float dist = sqrt(shadowProjPos.x * shadowProjPos.x + shadowProjPos.y * shadowProjPos.y);
	float distortFactor = (1.0f - SHADOW_MAP_BIAS) + dist * SHADOW_MAP_BIAS;
	shadowProjPos.xy *= 0.95f / distortFactor;
	shadowProjPos.z = mix(shadowProjPos.z, 0.5, 0.8);
	shadowProjPos = shadowProjPos * 0.5f + 0.5f;		//Transform from shadow space to shadow map coordinates

	//move to quadrant
	shadowProjPos.xy *= 0.5;
	shadowProjPos.xy += 0.5;

	float subsurfaceDepth = 0.0;
	float depthThresh = 0.0005;
	float weights = 0.0;

	vec2 dither = BlueNoiseTemporal(texcoord.st).xy - 0.5;

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			vec2 coordOffset = vec2(i + dither.x, j + dither.y) * 0.001;
			subsurfaceDepth += max(0.0, (shadowProjPos.z - texture2DLod(shadowtex1, shadowProjPos.xy + coordOffset, 0).x) / depthThresh);
			weights += 1.0;
		}
	}

	subsurfaceDepth /= weights;

	// subsurfaceDepth = exp(-subsurfaceDepth * 10.0);

	vec3 subsurfaceColor = 1.0 - (normalize(albedo.rgb + 0.000001) * 0.3);
	// vec3 subsurfaceColor = 1.0 - (albedo.rgb * 0.5);
	// vec3 subsurfaceColor = 1.0 - (albedo.rgb * 0.8);
	// vec3 subsurfaceColor = 1.0 - vec3(0.7, 0.5, 0.1);
	vec3 sss = exp(-subsurfaceDepth * subsurfaceColor * 6.0) * (1.0 - subsurfaceColor);

	return sss * 24.0 * colorSunlight;
}

float ScreenSpaceShadow(vec3 origin, vec3 normal, MaterialMask materialMask)
{
	if (materialMask.sky > 0.5 || rainStrength >= 0.999)
	{
		return 1.0;
	}

	if (isEyeInWater > 0.5)
	{
		origin.xy /= 0.82;
	}

	vec3 viewDir = normalize(origin.xyz);

	float nearCutoff = 0.50;
	float traceBias = 0.015;

	//Prevent self-intersection issues
	float viewDirDiff = dot(fwidth(viewDir), vec3(0.333333));

	vec3 rayPos = origin;
	vec3 rayDir = lightVector * 0.01;
	rayDir *= viewDirDiff * 2000.001;
	rayDir *= -origin.z * 0.28 + nearCutoff;

	rayPos += rayDir * -origin.z * 0.000017 * traceBias;

	float randomness = rand(texcoord.st + sin(frameTimeCounter)).x;

	rayPos += rayDir * randomness;

	float zThickness = 0.025 * -origin.z;

	float shadow = 1.0;

	float numSamplesf = 64.0;
	//numSamplesf /= -origin.z * 0.125 + nearCutoff;

	int numSamples = int(numSamplesf);

	float shadowStrength = 0.9;

	if (materialMask.grass > 0.5)
	{
		shadowStrength = 0.6;
		zThickness *= 2.0;
	}
	if (materialMask.leaves > 0.5)
	{
		shadowStrength = 0.4;
	}

	// shadowStrength = pow(shadowStrength, exp2(-length(origin) * 0.05));

	// vec3 prevRayProjPos = ProjectBack(rayPos);

	for (int i = 0; i < 6; i++)
	{
		float fi = float(i) / float(12);

		rayPos += rayDir;

		vec2 rayProjPos = ProjectBack(rayPos).xy;

		TemporalJitterProjPos01(rayProjPos);

		// vec2 pixelPos = floor(rayProjPos.xy * vec2(viewWidth, viewHeight));
		// vec2 pixelPosPrev = floor(prevRayProjPos.xy * vec2(viewWidth, viewHeight));
		// if (pixelPos.x == pixelPosPrev.x || pixelPos.y == pixelPosPrev.y)
		// {
		// 	continue;
		// }

		// prevRayProjPos = rayProjPos;

		/*
		float sampleDepth = GetDepthLinear(rayProjPos.xy);

		float depthDiff = -rayPos.z - sampleDepth;
		*/

		vec3 samplePos = GetViewPositionNoJitter(rayProjPos.xy, GetDepth(rayProjPos.xy)).xyz;

		float depthDiff = samplePos.z - rayPos.z - 0.02 * -origin.z * traceBias;

		if (depthDiff > 0.0 && depthDiff < zThickness)
		{
			shadow *= 1.0 - shadowStrength;
		}
	}

	return shadow;
}

float OrenNayar(vec3 normal, vec3 eyeDir, vec3 lightDir)
{
	const float PI = 3.14159;
	const float roughness = 0.55;

	// interpolating normals will change the length of the normal, so renormalize the normal.

	// normal = normalize(normal + surface.lightVector * pow(clamp(dot(eyeDir, surface.lightVector), 0.0, 1.0), 5.0) * 0.5);

	// normal = normalize(normal + eyeDir * clamp(dot(normal, eyeDir), 0.0f, 1.0f));

	// calculate intermediary values
	float NdotL = dot(normal, lightDir);
	float NdotV = dot(normal, eyeDir);

	float angleVN = acos(NdotV);
	float angleLN = acos(NdotL);

	float alpha = max(angleVN, angleLN);
	float beta = min(angleVN, angleLN);
	float gamma = dot(eyeDir - normal * dot(eyeDir, normal), lightDir - normal * dot(lightDir, normal));

	float roughnessSquared = roughness * roughness;

	// calculate A and B
	float A = 1.0 - 0.5 * (roughnessSquared / (roughnessSquared + 0.57));

	float B = 0.45 * (roughnessSquared / (roughnessSquared + 0.09));

	float C = sin(alpha) * tan(beta);

	// put it all together
	float L1 = max(0.0, NdotL) * (A + B * max(0.0, gamma) * C);

	//return max(0.0f, surface.NdotL * 0.99f + 0.01f);
	return clamp(L1, 0.0f, 1.0f);
}

float GetCoverage(in float coverage, in float density, in float clouds)
{
	clouds = clamp(clouds - (1.0f - coverage), 0.0f, 1.0f -density) / (1.0f - density);
		clouds = max(0.0f, clouds * 1.1f - 0.1f);
	 clouds = clouds = clouds * clouds * (3.0f - 2.0f * clouds);
	 // clouds = pow(clouds, 1.0f);
	return clouds;
}

float   CalculateSunglow(vec3 npos, vec3 lightVector) {

	float curve = 4.0f;

	vec3 halfVector2 = normalize(-lightVector + npos);
	float factor = 1.0f - dot(halfVector2, npos);

	return factor * factor * factor * factor;
}

float G1V(float dotNV, float k)
{
	return 1.0 / (dotNV * (1.0 - k) + k);
}

vec3 SpecularGGX(vec3 N, vec3 V, vec3 L, float roughness, float F0)
{
	float alpha = roughness * roughness;

	vec3 H = normalize(V + L);

	float dotNL = saturate(dot(N, L));
	float dotNV = saturate(dot(N, V));
	float dotNH = saturate(dot(N, H));
	float dotLH = saturate(dot(L, H));

	float F, D, vis;

	float alphaSqr = alpha * alpha;
	float pi = 3.14159265359;
	float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
	D = alphaSqr / (pi * denom * denom);

	float dotLH5 = pow(1.0f - dotLH, 5.0);
	F = F0 + (1.0 - F0) * dotLH5;

	float k = alpha / 2.0;
	vis = G1V(dotNL, k) * G1V(dotNV, k);

	vec3 specular = vec3(dotNL * D * F * vis) * colorSunlight;

	//specular = vec3(0.1);
	#ifndef PHYSICALLY_BASED_MAX_ROUGHNESS
	specular *= saturate(pow(1.0 - roughness, 0.7) * 2.0);
	#endif

	return specular;
}

#include "lib/VoxelTracing.inc"

// Compatibility wrappers for deobfuscated call sites
int f() { return GetScreenVolumeSide(); }
int n() { return GetShadowVolumeSide(); }
vec3 d(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 v(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 x(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec2 d(vec3 v,int side) { return VolumeCoordToShadowUV(v, side); }
vec3 f(vec3 v,int side) { return WorldOffsetToVolumeCoord(v, side); }
vec3 s(vec3 v) { return WorldOffsetToScreenVolumeCoord(v); }
vec3 d() { return GetCameraVoxelDelta(); }
vec3 e(vec3 v) { return WorldPosToShadowMapCoord(v); }
vec3 d(vec3 v,vec3 i,vec2 s,vec2 n,vec4 f,vec4 z,inout float x,out vec2 r) { return AdjustVoxelHitForBlockShape(v, i, s, n, f, z, x, r); }
VoxelDDA p(Ray v) { return InitVoxelDDA(v); }
void h(inout VoxelDDA v) { StepVoxelDDA(v); }

 void d(in Ray v,in vec3 i[2],out float r,out float z)
 {
   float x,y,f,n;
   r=(i[v.sign[0]].x-v.origin.x)*v.inv_direction.x;
   z=(i[1-v.sign[0]].x-v.origin.x)*v.inv_direction.x;
   x=(i[v.sign[1]].y-v.origin.y)*v.inv_direction.y;
   y=(i[1-v.sign[1]].y-v.origin.y)*v.inv_direction.y;
   f=(i[v.sign[2]].z-v.origin.z)*v.inv_direction.z;
   n=(i[1-v.sign[2]].z-v.origin.z)*v.inv_direction.z;
   r=max(max(r,x),f);
   z=min(min(z,y),n);
 }
 vec3 d(const vec3 v,const vec3 z,vec3 i)
 {
   const float x=1e-05;
   vec3 y=(z+v)*.5,n=(z-v)*.5,s=i-y,r=vec3(0.);
   r+=vec3(sign(s.x),0.,0.)*step(abs(abs(s.x)-n.x),x);
   r+=vec3(0.,sign(s.y),0.)*step(abs(abs(s.y)-n.y),x);
   r+=vec3(0.,0.,sign(s.z))*step(abs(abs(s.z)-n.z),x);
   return normalize(r);
 }
 bool e(const vec3 v,const vec3 z,Ray i,out vec2 r) { return RayAABBIntersect(v, z, i, r); }
bool d(const vec3 v,const vec3 i,Ray z,inout float x,inout vec3 y) { return RayAABBHitCloser(v, i, z, x, y); }
vec3 e(vec3 v,vec3 i,vec3 z,vec3 x,int y) { return SampleShadowedSunlightWithCaustics(v, i, z, x, y); }
vec3 f(vec3 v,vec3 i,vec3 z,vec3 x,int y) { return SampleShadowedSunlightVolume(v, i, z, x, y); }
vec3 h(vec3 v,vec3 i,vec3 z,vec3 x,int y) { return SampleShadowedSunlightStained(v, i, z, x, y); }
vec4 w(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData i(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData c(vec2 v) { return SampleGIBuffer(v); }
float c(float v,float z) { return ReflectionStrengthFromSmoothness(v, z); }
bool c(vec3 v,float z,Ray i,bool s,inout float x,inout vec3 n) { return TraceBlockShape(v, z, i, s, x, n); }

 vec3 y(vec3 v)
 {
   float z=fract(frameCounter*.0123456);
   int i=n(),x=f();
   vec3 y=BlueNoiseTemporal(texcoord.xy).xyz,c=BlueNoiseTemporal(texcoord.xy+.1).xyz,r=v,s=Fract01(cameraPosition.xyz+.5)+vec3(0.,1.7,0.),t=s;
   s=f(s,i);
   Ray m=MakeRay(s*i-vec3(1.),r);
   vec3 w=vec3(1.),G=vec3(0.);
   for(int e=0;
e<1;
e++)
     {
       vec3 a=vec3(floor(m.origin)),h=abs(vec3(length(m.direction))/(m.direction+.0001)),o=sign(m.direction),p=(sign(m.direction)*(a-m.origin)+sign(m.direction)*.5+.5)*h,A;
       vec4 M=vec4(0.);
       vec3 l=vec3(0.);
       float Y=.5;
       for(int R=0;
R<190;
R++)
         {
           l=a/float(i);
           vec2 H=d(l,i);
           M=texture2DLod(shadowcolor,H,0);
           if(abs(M.w*255.-130.)<.5)
             G+=.06125*w*colorTorchlight*Y;
           else
             {
               if(M.w*255.<254.f&&R!=0)
                 {
                   break;
                 }
             }
           A=step(p.xyz,p.yzx)*step(p.xyz,p.zxy);
           p+=A*h;
           a+=A*o;
           Y=1.;
         }
       G+=M.xyz;
     }
   G*=1.;
   return G;
 }
 vec3 e(vec3 z,vec3 x)
 {
   z+=Fract01(cameraPosition.xyz+.5)-.5;
   vec3 y=s(z+x*.1),i=c(v(y)).irradiance;
   return i;
 }
 vec3 c(vec2 v,vec3 i,float z,vec3 n)
 {
   vec3 y=texture2DLod(colortex6,v,0).xyz;
   return y;
 }
 void main()
 {
   GBufferData v=GetGBufferData();
   MaterialMask z=CalculateMasks(v.materialID);
   vec4 i=GetViewPosition(texcoord.xy,v.depth),x=gbufferModelViewInverse*vec4(i.xyz,1.),s=gbufferModelViewInverse*vec4(i.xyz,0.);
   vec3 y=normalize(i.xyz),n=normalize(s.xyz),r=normalize((gbufferModelViewInverse*vec4(v.normal,0.)).xyz),f=normalize((gbufferModelViewInverse*vec4(v.geoNormal,0.)).xyz);
   float t=length(i.xyz);
   vec3 m=vec3(0.);
   if(z.grass>.5)
     r=vec3(0.,1.,0.);
   vec3 w=c(texcoord.xy,v.normal,v.depth,i.xyz)*10.,G=w*v.albedo.xyz;
   const float h=75.;
   if(t>h)
     {
       vec3 e=FromSH(skySHR,skySHG,skySHB,r);
       e=mix(e,vec3(.2)*(dot(r,vec3(0.,1.,0.))*.35+.65)*Luminance(colorSkylight),vec3(rainStrength));
       e*=v.mcLightmap.y;
       vec3 R=e*v.albedo.xyz*4.5;
       const float Y=3.7;
       R+=v.mcLightmap.x*colorTorchlight*v.albedo.xyz*.025*Y;
       vec3 A=normalize(v.albedo.xyz+.0001)*pow(length(v.albedo.xyz),1.)*colorSunlight*.13*v.mcLightmap.y;
       R+=A*v.albedo.xyz*5.;
       float M=.3;
       G=mix(G,R,vec3(saturate(t*M-h*M)));
     }
   m.xyz=G;
   #ifdef HELD_LIGHT
   {
     float e=float(heldBlockLightValue+heldBlockLightValue2)/16.,A=OrenNayar(r,-n,-n),o=1./(dot(s.xyz,s.xyz)+.3);
     m+=v.albedo.xyz*e*o*A*colorTorchlight*.3;
   }
   #endif
   #ifdef VISUALIZE_DANGEROUS_LIGHT_LEVEL
   {
     float e=BlockLightTorchLinear(v.mcLightmap.x)*16.;
     e=e;
     m.x+=e<=6.75?1.:0.;
   }
   #endif
   float e=24.*(1.-rainStrength),o=dot(r,worldLightVector),A=OrenNayar(r,-n,worldLightVector);
   if(z.leaves>.5)
     A=mix(A,.5,.5);
   if(z.grass>.5)
     v.metalness=0.;
   vec3 Y=CalculateSunlightVisibility(i,z,f);
   #ifdef SUNLIGHT_LEAK_FIX
   float M=saturate(v.mcLightmap.y*100.);
   if(isEyeInWater<1)
     Y*=M;
   #endif
   if(isEyeInWater<1)
     Y*=ScreenSpaceShadow(i.xyz,v.normal.xyz,z);
   m+=TintUnderwaterDepth(DoNightEyeAtNight(A*v.albedo.xyz*Y*e*colorSunlight,timeMidnight));
   vec3 R=SpecularGGX(r,-n,worldLightVector,1.-v.smoothness,v.metalness*.98+.02)*e*Y;
   R*=mix(vec3(1.),v.albedo.xyz,vec3(v.metalness));
   if(isEyeInWater<.5)
     m*=1.-c(v.smoothness,v.metalness)*v.metalness,m+=DoNightEyeAtNight(R,timeMidnight);
   if(z.sky>.5||v.depth>1.)
     {
       vec3 p=n.xyz;
       if(isEyeInWater>0)
         p.xyz=refract(p.xyz,vec3(0.,-1.,0.),1.2533);
       vec3 l=SkyShading(p.xyz,worldSunVector.xyz,rainStrength);
       m=l;
       vec3 H=AtmosphereAbsorption(p.xyz);
       m+=v.albedo.xyz*H*.5;
       m+=RenderSunDisc(p,worldSunVector)*H*2000.*(1.-rainStrength);
       CloudPlane(m,-p,worldLightVector,colorSunlight,colorSkyUp,l,timeMidnight,true);
     }
   if(z.glowstone>.5)
     m.xyz+=v.albedo.xyz*GI_LIGHT_BLOCK_INTENSITY;
   if(z.torch>.5)
     m.xyz+=v.albedo.xyz*pow(length(v.albedo.xyz),2.)*.5*GI_LIGHT_TORCH_INTENSITY;
   if(z.lava>.5)
     m+=v.albedo.xyz*.75*GI_LIGHT_BLOCK_INTENSITY;
   if(z.fire>.5)
     m+=v.albedo.xyz*3.*GI_LIGHT_TORCH_INTENSITY;
   if(z.litFurnace>.5)
     {
       float H=saturate(v.albedo.x-(v.albedo.y+v.albedo.z)*.5-.2);
       m+=v.albedo.xyz*H*2.*GI_LIGHT_TORCH_INTENSITY*vec3(2.,.35,.025);
     }
   float H=0.;
   m*=.001;
   m=LinearToGamma(m);
   m+=rand(texcoord.xy+sin(frameTimeCounter))*(1./65535.);
   gl_FragData[0]=vec4(m.xyz,1.);
 }

/* RENDERTARGETS: 3 */
