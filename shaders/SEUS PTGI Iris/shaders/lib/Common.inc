#include "DataPacking.inc"
#include "TAA.inc"






float saturate(float x)
{
	return clamp(x, 0.0, 1.0);
}

vec3 saturate(vec3 x)
{
	return clamp(x, vec3(0.0), vec3(1.0));
}

vec2 saturate(vec2 x)
{
	return clamp(x, vec2(0.0), vec2(1.0));
}

// polynomial smooth min (k = 0.1); https://www.iquilezles.org/www/articles/smin/smin.htm
float SmoothMin( float a, float b, float k )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float SmoothMax( float a, float b, float k )
{
    return -SmoothMin(-a, -b, k);
}

vec3 nozero(vec3 x)
{
	if (abs(x.x) < 0.001)
		x.x = 0.001;

	if (abs(x.y) < 0.001)
		x.y = 0.001;

	if (abs(x.z) < 0.001)
		x.z = 0.001;

	return x;
}

vec3 Fract01(vec3 pos)
{













    return fract(pos - vec3(0.00001)) + vec3(0.00001);

}

vec3 ClipCheck(vec3 color)
{
	color.r = color.r > 1.0 ? 0.0 : color.r;
	color.g = color.g > 1.0 ? 0.0 : color.g;
	color.b = color.b > 1.0 ? 0.0 : color.b;

	return color;
}






vec2 EncodeNormal(vec3 normal)
{
	float p = sqrt(normal.z * 8.0 + 8.0);
	return vec2(normal.xy / p + 0.5);
}

vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 normal;
	normal.xy = fenc * g;
	normal.z = 1.0 - f / 2.0;
	return normal;
}






vec4 GetViewPosition(in vec2 coord, in float depth) 
{	
	vec2 tcoord = coord;
	TemporalJitterProjPosInv01(tcoord);

	vec4 fragposition = gbufferProjectionInverse * vec4(tcoord.s * 2.0f - 1.0f, tcoord.t * 2.0f - 1.0f, 2.0f * depth - 1.0f, 1.0f);
		 fragposition /= fragposition.w;
	
	return fragposition;
}

vec4 GetViewPositionNoJitter(in vec2 coord, in float depth) 
{	
	vec4 tcoord = vec4(coord.xy, 0.0, 0.0);

	vec4 fragposition = gbufferProjectionInverse * vec4(tcoord.s * 2.0f - 1.0f, tcoord.t * 2.0f - 1.0f, 2.0f * depth - 1.0f, 1.0f);
		 fragposition /= fragposition.w;

	
	return fragposition;
}

vec3 ProjectBack(vec3 cameraSpace) 
{
    vec4 clipSpace = gbufferProjection * vec4(cameraSpace, 1.0);
    vec3 NDCSpace = clipSpace.xyz / clipSpace.w;
    vec3 screenSpace = 0.5 * NDCSpace + 0.5;
		 //screenSpace.z = 0.1f;
    return screenSpace;
}


vec3 WorldPosToShadowProjPos(vec3 worldPos, out float dist, out float distortFactor)
{
	vec4 sp = (shadowModelView * vec4(worldPos, 1.0));
	sp = shadowProjection * sp;
	sp /= sp.w;

	dist = sqrt(sp.x * sp.x + sp.y * sp.y);
	distortFactor = (1.0f - SHADOW_MAP_BIAS) + dist * SHADOW_MAP_BIAS;
	sp.xy *= 0.95f / distortFactor;
	sp.z = mix(sp.z, 0.5, 0.8);
	sp = sp * 0.5f + 0.5f;		//Transform from shadow space to shadow map coordinates


	//move to quadrant
	sp.xy *= 0.5;
	sp.xy += 0.5;

	return sp.xyz;
}



float 	ExpToLinearDepth(in float depth)
{






		vec2 a = vec2(depth * 2.0 - 1.0, 1.0);

		// vec2 d = vec2(dot(vec2(gbufferProjectionInverse[2].z, gbufferProjectionInverse[3].z), a),
		// 			  dot(vec2(gbufferProjectionInverse[2].w, gbufferProjectionInverse[3].w), a));

		mat2 ipm = mat2(gbufferProjectionInverse[2].z, gbufferProjectionInverse[2].w,
						gbufferProjectionInverse[3].z, gbufferProjectionInverse[3].w);

		vec2 d = ipm * a;

		d.x /= d.y;

		return -d.x;










}

float GetDepthLinear(in vec2 coord) 
{					
	// return (near * far) / (texture2D(depthtex1, coord).x * (near - far) + far);
	return ExpToLinearDepth(texture2D(depthtex1, coord).x);
}

float GetDepth(vec2 coord)
{
	return texture2D(depthtex1, coord).x;
}

vec3 GetNormals(vec2 coord)
{
	return DecodeNormal(texture2D(colortex2, coord).xy);
}









vec4 SampleLinear(sampler2D tex, vec2 coord)
{
	return pow(texture2D(tex, coord), vec4(2.2));
}

vec3 LinearToGamma(vec3 c)
{
	return pow(c, vec3(1.0 / 2.2));
}

vec3 GammaToLinear(vec3 c)
{
	return pow(c, vec3(2.2));
}





float curve(float x)
{
	return x * x * (3.0 - 2.0 * x);
}

float Luminance(in vec3 color)
{
	return dot(color.rgb, vec3(0.2125f, 0.7154f, 0.0721f));
}

vec3 CurveChroma(vec3 color, float p)
{
	return length(color) * pow(normalize(color + 0.00001), vec3(p));
}




















vec3 rand(vec2 coord)
{
	float noiseX = saturate(fract(sin(dot(coord, vec2(12.9898, 78.223))) * 43758.5453));
	float noiseY = saturate(fract(sin(dot(coord, vec2(12.9898, 78.223)*2.0)) * 43758.5453));
	float noiseZ = saturate(fract(sin(dot(coord, vec2(12.9898, 78.223)*3.0)) * 43758.5453));

	return vec3(noiseX, noiseY, noiseZ);
}

vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);
}

vec3 BlueNoiseTemporal(vec2 coord)
{
	vec2 noiseCoord = vec2(coord.st * vec2(viewWidth, viewHeight)) / 64.0;
	// //noiseCoord += vec2(frameCounter, frameCounter);
	// //noiseCoord += mod(frameCounter, 16.0) / 16.0;
	// //noiseCoord += rand(vec2(mod(frameCounter, 16.0) / 16.0, mod(frameCounter, 16.0) / 16.0) + 0.5).xy;
	// noiseCoord += vec2(sin(frameCounter * 0.75), cos(frameCounter * 0.75));

	// noiseCoord = (floor(noiseCoord * 64.0) + 0.5) / 64.0;

	vec3 irrationals = vec3(sqrt(1.0 / 5.0), sqrt(2.0), 1.61803398) * 1.0;

	vec3 n = texture2D(noisetex, noiseCoord).rgb;

	n = mod(n + irrationals * mod(frameCounter, 64.0f), vec3(1.0));

	return n;
}

float Get2DNoise(in vec2 pos)
{
	pos.xy += 0.5f;

	vec2 p = floor(pos);
	vec2 f = fract(pos);

	#ifdef CLOUDS_BLOCKY
	for (int i = 0; i < 3; i++)
	#endif
	{
		f.x = f.x * f.x * (3.0f - 2.0f * f.x);
		f.y = f.y * f.y * (3.0f - 2.0f * f.y);
	}

	vec2 uv =  p.xy + f.xy;

	// uv -= 0.5f;
	// uv2 -= 0.5f;

	vec2 coord =  (uv  + 0.5f) / 64.0;
	float xy1 = texture2D(noisetex, coord).x;
	return xy1;
}

float Get3DNoise(in vec3 pos)
{
	pos.z += 0.0f;

	pos.xyz += 0.5f;

	vec3 p = floor(pos);
	vec3 f = fract(pos);

	// f.x = f.x * f.x * (3.0f - 2.0f * f.x);
	// f.y = f.y * f.y * (3.0f - 2.0f * f.y);
	// f.z = f.z * f.z * (3.0f - 2.0f * f.z);

	vec2 uv =  (p.xy + p.z * vec2(17.0f)) + f.xy;
	vec2 uv2 = (p.xy + (p.z + 1.0f) * vec2(17.0f)) + f.xy;

	// uv -= 0.5f;
	// uv2 -= 0.5f;

	vec2 coord =  (uv  + 0.5f) / 64.0;
	vec2 coord2 = (uv2 + 0.5f) / 64.0;
	float xy1 = texture2D(noisetex, coord).x;
	float xy2 = texture2D(noisetex, coord2).x;
	return mix(xy1, xy2, f.z);
}


/* https://www.shadertoy.com/view/XsX3zB
 *
 * The MIT License
 * Copyright © 2013 Nikita Miropolskiy
 * 
 * ( license has been changed from CCA-NC-SA 3.0 to MIT
 *
 *   but thanks for attributing your source code when deriving from this sample 
 *   with a following link: https://www.shadertoy.com/view/XsX3zB )
 *
 * ~
 * ~ if you're looking for procedural noise implementation examples you might 
 * ~ also want to look at the following shaders:
 * ~ 
 * ~ Noise Lab shader by candycat: https://www.shadertoy.com/view/4sc3z2
 * ~
 * ~ Noise shaders by iq:
 * ~     Value    Noise 2D, Derivatives: https://www.shadertoy.com/view/4dXBRH
 * ~     Gradient Noise 2D, Derivatives: https://www.shadertoy.com/view/XdXBRH
 * ~     Value    Noise 3D, Derivatives: https://www.shadertoy.com/view/XsXfRH
 * ~     Gradient Noise 3D, Derivatives: https://www.shadertoy.com/view/4dffRH
 * ~     Value    Noise 2D             : https://www.shadertoy.com/view/lsf3WH
 * ~     Value    Noise 3D             : https://www.shadertoy.com/view/4sfGzS
 * ~     Gradient Noise 2D             : https://www.shadertoy.com/view/XdXGW8
 * ~     Gradient Noise 3D             : https://www.shadertoy.com/view/Xsl3Dl
 * ~     Simplex  Noise 2D             : https://www.shadertoy.com/view/Msf3WH
 * ~     Voronoise: https://www.shadertoy.com/view/Xd23Dh
 * ~ 
 *
 */

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) {
  float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
  vec3 r;
  r.z = fract(512.0*j);
  j *= .125;
  r.x = fract(512.0*j);
  j *= .125;
  r.y = fract(512.0*j);
  return r-0.5;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333;
const float G3 =  0.1666667;

/* 3d simplex noise */
float simplex3d(vec3 p) {
   /* 1. find current tetrahedron T and it's four vertices */
   /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
   /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
   
   /* calculate s and x */
   vec3 s = floor(p + dot(p, vec3(F3)));
   vec3 x = p - s + dot(s, vec3(G3));
   
   /* calculate i1 and i2 */
   vec3 e = step(vec3(0.0), x - x.yzx);
   vec3 i1 = e*(1.0 - e.zxy);
   vec3 i2 = 1.0 - e.zxy*(1.0 - e);
    
   /* x1, x2, x3 */
   vec3 x1 = x - i1 + G3;
   vec3 x2 = x - i2 + 2.0*G3;
   vec3 x3 = x - 1.0 + 3.0*G3;
   
   /* 2. find four surflets and store them in d */
   vec4 w, d;
   
   /* calculate surflet weights */
   w.x = dot(x, x);
   w.y = dot(x1, x1);
   w.z = dot(x2, x2);
   w.w = dot(x3, x3);
   
   /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
   w = max(0.6 - w, 0.0);
   
   /* calculate surflet components */
   d.x = dot(random3(s), x);
   d.y = dot(random3(s + i1), x1);
   d.z = dot(random3(s + i2), x2);
   d.w = dot(random3(s + 1.0), x3);
   
   /* multiply d by w^4 */
   w *= w;
   w *= w;
   d *= w;
   
   /* 3. return the sum of the four surflets */
   return dot(d, vec4(52.0));
}





















vec4 ToSH(float value, vec3 dir)
{
	const float PI = 3.14159265359;
	const float N1 = sqrt(4 * PI / 3);
	const float transferl1 = (sqrt(PI) / 3.0) * N1;
	const float transferl0 = PI;

	const float sqrt1OverPI = sqrt(1.0 / PI);
	const float sqrt3OverPI = sqrt(3.0 / PI);

	vec4 coeffs;

	coeffs.x = 0.5 * sqrt1OverPI * value * transferl0;
	coeffs.y = -0.5 * sqrt3OverPI * dir.y * value * transferl1;
	coeffs.z = 0.5 * sqrt3OverPI * dir.z * value * transferl1;
	coeffs.w = -0.5 * sqrt3OverPI * dir.x * value * transferl1; //TODO: Vectorize the math so it's faster

	return coeffs;
}


vec3 FromSH(vec4 cR, vec4 cG, vec4 cB, vec3 lightDir)
{
	const float PI = 3.14159265;

	const float N1 = sqrt(4 * PI / 3);
	const float transferl1 = (sqrt(PI) / 3.0) * N1;
	const float transferl0 = PI;

	const float sqrt1OverPI = sqrt(1.0 / PI);
	const float sqrt3OverPI = sqrt(3.0 / PI);

	vec4 sh;

	sh.x = 0.5 * sqrt1OverPI;
	sh.y = -0.5 * sqrt3OverPI * lightDir.y;
	sh.z = 0.5 * sqrt3OverPI * lightDir.z;
	sh.w = -0.5 * sqrt3OverPI * lightDir.x;

	vec3 result;
	result.r = sh.x * cR.x;
	result.r += sh.y * cR.y;
	result.r += sh.z * cR.z;
	result.r += sh.w * cR.w;

	result.g = sh.x * cG.x;
	result.g += sh.y * cG.y;
	result.g += sh.z * cG.z;
	result.g += sh.w * cG.w;

	result.b = sh.x * cB.x;
	result.b += sh.y * cB.y;
	result.b += sh.z * cB.z;
	result.b += sh.w * cB.w;

	return result.rgb;
}






struct Ray 
{
    vec3 origin;
    vec3 direction;
    vec3 inv_direction;
    ivec3 sign;
};

Ray MakeRay(vec3 origin, vec3 direction) 
{
    vec3 inv_direction = vec3(1.0) / direction;
    return Ray(
        origin,
        direction,
        inv_direction,
        ivec3((inv_direction.x < 0) ? 1 : 0,
            (inv_direction.y < 0) ? 1 : 0,
            (inv_direction.z < 0) ? 1 : 0
        )
    );
}







vec3 DoNightEyeAtNight(in vec3 color, float timeFactor)
{
	float luminance = Luminance(color);

	float rodFactor = 1.0f / (luminance * 1000.0 + 1.0);

	color = mix(color, luminance * vec3(0.2, 0.4, 0.9) * 1.5, vec3(0.6 * rodFactor * timeFactor));

	return color;
}

vec3 GetWaterFogColor()
{
	// return vec3(0.2, 0.8, 1.0);
	// return vec3(0.8, 0.9, 1.0) * 0.7;
	return vec3(0.8, 0.9, 1.0) * 0.5;
	// return vec3(1.0, 1.9, 0.4);
}

vec3 GetWaterAbsorption()
{
	// return vec3(1.0, 0.22, 0.005);
	// return vec3(1.0, 0.32, 0.05);

	// return vec3(0.25, 0.03, 0.01);// * 0.0;
	return vec3(0.25, 0.04, 0.01);// * 0.0;
}

vec3 TintUnderwaterDepth(vec3 color)
{
	// return color;
	if (isEyeInWater > 0)
	{
		float underwaterDepth = 1.0 - (eyeBrightnessSmooth.y / 240.0);
		color *= exp((-GetWaterAbsorption()) * (underwaterDepth * 8.0 + 0.0));
	}

	return color;
}

vec3 TintUnderwaterMCSkylight(vec3 color, float mcSkylight)
{
	return color;
	if (isEyeInWater > 0)
	{
		float underwaterDepth = 1.0 - mcSkylight;
		color *= exp(-GetWaterAbsorption() * underwaterDepth * 1.0);
	}

	return color;
}



void UnderwaterFog(inout vec3 color, float eyeLength, vec3 eyeDir, vec3 skyColor, vec3 sunColor)
{
	if (isEyeInWater == 0)
	return;

	vec3 fogColor = GetWaterFogColor() * 0.05;
	// fogColor *= exp(-GetWaterAbsorption() * ((1.0 - (eyeBrightnessSmooth.y / 240.0)) * 4.0 + (-eyeDir.y * 0.5 + 0.5) * 3.0));
	fogColor *= exp(-GetWaterAbsorption() * ((-eyeDir.y * 0.5 + 0.5) * 3.0));
	fogColor *= pow(curve(saturate(mix(eyeDir.y, 1.0, 0.5))), 2.0) * 0.6 + 0.4;

	// fogColor *= (skyColor * 0.1 + sunColor * 2.0);
	fogColor *= (skyColor * 1.0 + sunColor * 2.0) * 1.4;
	fogColor = TintUnderwaterDepth(fogColor);
	// fogColor *= eyeBrightnessSmooth.y / 240.0;

	float fogFactor = (exp(-eyeLength * 0.03));
	fogFactor = saturate(fogFactor * 1.1);
	fogFactor = 1.0 - pow(1.0 - fogFactor, 2.0);

	color *= exp(-GetWaterAbsorption() * eyeLength);

	// fogColor *= exp(-GetWaterAbsorption() * min(eyeLength * 1.0, 30.2));
	// fogColor *= exp(-GetWaterAbsorption() * 30.2);
	fogColor *= exp(-GetWaterAbsorption() * (1.0 - fogFactor) * 20.0);
	color = mix(fogColor, color, vec3(fogFactor));
}







vec4 GetCausticsTexComposite(vec2 coord)
{
	vec2 lookupCoord = vec2(coord.x, (coord.y - floor(mod(FRAME_TIME * 60.0f, 60.0f))) / 60.0f);
	return textureLod(colortex4, lookupCoord.st, 0);
}

float GetCausticsComposite(vec3 worldPos, vec3 worldLightVector, float waterDepth)
{
	vec3 causticPos = worldPos.xyz + cameraPosition.xyz;
	vec3 refractLightVector = refract(worldLightVector, vec3(0.0, 1.0, 0.0), 1.0 / 1.333);
	causticPos += refractLightVector * ((worldPos.y + cameraPosition.y) / (refractLightVector.y));

	vec4 causticsTex = GetCausticsTexComposite(mod((causticPos.xz / 2.0), vec2(1.0)));

	float depthBlend = pow(waterDepth / 2.0, 0.5);

	float caustics = pow(causticsTex.r, saturate(depthBlend * 0.5 + 0.5));
	caustics = mix(caustics, causticsTex.g, saturate(depthBlend - 1.0));
	caustics = mix(caustics, causticsTex.b, saturate(depthBlend - 2.0));
	caustics = mix(caustics, causticsTex.a, saturate(depthBlend - 3.0));

	return caustics * 10.0;
}

float GetCausticsDeferred(vec3 worldPos, vec3 worldLightVector, float waterDepth)
{
	vec3 causticPos = worldPos.xyz + cameraPosition.xyz;
	vec3 refractLightVector = refract(worldLightVector, vec3(0.0, 1.0, 0.0), 1.0 / 1.333);
	causticPos += refractLightVector * ((worldPos.y + cameraPosition.y) / (refractLightVector.y));
	// causticPos.x += 0.3;

	float caustic0 = texture2D(colortex6, mod((causticPos.xz / 2.0) * 0.25, vec2(0.249))).a;
	float caustic1 = texture2D(colortex6, mod((causticPos.xz / 2.0) * 0.25, vec2(0.249)) + vec2(0.25, 0.0)).a;
	float caustic2 = texture2D(colortex6, mod((causticPos.xz / 2.0) * 0.25, vec2(0.249)) + vec2(0.5, 0.0)).a ;
	float caustic3 = texture2D(colortex6, mod((causticPos.xz / 2.0) * 0.25, vec2(0.249)) + vec2(0.75, 0.0)).a;

	// float depthBlend = pow(waterDepth / 4.0, 1.0 / 1.5);
	float depthBlend = pow(waterDepth / 2.0, 0.5);

	// float caustics = mix(1.0, caustic0, saturate(depthBlend));

	// float caustics = mix(caustic0, caustic1, saturate(depthBlend));
	// caustics = mix(caustics, caustic2, saturate(depthBlend - 1.0));
	// caustics = mix(caustics, caustic3, saturate(depthBlend - 2.0));

	float caustics = pow(caustic0, saturate(depthBlend * 0.5 + 0.5));
	caustics = mix(caustics, caustic1, saturate(depthBlend - 1.0));
	caustics = mix(caustics, caustic2, saturate(depthBlend - 2.0));
	caustics = mix(caustics, caustic3, saturate(depthBlend - 3.0));

	return caustics * 10.0;
}







vec2 IntersectSphere( in vec3 ro, in vec3 rd, in vec3 ce, float ra )
{
    vec3 oc = ro - ce;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - ra*ra;
    float h = b*b - c;
    if( h < 0.0 ) return vec2(-1.0); // no intersection
    h = sqrt( h );
    return vec2( -b-h, -b+h );
}













// https://pubs.rsc.org/en/content/articlehtml/2019/nr/c9nr01707k
// 
// Mie
// g : ( -0.75, -0.999 )
//      3 * ( 1 - g^2 )               1 + c^2
// F = ----------------- * -------------------------------
//      2 * ( 2 + g^2 )     ( 1 + g^2 - 2 * g * c )^(3/2)
float PhaseMie( float g, float LdotV, float LdotV2 ) {
	float gg = g * g;
	
	float a = ( 1.0 - gg ) * ( 1.0 + LdotV2 );

	float b = 1.0 + gg - 2.0 * g * LdotV;
	b *= sqrt( b );
	b *= 2.0 + gg;	
	
	return 1.5 * a / b;
}

float PhaseMiePRM(float LdotV, float g, float a)
{
	float gg = g*g;

	return 10.0 * (a*g*pow(1.0 - gg, 2.0*a))    /    (3.14159 * pow(1.0 + gg - 2.0*g*LdotV, a + 1.0) * (pow(1.0 + g, 2.0*a) - pow(1.0 - g, 2.0*a)));
}

// Reyleigh
// g : 0
// F = 3/4 * ( 1 + c^2 )
float PhaseRayleigh( float LdotV2 ) 
{
	return 0.75 * ( 1.0 + LdotV2 );
}


float TotalAtmosphereDensity(float dirY, const bool considerLand)
{
	// return 1.0 / (max(0.0, dirY + 0.06) * 3.0 + 0.00001);

	if (!considerLand || (considerLand && dirY > 0.0))
	{
		return 1.0 / (max(0.0, dirY) * 6.0 + 0.03);
	}
	else
	{
		return 1.0 / (max(0.0, -dirY) * 50.0 + 0.4);
	}
}

// const vec3 Rayleigh = pow(vec3(0.17, 0.44, 0.99), vec3(1.0));
// const vec3 Rayleigh = pow(vec3(0.17, 0.46, 0.99), vec3(1.0));
// const vec3 Rayleigh = pow(vec3(0.17, 0.43, 0.99), vec3(1.0));


// const vec3 Rayleigh = pow(vec3(0.12, 0.39, 0.99), vec3(1.0));

// const vec3 Rayleigh = pow(vec3(0.17, 0.41, 0.99), vec3(1.2));

// const vec3 Rayleigh = vec3( 3.8, 13.5, 33.1 ) * 0.05;

const vec3 Rayleigh = pow(vec3(0.17, 0.40, 0.99), vec3(1.0));
// const vec3 Rayleigh = pow(vec3(0.17, 0.40, 0.69), vec3(1.0));


vec3 SunlightColorFromSunVector(vec3 worldSunVector)
{
	float thickness = 1.0 / (max(0.0, worldSunVector.y) * 6.0 + 0.01);

	vec3 color = exp(-thickness * 1.0 * (Rayleigh + 0.3));

	// color *= pow(saturate(worldSunVector.y), 0.125);

	return color;
}

vec3 Atmosphere(vec3 worldViewVector, vec3 worldSunVector, float mieAmount, float depthFactor)
{
	// return vec3(1.0);

	float LdotV = dot(worldViewVector, worldSunVector);
	float LdotV2 = LdotV * LdotV;


	vec3 color = vec3(0.0, 0.0, 0.0);

	
	vec3 r = Rayleigh;
	float d = 0.3;
	float h = 1.0;
	float v = worldViewVector.y;
	float m = 0.1;
	float s = worldSunVector.y;
	float q = PhaseMie(0.7, LdotV, LdotV2);

	// Fix sunset
	s = SmoothMax(0.01, s, 0.07);
	v = pow(abs(v), 1.0 + saturate(-LdotV * 0.5 + 0.5) * 0.0005 / (s + 0.0005)) * sign(v);
	float energyFade = exp(min(0.0, worldSunVector.y) * 100.0);

	float x = depthFactor;
	float sMax = 1000000;

	float t1 = (exp(-sMax*s) - 1)*v + s;
	vec3 mpr = m + r;
	vec3 dmpr = d * mpr;
	vec3 mprt1 = mpr*t1;
	vec3 C =  (s*exp((dmpr*(t1/s - 1))/v)) / mprt1;
	vec3 atmos = -(s*exp((dmpr*((t1*exp(-v*x))/s - 1))/v)) / mprt1 + C;
	atmos *= energyFade;
	color = max(
			atmos * pow(r, vec3(1.0)) * PhaseMie(0.0, LdotV, LdotV2)  // g of 0 is like rayleigh
			+ atmos * m * 0.25 * PhaseMie(0.82, LdotV, LdotV2)
			+ atmos * m * 1.0 * PhaseMie(0.5, LdotV, LdotV2)
		, vec3(0.0));


	float t = x;
	float k = sMax;


	// Multiple scattering. Sky atmosphere but with no sunlight absorption
	{
		s = 0.5;
		m = 0.2;

		t1 = ((exp(-sMax*s) - 1)*v + s);
		mpr = m + r;
		dmpr = d * mpr;
		mprt1 = mpr * t1;

		vec3 C2 =  (s*exp((dmpr*((t1        )/s - 1))/v)) / mprt1;
		vec3 ms = -(s*exp((dmpr*((t1*exp(-v*x))/s - 1))/v)) / mprt1 + C2;
		color += max(vec3(0.0), ms * r) * exp(min(0.0, worldSunVector.y) * 150.0) * 0.1;
	}


	color *= 7.0 / SUNLIGHT_BRIGHTNESS;


	return color;
}


vec3 Atmosphere(vec3 worldViewVector, vec3 worldSunVector, float mieAmount)
{
	return Atmosphere(worldViewVector, worldSunVector, mieAmount, 40.0);
}



vec3 AtmosphereAbsorption(vec3 worldViewVector)
{
	worldViewVector.y = max(worldViewVector.y, 0.0);

	float thickness = 1.0 / ((worldViewVector.y) * (1.0) + 0.15);
	float thicknessHaze = 1.0 / ((worldViewVector.y) * (10.0));

	return exp(-thickness * (Rayleigh) * 0.7) * exp(-thicknessHaze);
}


// Main sky shading function
vec3 SkyShading(vec3 worldViewVector, vec3 worldSunVector, float rainStrength)
{
	vec3 atmosphere = Atmosphere(worldViewVector, worldSunVector, 1.0);
	atmosphere += Atmosphere(worldViewVector, -worldSunVector, 1.0) * NIGHT_BRIGHTNESS;

	// rainy
	vec3 rainSky = vec3(0.5) * (exp(-TotalAtmosphereDensity(worldSunVector.y, false)) + 1.0 * NIGHT_BRIGHTNESS);
	atmosphere = mix(atmosphere, rainSky, vec3(rainStrength));

	// return vec3(0.0);
	return atmosphere;
}

vec3 AtmosphericScattering(vec3 rayDir, vec3 lightVector, float mieAmount)
{
	return Atmosphere(rayDir, lightVector, mieAmount);
}

vec3 AtmosphericScattering(vec3 rayDir, vec3 lightVector, float mieAmount, float depth)
{
	float LdotV = dot(lightVector, rayDir);
	float LdotV2 = LdotV * LdotV;

	rayDir.y = rayDir.y * 0.5 + 0.5;
	rayDir = normalize(rayDir);

	// vec3 atmosphere = Atmosphere(rayDir, lightVector, 0.0);
	vec3 atmosphere = Rayleigh * 1.268 * exp(-TotalAtmosphereDensity(lightVector.y + 0.02, false) * 0.4 );
	atmosphere += PhaseMie(0.78, LdotV, LdotV2) * SunlightColorFromSunVector(lightVector) * mieAmount;

	atmosphere *= 1.0 - exp(-depth * 500.0);

	atmosphere *= saturate(lightVector.y * 10.0);

	return atmosphere;
	// return vec3(0.0);
}

vec3 AtmosphericScatteringSingle(vec3 rayDir, vec3 lightVector, float mieAmount)
{
	return AtmosphereAbsorption(rayDir);
}

vec3 ModulateSkyForRain(vec3 skyColor, vec3 colorSkylight, float rainStrength)
{
	skyColor = mix(skyColor, vec3(1.0) * Luminance(colorSkylight), vec3(rainStrength * 0.95));

	return skyColor;
}

float RenderSunDisc(vec3 worldDir, vec3 sunDir)
{
	float d = dot(worldDir, sunDir);

	float disc = 0.0;

	//if (d > 0.99)
	//	disc = 1.0;

	float size = 0.00105;
	float hardness = 2000.0;

	disc = pow(curve(saturate((d - (1.0 - size)) * hardness)), 2.0);

	float visibility = curve(saturate(worldDir.y * 30.0));

	disc *= visibility;

	return disc;
}




















void LandAtmosphericScattering(inout vec3 color, in vec3 viewPos, in vec3 viewDir, 
	vec3 worldDir, vec3 worldSunVector, float rayFactor)
{
	// return;
	float dist = length(viewPos);

	#ifdef FADE_TO_ATMOSPHERE
		float depthFactor = length(viewPos) / far;
		depthFactor = pow(depthFactor, 6.0) * 0.07 + pow(depthFactor, 2.0) * 0.003;
	#else
		float depthFactor = length(viewPos) / 300.0;
		depthFactor = pow(depthFactor, 3.0) * 0.002;
	#endif

		depthFactor *= pow(eyeBrightnessSmooth.y / 240.0f, 6.0f);


	depthFactor = clamp(depthFactor, 0.0, 0.03);
	depthFactor /= saturate(worldDir.y) * 2.0 + 1.0;

	vec3 absorption = vec3(0.2, 0.45, 1.0);

	color *= exp(-depthFactor * absorption * 100.67);
	// color += max(vec3(0.0), vec3(1.0) - exp(-fogFactor * absorption)) * mix(colorSunlight, vec3(dot(colorSunlight, vec3(0.33333))), vec3(0.9)) * 7.0;

	// float VdotL = dot(viewDir, sunVector);

	// float g = 0.77;
	// 			//float g = 0.9;
	// float g2 = g * g;
	// float theta = VdotL * 0.5 + 0.5;
	// float anisoFactor = 1.5 * ((1.0 - g2) / (2.0 + g2)) * ((1.0 + theta * theta) / (1.0 + g2 - 2.0 * g * theta)) + g * theta;

	

	// color += colorSunlight * fogFactor * 0.2 * anisoFactor;

	// float depthFactor = length(viewPos) / far;
	// depthFactor = pow(depthFactor, 2.0) * 0.04;

	// depthFactor = min(depthFactor, 0.04);

	#ifdef GODRAYS
	color += AtmosphericScattering(normalize(worldDir), worldSunVector, 0.0, depthFactor) * 0.9 * mix(rayFactor, 1.0, 0.5) * mix(1.0, 0.35, wetness) * 0.1 * (1.0 - rainStrength);
	#else
	color += AtmosphericScattering(normalize(worldDir), worldSunVector, 1.0, depthFactor) * 0.9 * mix(rayFactor, 1.0, 0.5) * mix(1.0, 0.35, wetness) * 0.1 * (1.0 - rainStrength);
	#endif

}






struct Plane {
	vec3 normal;
	vec3 origin;
};

struct Intersection {
	vec3 pos;
	float distance;
	float angle;
};


Intersection 	RayPlaneIntersectionWorld(in Ray ray, in Plane plane)
{
	float rayPlaneAngle = dot(ray.direction, plane.normal);

	float planeRayDist = 100000000.0f;
	vec3 intersectionPos = ray.direction * planeRayDist;

	if (rayPlaneAngle > 0.0001f || rayPlaneAngle < -0.0001f)
	{
		planeRayDist = dot((plane.origin), plane.normal) / rayPlaneAngle;
		intersectionPos = ray.direction * planeRayDist;
		intersectionPos = -intersectionPos;

		intersectionPos += cameraPosition.xyz;
	}

	Intersection i;

	i.pos = intersectionPos;
	i.distance = planeRayDist;
	i.angle = rayPlaneAngle;

	return i;
}


#define CLOUD_SPEED 0.5

float CloudDensity(vec3 pos, const int level, const float lunacrity)
{
	float cloudDist =length(pos.xz - cameraPosition.xz);
	pos /= cloudDist * 0.00015 + 1.0;
	pos *= 0.002;

	float n = 0.0;
	float g = 1.0;
	float w = 0.0;
	for (int i = 0; i < level; i++)
	{
		n += Get2DNoise(pos.xz) * g;
		w += g;

		pos.x += FRAME_TIME * 0.02 * CLOUD_SPEED;
		pos *= 3.0;
		g *= lunacrity;
	}

	n /= w;

	// n += 0.0 + cloudDist * 0.00001;

	#ifdef CLOUDS_BLOCKY
	#else
	n = n * 1.5 - 0.25;
	#endif
	n = saturate(n * 1.0 - mix(0.4, 0.0, rainStrength));
	// n = saturate(n * 1.0 - (sin(FRAME_TIME) * 0.5 + 0.5) * 0.5);

	return n;
}

float TraceCloudDensity(vec3 pos, vec3 lightDir, float perturbNoise, float cloudDensity, float cloudDist)
{
	float shadowFactor = 0.0;
	for (int i = 1; i <= 2; i++)
	{
		vec3 sp = pos + i * 350.0 * lightDir * (1.0 + perturbNoise * 0.025 * 0) * saturate(cloudDensity * 1.0);
		float d = max(0.0, CloudDensity(sp, 3, 0.23) - cloudDist * 0.00005);

		shadowFactor += d;
	}
	return shadowFactor;
}

vec3 CirrusSunlightColor(vec3 worldLightVector, vec3 colorSunlight)
{
	// return pow(colorSunlight, vec3(0.25));
	return colorSunlight;
}

vec4 CloudColor(vec3 pos, vec3 worldDir, vec3 worldLightVector, vec3 colorSunlight, 
	vec3 colorSkyUp, vec3 atmosphere, const bool detail)
{
	#ifndef CUMULUS_CLOUDS
	return vec4(0.0);
	#endif

	float LdotV = dot(worldDir, -worldLightVector);
	float LdotV01 = LdotV * 0.5 + 0.5;
	float LdotV2 = LdotV * LdotV;
	float cloudDist = length(pos.xz - cameraPosition.xz);

	float cloudDensity = CloudDensity(pos, 3, 0.23);

	float perturbNoise = 2.0;
	{
		if (detail)
		{
			vec3 motion = vec3(FRAME_TIME * 0.01 * CLOUD_SPEED, 0, 0);
			vec3 ppos = worldDir + Get3DNoise((worldDir) * 50.0) * 0.01;
			perturbNoise =  (1.0 - Get3DNoise((ppos - motion) * 50.0)) * 2.0;
			perturbNoise += (1.0 - Get3DNoise((ppos - motion) * 100.0)) * 1.0;
			perturbNoise += (1.0 - Get3DNoise((ppos - motion) * 200.0)) * 0.5;
			perturbNoise += (1.0 - Get3DNoise((ppos - motion) * 400.0)) * 0.25;
		}
		perturbNoise *= cloudDist * 0.002 + 1.0;
		// perturbNoise -= cloudDist * 0.0009 + 0.0;
	}

	// perturbNoise *= 1.2;


	cloudDensity = max(0.0, cloudDensity - perturbNoise * 0.005);

	// return vec4(cloudDensity);



	// For sunlight
	float sunFlux = 0.0;
	float shadowFactor = 0.0;
	{
		vec3 lightDir = normalize(worldLightVector + worldDir * 1.0);
		// lightDir += worldDir * cloudDist * 0.0001;
		lightDir *= pow(-LdotV * 0.5 + 0.5, 0.25);

		shadowFactor = TraceCloudDensity(pos + worldDir * cloudDensity * 0.1 * cloudDist, lightDir, perturbNoise, cloudDensity, cloudDist);
		shadowFactor += cloudDensity * (pow(LdotV01, 6.5) * 0.9 + 0.0);
		shadowFactor -= perturbNoise * 0.0021;
		shadowFactor /= abs(worldLightVector.y) * 2.0 + 1.0;
		shadowFactor = max(0.0, shadowFactor);

		sunFlux = exp2(-shadowFactor * 8.5) * 3.0;
		sunFlux += exp2(-shadowFactor * 0.7) * 0.035;
	}


	// For skylight
	float skyFlux = 0.0;
	float skyFactor = 0.0;
	{
		vec3 lightDir = worldDir;
		lightDir *= pow(-LdotV * 0.5 + 0.5, 0.25);

		skyFactor = TraceCloudDensity(pos, lightDir, perturbNoise, cloudDensity, cloudDist);

		skyFlux = exp2(-skyFactor * 2.5) * 2.0;
		skyFlux += exp2(-skyFactor * 0.7) * 0.075;
	}


	// cloudDensity = max(0.0, cloudDensity - perturbNoise * 0.0025);



	vec3 color = vec3(0.0);

	const float sunMult = 0.3;

	color += sunFlux * PhaseMie(exp(-cloudDensity * 4.0), LdotV, LdotV2) * 9.0 * sunMult;
	color += vec3(1.0) * exp2(-shadowFactor * 2.0) * 24.15 * sunMult * cloudDensity; 			// multiple scattering fill
	// color += exp2(-shadowFactor * 15.7) * 10.035 * sunMult * PhaseMie(0.7, LdotV, LdotV2); 								// single scattering boost
	// color = vec3(1.0) * sunFlux * 10.5;
	// color *= CurveChroma(colorSunlight, 0.5);
	// color *= pow(colorSunlight, vec3(0.5));
	color *= colorSunlight;

	color += mix(colorSkyUp * skyFlux, atmosphere, vec3(exp2(-cloudDensity * 12.0))) * mix(1.0, 0.7, rainStrength);
	vec3 cirrusLightColor = CirrusSunlightColor(worldLightVector, colorSunlight);
	color += mix(cirrusLightColor * skyFlux * 2.0, cirrusLightColor * 7.0, vec3(exp2(-cloudDensity * 12.0))) * 0.5 * sunMult;


	cloudDensity = max(0.0, cloudDensity - perturbNoise * 0.005);
	// color *= exp2(-cloudDist * 0.0003 * Rayleigh);
	// color += cloudDist * 0.0004 * Rayleigh * exp2(-cloudDist * 0.0001 * Rayleigh);

	// cloudDensity = max(0.0, cloudDensity - perturbNoise * 0.013);

	cloudDensity /= cloudDist * 0.0001 + 0.25;
	// cloudDensity = smoothstep(0.0, 1.0, saturate(cloudDensity * 4.5));
	cloudDensity = 1.0 - exp(-cloudDensity * cloudDensity * 45.0);
	vec4 result = vec4(color, cloudDensity);


	return result;

}

float CloudDensityCirrus(vec3 pos, const int level, const float lunacrity)
{
	float cloudDist =length(pos.xz - cameraPosition.xz);
	pos /= cloudDist * 0.00005 + 1.0;
	pos *= 0.001;

	float n = 0.0;
	float g = 1.0;
	float w = 0.0;
	for (int i = 0; i < level; i++)
	{
		n += Get2DNoise(pos.xz + vec2(FRAME_TIME * 0.04 * CLOUD_SPEED, 0.0)) * g;
		w += g;

		// pos.x += FRAME_TIME * 0.02 * CLOUD_SPEED;
		pos.z += pos.x * 0.15 * n;
		pos.x += pos.z * 0.15 * n;
		pos *= 2.52;
		g *= lunacrity;
	}

	n /= w;

	// n += 0.0 + cloudDist * 0.00001;

	float takeAway = 0.3;
	#ifndef CUMULUS_CLOUDS
	takeAway = 0.1;
	#endif

	n = saturate(n * 1.0 - takeAway);
	// n = smoothstep(0.0, 1.0, n);
	// n = saturate(n * 1.0 - (sin(FRAME_TIME) * 0.5 + 0.5) * 0.5);

	n *= 1.0;

	return n;
}

vec4 CloudColorCirrus(vec3 pos, vec3 worldDir, vec3 worldLightVector, vec3 colorSunlight, 
	vec3 colorSkyUp, vec3 atmosphere, const bool detail)
{
	#ifndef CIRRUS_CLOUDS
	return vec4(0.0);
	#endif

	float LdotV = dot(worldDir, -worldLightVector);
	float LdotV01 = LdotV * 0.5 + 0.5;
	float LdotV2 = LdotV * LdotV;
	float cloudDist = length(pos.xz - cameraPosition.xz);

	float cloudDensity = CloudDensityCirrus(pos * vec3(1.0, 1.0, 0.5), 3, 0.43);

	float perturbNoise = 2.0;

	{
		if (detail)
		{
			vec3 motion = vec3(FRAME_TIME * 0.002 * CLOUD_SPEED, 0, 0);
			vec3 ppos = worldDir;
			ppos += vec3(0.0, -1.0, 0.0) * Get3DNoise((worldDir) * 50.0) * 0.02;
			ppos -= cloudDensity * 0.027;
			ppos *= 0.2 / (pow(abs(worldDir.y), 0.5) + 0.01);
			ppos.x *= 0.25;
			ppos.z += ppos.x * 0.4;
			perturbNoise =  (1.0 - Get3DNoise((ppos - motion) * 50.0)) * 2.0; ppos += worldDir * perturbNoise * 0.015;
			perturbNoise += (1.0 - Get3DNoise((ppos - motion) * 100.0)) * 2.0;
			perturbNoise += (1.0 - Get3DNoise((ppos - motion) * 200.0)) * 1.0;
			perturbNoise += (1.0 - Get3DNoise((ppos - motion) * 400.0)) * 0.5;
		}
		perturbNoise *= pow(cloudDist, 0.5) * 0.08 + 1.0;
		// perturbNoise -= cloudDist * 0.0029 + 0.0;
	}


	cloudDensity = max(0.0, cloudDensity - perturbNoise * 0.02);




	// float cloudDensity = 0.0;
	// {
	// 	const int level = 4;
	// 	const float lunacrity = 0.22;

	// 	pos /= cloudDist * 0.00005 + 1.0;
	// 	pos *= 0.001;

	// 	float n = 0.0;
	// 	float g = 1.0;
	// 	float w = 0.0;
	// 	for (int i = 0; i < level; i++)
	// 	{
	// 		n += Get2DNoise(pos.xz) * g;
	// 		w += g;

	// 		pos += worldDir * n;

	// 		pos.x += FRAME_TIME * 0.02 * CLOUD_SPEED;
	// 		pos *= 3.0;
	// 		g *= lunacrity;
	// 	}

	// 	n /= w;

	// 	// n += 0.0 + cloudDist * 0.00001;


	// 	n = saturate(n * 1.0 - 0.3);
	// 	// n = smoothstep(0.0, 1.0, n);
	// 	// n = saturate(n * 1.0 - (sin(FRAME_TIME) * 0.5 + 0.5) * 0.5);

	// 	n *= 1.5;

	// 	cloudDensity = n;
	// }



	const float sunMult = 0.4;

	vec3 color = CirrusSunlightColor(worldLightVector, colorSunlight) * 25.0 * sunMult * (PhaseMie(exp(-cloudDensity * 1.0), LdotV, LdotV2) + 0.5);
	color += mix(colorSkyUp, atmosphere, vec3(exp2(-cloudDensity * 12.0))) * 1.0;

	vec4 result = vec4(0.0);
	result.rgb = color;

	result.a = 1.0 - exp(-cloudDensity * cloudDensity * 4.0);


	return result;

}

void CloudPlane(inout vec3 color, vec3 worldVector, vec3 worldLightVector, vec3 colorSunlight, 
	vec3 colorSkyUp, vec3 atmosphere, float timeMidnight, const bool detail)
{
	// return;

	Ray viewRay = MakeRay
	(
		vec3(0.0),
		normalize(worldVector.xyz)
	);


	float planeHeight = 940;


	Plane pl;
	pl.origin = vec3(0.0f, cameraPosition.y - planeHeight, 0.0f);
	pl.normal = vec3(0.0f, 1.0f, 0.0f);

	Intersection i = RayPlaneIntersectionWorld(viewRay, pl);

	vec3 original = color.rgb;

	if (i.angle < 0.0f)
	{
		// if (i.distance < linearDepth || masks.sky > 0.5 || linearDepth >= far - 0.1)
		{
			vec4 cirrusCloud = CloudColorCirrus(i.pos.xyz, worldVector, worldLightVector, colorSunlight, colorSkyUp, atmosphere, detail);

			vec4 cloudSample = CloudColor(i.pos.xyz, worldVector, worldLightVector, colorSunlight, colorSkyUp, atmosphere, detail);

			// float atmosphereDist = 0.15 / (abs(worldVector.y) + 0.01);
			float atmosphereDist = i.distance * 0.0002;
			vec3 absorb = exp(-(mix(Rayleigh, vec3(0.4), vec3(rainStrength)) + 0.4) * atmosphereDist * 1.7);

			cloudSample.rgb *= absorb;
			cirrusCloud.rgb *= absorb;
			// cloudSample.rgb += min(1.0, atmosphereDist * 1.1) * atmosphere;
			// cirrusCloud.rgb += min(1.0, atmosphereDist * 1.1) * atmosphere;

			// vec3 atmos = Atmosphere(normalize(-worldVector), worldLightVector, 1.0, atmosphereDist * 8.0);
			vec3 atmos = min(1.0, atmosphereDist * 0.7) * atmosphere;
			cirrusCloud.rgb += atmos;
			cloudSample.rgb += atmos;

			cloudSample.rgb = DoNightEyeAtNight(cloudSample.rgb, timeMidnight);
			cirrusCloud.rgb = DoNightEyeAtNight(cirrusCloud.rgb, timeMidnight);

			color.rgb = mix(color.rgb, cirrusCloud.rgb * 1.0f, cirrusCloud.a);
			color.rgb = mix(color.rgb, cloudSample.rgb * 1.0f, cloudSample.a);

		}
	}
}








vec3 KelvinToRGB(float k)
{
	// float t = k / 100.0;

	// vec3 color = vec3(0.0);

	// if (t <= 66.0)
	// {
	// 	color.r = 1.0;
	// }
	// else
	// {
	// 	color.r = saturate((pow(t - 60.0, -0.133204) * 329.698) / 255.0);
	// }

	// if (t <= 66.0)
	// {
	// 	color.g = saturate((log(t) * 99.4708 - 161.119) / 255.0);
	// }
	// else
	// {
	// 	color.g = saturate((pow(t - 60.0, -0.075514) * 288.1221) / 255.0);
	// }

	// if (t >= 66.0)
	// {
	// 	color.b = 1.0;
	// }
	// else
	// {
	// 	color.b = saturate((log(t - 10.0) * 138.51773 - 305.044) / 255.0);
	// }

	// return color;

	const vec3 c = pow(vec3(0.1, 0.4, 0.98), vec3(0.9));

	float x = k - 6500.0;
	float xc = pow(abs(x), 1.1) * sign(x);

	return (exp(xc * 0.00045 * c) / exp(xc * 0.00017)) * 0.5;
}







// Lighting colors and data
vec3 GetColorSunlight(vec3 worldSunVector, float rainStrength)
{
	vec3 color = SunlightColorFromSunVector(worldSunVector);


	color += SunlightColorFromSunVector(-worldSunVector) * NIGHT_BRIGHTNESS;

	color *= 1.0 - rainStrength;
	// return vec3(1.0);
	return color;
}


vec3 GetColorTorchlight()
{
	// vec3 colorTorchlight = vec3(0.0);

	// if (TORCHLIGHT_COLOR_TEMPERATURE == 2000)
	// 	//2000k
	// 	colorTorchlight = pow(vec3(255, 141, 11) / 255.0, vec3(2.2));
	// else if (TORCHLIGHT_COLOR_TEMPERATURE == 2300)
	// 	//2300k
	// 	colorTorchlight = pow(vec3(255, 152, 54) / 255.0, vec3(2.2));
	// else if (TORCHLIGHT_COLOR_TEMPERATURE == 2500)
	// 	//2500k
	// 	colorTorchlight = pow(vec3(255, 166, 69) / 255.0, vec3(2.2));
	// else
	// 	//3000k
	// 	colorTorchlight = pow(vec3(255, 180, 107) / 255.0, vec3(2.2));

	// return colorTorchlight;

	return KelvinToRGB(float(TORCHLIGHT_COLOR_TEMPERATURE));
	// return KelvinToRGB((sin(FRAME_TIME) * 0.5 + 0.5) * 10000 + 1000);
}

void GetSkylightData(vec3 worldSunVector, float rainStrength,
	out vec4 skySHR, out vec4 skySHG, out vec4 skySHB, 
	out vec3 colorSkylight, out vec3 colorSkyUp)
{
	colorSkylight = vec3(0.0);

	const int latSamples = 5;
	const int lonSamples = 5;

	skySHR = vec4(0.0);
	skySHG = vec4(0.0);
	skySHB = vec4(0.0);

	for (int i = 0; i < latSamples; i++)
	{
		float latitude = (float(i) / float(latSamples)) * 3.14159265;
			  latitude = latitude;
		for (int j = 0; j < lonSamples; j++)
		{
			float longitude = (float(j) / float(lonSamples)) * 3.14159265 * 2.0;

			vec3 rayDir;
			rayDir.x = cos(latitude) * cos(longitude);
			rayDir.z = cos(latitude) * sin(longitude);
			rayDir.y = sin(latitude);

			vec3 skyCol = SkyShading(rayDir, worldSunVector, rainStrength);
			colorSkylight += skyCol;

			skySHR += ToSH(skyCol.r, rayDir);
			skySHG += ToSH(skyCol.g, rayDir);
			skySHB += ToSH(skyCol.b, rayDir);
		}
	}

	skySHR /= latSamples * lonSamples;
	skySHG /= latSamples * lonSamples;
	skySHB /= latSamples * lonSamples;

	colorSkylight /= latSamples * lonSamples;
	colorSkyUp = SkyShading(vec3(0.0, 1.0, 0.0), worldSunVector, rainStrength);
}





// Time data
float GetTimeMidnight(vec3 worldSunVector)
{
	return saturate(-worldSunVector.y * 10.0);
}
