// Shared GI sampling helpers (deobfuscated from SEUS PTGI E11).

#ifndef SEUS_GI_HELPERS_INC
#define SEUS_GI_HELPERS_INC

vec2 Hash2(inout float seed) {
	return fract(sin(vec2(seed += 0.1, seed += 0.1)) * vec2(43758.5, 22578.1));
}

float R2Noise(vec2 uv) {
	uv *= vec2(viewWidth, viewHeight);
	const float g = 1.61803;
	const float a1 = 1.32472;
	return fract(dot(uv, 1.0 / vec2(g, a1 * a1)));
}

vec3 SampleBlueNoise(vec2 uv) {
	vec2 noiseUV = vec2(uv.xy * vec2(viewWidth, viewHeight)) / 64.0;
	const vec2 ditherOffsets[16] = vec2[16](
		vec2(-1.0, -1.0), vec2(0.0, -0.333333), vec2(-0.5, 0.333333), vec2(0.5, -0.777778),
		vec2(-0.75, -0.111111), vec2(0.25, 0.555556), vec2(-0.25, -0.555556), vec2(0.75, 0.111111),
		vec2(-0.875, 0.777778), vec2(0.125, -0.925926), vec2(-0.375, -0.259259), vec2(0.625, 0.407407),
		vec2(-0.625, -0.703704), vec2(0.375, -0.037037), vec2(-0.125, 0.62963), vec2(0.875, -0.481482)
	);
	if (!(uv.x < 2.0 / viewWidth || uv.x > 1.0 - 2.0 / viewWidth
		|| uv.y < 2.0 / viewHeight || uv.y > 1.0 - 2.0 / viewHeight)) {
		noiseUV += ditherOffsets[int(mod(frameCounter, 8.0))] * 0.5;
	}
	noiseUV = (floor(noiseUV * 64.0) + 0.5) / 64.0;
	return texture2D(noisetex, noiseUV).xyz;
}

// Cosine-weighted hemisphere around `normal`, using 2D random `rand` in [0,1].
vec3 SampleHemisphere(vec3 normal, vec2 rand) {
	rand = rand * 0.99 + 0.005;
	#if GI_RESPONSIVENESS < 1
	#endif
	float phi = 6.28319 * rand.x;
	float cosTheta = sqrt(rand.y);
	vec3 tangent = normalize(cross(normal, vec3(0.0, 1.0, 1.0)));
	vec3 bitangent = cross(normal, tangent);
	return tangent * cos(phi) * cosTheta + bitangent * sin(phi) * cosTheta + normal.xyz * sqrt(1.0 - rand.y);
}

vec3 RandomUnitVector(inout float seed) {
	vec3 noise = SampleBlueNoise(texcoord.xy).xyz;
	noise = fract(noise + vec3(Hash2(seed), Hash2(seed).x) * 0.1);
	noise = noise * 2.0 - 1.0;
	return normalize(noise);
}

float GaussianWeight(float x, float sigma) {
	return exp(-pow(x / (0.9 * sigma), 2.0));
}

#endif
