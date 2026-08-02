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

in vec4 texcoord;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"
#include "lib/GBufferData.inc"
#include "lib/VoxelTracing.inc"
#include "lib/GIHelpers.inc"

/* RENDERTARGETS: 5,6 */

layout(location = 0) out vec4 outGI;
layout(location = 1) out vec4 outColorVariance;

void main() {
	GIBufferData gi = SampleGIBuffer(texcoord.xy);
	float age = gi.auxG;

	vec4 colorSample = textureLod(colortex6, texcoord.xy, 0);
	vec3 centerColor = colorSample.xyz;

	vec3 normal = GetNormals(texcoord.xy);
	float linearDepth = GetDepthLinear(texcoord.xy);

	// 3x3 color mean / second-moment for a cheap variance estimate
	const float radius = 4.0;
	vec4 colorSum = vec4(0.0);
	vec4 colorSqSum = vec4(0.0);
	int sampleCount = 0;

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			vec2 offset = vec2(x, y) / vec2(viewWidth, viewHeight) * radius;
			vec2 sampleUV = clamp(
				texcoord.xy + offset,
				4.0 / vec2(viewWidth, viewHeight),
				1.0 - 4.0 / vec2(viewWidth, viewHeight)
			);
			vec4 sampleColor = textureLod(colortex6, sampleUV, 0);
			colorSum += sampleColor;
			colorSqSum += sampleColor * sampleColor;
			sampleCount++;
		}
	}

	colorSum /= float(sampleCount) + 1e-06;
	colorSqSum /= float(sampleCount) + 1e-06;

	vec3 filteredColor = colorSum.xyz;
	vec4 colorStd = sqrt(max(vec4(0.0), colorSqSum - colorSum * colorSum));
	float varianceMetric = dot(colorStd.xyz, vec3(6.0));

	// Keep center color if the unused blend weight stays zero (original SEUS path).
	filteredColor = centerColor;

	// Temporal-stability: keep the minimum auxG (age) in a 3x3 neighborhood.
	float minAge = age;
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			vec2 sampleUV = texcoord.xy + vec2(x, y) / vec2(viewWidth, viewHeight);
			minAge = min(minAge, SampleGIBuffer(sampleUV).auxG);
		}
	}
	gi.auxG = minAge;

	outGI = PackGIBufferData(gi);
	outColorVariance = vec4(colorSample.xyz, varianceMetric);
}
