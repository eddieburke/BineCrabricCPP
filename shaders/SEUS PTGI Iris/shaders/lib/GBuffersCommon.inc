vec3 GetRainAnimationTex(sampler2D tex, vec2 uv, float wet)
{
	//float frame = mod(floor(float(frameCounter) * 1.0), 60.0);
	// frame = 0.0;

	float frame = mod(floor(frameTimeCounter * 60.0), 60.0);
	vec2 coord = vec2(uv.x, mod(uv.y / 60.0, 1.0) - frame / 60.0);

	vec3 n = texture2D(tex, coord).rgb * 2.0 - 1.0;
	n.y *= -1.0;

	n.xy = pow(abs(n.xy) * 1.0, vec2(2.0 - wet * wet * wet * 1.2)) * sign(n.xy);
	// n.xy = pow(abs(n.xy) * 1.0, vec2(1.0)) * sign(n.xy);

	return n;
}

vec3 BilateralRainTex(sampler2D tex, vec2 uv, float wet)
{
	vec3 n = GetRainAnimationTex(tex, uv.xy, wet);
	vec3 nR = GetRainAnimationTex(tex, uv.xy + vec2(1.0, 0.0) / 128.0, wet);
	vec3 nU = GetRainAnimationTex(tex, uv.xy + vec2(0.0, 1.0) / 128.0, wet);
	vec3 nUR = GetRainAnimationTex(tex, uv.xy + vec2(1.0, 1.0) / 128.0, wet);

	vec2 fractCoord = fract(uv.xy * 128.0);

	vec3 lerpX = mix(n, nR, fractCoord.x);
	vec3 lerpX2 = mix(nU, nUR, fractCoord.x);
	vec3 lerpY = mix(lerpX, lerpX2, fractCoord.y);

	return lerpY;
}



vec3 GetRainSplashNormal(vec3 worldPosition, vec3 worldNormal, inout float wet)
{
	if (wetness < 0.01)
	{
		return vec3(0.0, 0.0, 1.0);
	}

	vec3 pos = worldPosition * 0.5;

	vec3 flowPos = pos;


	#ifdef RAIN_SPLASH_BILATERAL
	vec3 n1 = BilateralRainTex(gaux2, pos.xz, wet);
	// vec3 n2 = BilateralRainTex(gaux2, pos.xz, wet);
	// vec3 n3 = BilateralRainTex(colortex6, pos.xz, wet);
	#else
	vec3 n1 = GetRainAnimationTex(gaux2, pos.xz, wet);
	// vec3 n2 = GetRainAnimationTex(gaux2, pos.xz, wet);
	// vec3 n3 = GetRainAnimationTex(colortex6, pos.xz, wet);
	#endif

	pos.x -= frameTimeCounter * 1.5;
	float downfall = texture2D(noisetex, pos.xz * 0.0025).x;
	downfall = saturate(downfall * 1.5 - 0.25);


	vec3 n = n1 * 1.0;
	// n += n2 * saturate(downfall * 2.0) * 1.0;
	// n += n3 * saturate(downfall * 2.0 - 1.0) * 1.0;
	// n = n3 * 3.0;


	float lod = dot(abs(fwidth(pos.xyz)), vec3(1.0));

	n.xy *= 1.0 / (1.0 + lod * 5.0);

	// n.xy /= wet + 0.1;
	// n.x = downfall;

	wet = saturate(wet * 1.0 + downfall * (1.0 - wet) * 0.95);
	// wet = downfall * 0.2 + 0.8;

	n.xy *= rainStrength;



	vec3 rainSplashNormal = n;




	vec3 rainFlowNormal = vec3(0.0, 0.0, 1.0);
	// flowPos.xz *= 12.0;
	// flowPos.y += frameTimeCounter * 6.0;
	// rainFlowNormal.xy = vec2(Get3DNoise(flowPos.xyz) * 2.0 - 1.0, Get3DNoise(flowPos.xyz + 2.0) * 2.0 - 1.0) * 0.05;
	// flowPos.xz *= 4.0;
	// rainFlowNormal.xy += vec2(Get3DNoise(flowPos.xyz) * 2.0 - 1.0, Get3DNoise(flowPos.xyz + 2.0) * 2.0 - 1.0) * 0.035;
	// rainFlowNormal = normalize(rainFlowNormal);

	n = mix(rainFlowNormal, rainSplashNormal, saturate(worldNormal.y));

	return n;
}

void CalculateNormalAndTBN(vec3 p, vec2 uv, out vec3 N, out mat3 tbn)
{
	N = normalize(cross(dFdx(p), dFdy(p)));

	// uv.x *= atlasSize.x;
	// uv.y *= atlasSize.y;

	uv.xy *= textureSize(texture, 0);

	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );

	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	tbn = mat3( T * invmax, B * invmax, N );
}



vec4 GetSourceTexture(sampler2D tex, vec2 texcoord)
{

	return texture2D(tex, texcoord, 0.0);

















}
