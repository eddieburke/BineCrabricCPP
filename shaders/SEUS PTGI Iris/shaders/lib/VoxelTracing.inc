// Voxel volume packing, DDA traversal, AABB hits, and Minecraft block-shape ray tests.
// Deobfuscated from SEUS PTGI E11 shared helpers (Iris port).

#ifndef SEUS_VOXEL_TRACING_INC
#define SEUS_VOXEL_TRACING_INC

int FloorToInt(float v) {
	return int(floor(v));
}

int ToEven(int v) {
	return v - FloorToInt(mod(float(v), 2.0)) - 0;
}

int ToOdd(int v) {
	return v - FloorToInt(mod(float(v), 2.0)) - 1;
}

// Side length of the screen-mapped voxel volume (even).
int GetScreenVolumeSide() {
	ivec2 res = ivec2(viewWidth, viewHeight);
	int pixels = res.x * res.y;
	return ToEven(FloorToInt(floor(pow(float(pixels), 0.333333))));
}

// Side length of the shadow-map-mapped voxel volume (odd), fixed 2048^2 atlas.
int GetShadowVolumeSide() {
	ivec2 res = ivec2(2048, 2048);
	int pixels = res.x * res.y;
	return ToOdd(FloorToInt(floor(pow(float(pixels), 0.333333))));
}

// Unpack a screen UV into a normalized volume coordinate (xzy layout).
vec3 ScreenUVToVolumeCoord(vec2 uv) {
	ivec2 res = ivec2(viewWidth, viewHeight);
	int side = GetScreenVolumeSide();
	ivec2 pixel = ivec2(uv.x * res.x, uv.y * res.y);
	float rowOfVolumes = float(pixel.y / side);
	float col = float(int(pixel.x + mod(res.x * rowOfVolumes, side)) / side);
	col += floor(res.x * rowOfVolumes / side);
	vec3 coord = vec3(0.0, 0.0, col);
	coord.x = mod(pixel.x + mod(res.x * rowOfVolumes, side), side);
	coord.y = mod(pixel.y, side);
	coord.xyz = floor(coord.xyz);
	coord /= side;
	coord.xyz = coord.xzy;
	return coord;
}

// Pack a normalized volume coordinate into a screen UV.
vec2 VolumeCoordToScreenUV(vec3 coord) {
	ivec2 res = ivec2(viewWidth, viewHeight);
	int side = GetScreenVolumeSide();
	vec3 cell = coord.xzy * side;
	cell = floor(cell + 1e-05);
	float z = cell.z;
	vec2 uv;
	uv.x = mod(cell.x + z * side, res.x);
	float linear = cell.x + z * side;
	uv.y = cell.y + floor(linear / res.x) * side;
	uv += 0.5;
	uv /= res;
	return uv;
}

// Unpack shadow-map UV (half-res quadrant) into a normalized volume coordinate.
vec3 ShadowUVToVolumeCoord(vec2 uv) {
	vec2 scaled = uv;
	scaled.xy /= 0.5;
	ivec2 res = ivec2(2048, 2048);
	int side = GetShadowVolumeSide();
	ivec2 pixel = ivec2(scaled.x * res.x, scaled.y * res.y);
	float rowOfVolumes = float(pixel.y / side);
	float col = float(int(pixel.x + mod(res.x * rowOfVolumes, side)) / side);
	col += floor(res.x * rowOfVolumes / side);
	vec3 coord = vec3(0.0, 0.0, col);
	coord.x = mod(pixel.x + mod(res.x * rowOfVolumes, side), side);
	coord.y = mod(pixel.y, side);
	coord.xyz = floor(coord.xyz);
	coord /= side;
	coord.xyz = coord.xzy;
	return coord;
}

// Pack a normalized volume coordinate into shadow-map UV (half-res quadrant).
vec2 VolumeCoordToShadowUV(vec3 coord, int side) {
	coord = clamp(coord, vec3(0.0), vec3(1.0));
	ivec2 res = ivec2(2048, 2048);
	vec3 cell = coord.xzy * side;
	cell = floor(cell + 1e-05);
	float z = cell.z;
	vec2 uv;
	uv.x = mod(cell.x + z * side, res.x);
	float linear = cell.x + z * side;
	uv.y = cell.y + floor(linear / res.x) * side;
	uv += 0.5;
	uv /= res;
	uv.xy *= 0.5;
	return uv;
}

vec3 WorldOffsetToVolumeCoordClamped(vec3 offset, int side) {
	offset *= 1.0 / float(side);
	offset = offset + vec3(0.5);
	offset = clamp(offset, vec3(0.0), vec3(1.0));
	return offset;
}

vec3 WorldOffsetToVolumeCoord(vec3 offset, int side) {
	offset *= 1.0 / float(side);
	offset = offset + vec3(0.5);
	return offset;
}

vec3 VolumeCoordToShadowWorldOffset(vec3 coord) {
	int side = GetShadowVolumeSide();
	coord = coord - vec3(0.5);
	coord *= side;
	return coord;
}

vec3 WorldOffsetToScreenVolumeCoord(vec3 offset) {
	int side = GetScreenVolumeSide();
	offset *= 1.0 / float(side);
	offset = offset + vec3(0.5);
	offset = clamp(offset, vec3(0.0), vec3(1.0));
	return offset;
}

vec3 ScreenVolumeCoordToWorldOffset(vec3 coord) {
	int side = GetScreenVolumeSide();
	coord = coord - vec3(0.5);
	coord *= side;
	return coord;
}

// Integer camera movement between frames (in blocks).
vec3 GetCameraVoxelDelta() {
	vec3 cur = cameraPosition.xyz + 0.5;
	vec3 prev = previousCameraPosition.xyz + 0.5;
	vec3 curFloor = floor(cur - 0.0001);
	vec3 prevFloor = floor(prev - 0.0001);
	return curFloor - prevFloor;
}

vec3 WorldPosToShadowMapCoord(vec3 worldPos) {
	vec4 sp = vec4(worldPos, 1.0);
	sp = shadowModelView * sp;
	sp = shadowProjection * sp;
	sp /= sp.w;
	float dist = sqrt(sp.x * sp.x + sp.y * sp.y);
	float distort = 1.0 - SHADOW_MAP_BIAS + dist * SHADOW_MAP_BIAS;
	sp.xy *= 0.95 / distort;
	sp.z = mix(sp.z, 0.5, 0.8);
	sp = sp * 0.5 + 0.5;
	sp.xy *= 0.5;
	sp.xy += 0.5;
	return sp.xyz;
}

// Adjust hit position for thin/non-full blocks using mid-texcoord / tangent data.
vec3 AdjustVoxelHitForBlockShape(
	vec3 voxelPos,
	vec3 faceNormal,
	vec2 texcoord,
	vec2 midTexcoord,
	vec4 tangent,
	vec4 entityData,
	inout float discardHit,
	out vec2 faceUV
) {
	// E11: m = (any axis off half-grid); m = !m  → true means full-cube aligned.
	bool isFullCube = !(
		(fract(voxelPos.x * 2.0) > 0.01 && fract(voxelPos.x * 2.0) < 0.99)
		|| (fract(voxelPos.y * 2.0) > 0.01 && fract(voxelPos.y * 2.0) < 0.99)
		|| (fract(voxelPos.z * 2.0) > 0.01 && fract(voxelPos.z * 2.0) < 0.99)
	);

	if (entityData.x == 8.0 || entityData.x == 9.0 || entityData.x == 79.0
		|| entityData.x < 1.0 || !isFullCube || entityData.x == 20.0 || entityData.x == 171.0
		|| min(abs(faceNormal.x), abs(faceNormal.z)) > 0.2) {
		discardHit = 1.0;
	}
	if (entityData.x == 50.0 || entityData.x == 76.0) {
		discardHit = 0.0;
		if (faceNormal.y < 0.5) {
			discardHit = 1.0;
		}
	}
	if (entityData.x == 51.0) {
		discardHit = 0.0;
	}

	vec3 tangentDir;
	vec3 bitangentDir;
	if (faceNormal.x > 0.5) {
		tangentDir = vec3(0.0, 0.0, -1.0);
		bitangentDir = vec3(0.0, -1.0, 0.0);
	} else if (faceNormal.x < -0.5) {
		tangentDir = vec3(0.0, 0.0, 1.0);
		bitangentDir = vec3(0.0, -1.0, 0.0);
	} else if (faceNormal.y > 0.5) {
		tangentDir = vec3(1.0, 0.0, 0.0);
		bitangentDir = vec3(0.0, 0.0, 1.0);
	} else if (faceNormal.y < -0.5) {
		tangentDir = vec3(1.0, 0.0, 0.0);
		bitangentDir = vec3(0.0, 0.0, -1.0);
	} else if (faceNormal.z > 0.5) {
		tangentDir = vec3(1.0, 0.0, 0.0);
		bitangentDir = vec3(0.0, -1.0, 0.0);
	} else if (faceNormal.z < -0.5) {
		tangentDir = vec3(-1.0, 0.0, 0.0);
		bitangentDir = vec3(0.0, -1.0, 0.0);
	}

	faceUV = clamp((texcoord.xy - midTexcoord.xy) * 100000.0, vec2(0.0), vec2(1.0));
	float tangentExtent = 0.15;
	float normalInset = 0.15;
	if (entityData.x == 10.0 || entityData.x == 11.0) {
		if (abs(faceNormal.y) < 0.01 && isFullCube || faceNormal.y > 0.99) {
			tangentExtent = 0.1;
			normalInset = 0.1;
			discardHit = 0.0;
		} else {
			discardHit = 1.0;
		}
	}
	if (entityData.x == 51.0) {
		tangentExtent = 0.5;
		normalInset = 0.1;
	}
	if (entityData.x == 76.0) {
		tangentExtent = 0.2;
		normalInset = 0.2;
	}
	if (entityData.x - 255.0 + 39.0 >= 103.0 && entityData.x - 255.0 + 39.0 <= 113.0) {
		normalInset = 0.025;
		tangentExtent = 0.025;
	}

	tangentDir = normalize(tangent.xyz);
	bitangentDir = normalize(cross(tangentDir, faceNormal.xyz) * sign(tangent.w));
	vec3 hit = voxelPos.xyz + mix(tangentDir * tangentExtent, -tangentDir * tangentExtent, vec3(faceUV.x));
	hit.xyz += mix(bitangentDir * tangentExtent, -bitangentDir * tangentExtent, vec3(faceUV.y));
	hit.xyz -= faceNormal.xyz * normalInset;
	return hit;
}

// Amanatides & Woo style voxel DDA state.
struct VoxelDDA {
	vec3 voxelPos;
	vec3 voxelOrigin;
	vec3 deltaDist;
	vec3 stepDir;
	vec3 sideDist;
	vec3 stepMask;
};

VoxelDDA InitVoxelDDA(Ray ray) {
	VoxelDDA dda;
	dda.voxelPos = floor(ray.origin);
	dda.voxelOrigin = dda.voxelPos;
	dda.deltaDist = abs(vec3(length(ray.direction)) / (ray.direction + 1e-07));
	dda.stepDir = sign(ray.direction);
	dda.sideDist = (sign(ray.direction) * (dda.voxelPos - ray.origin) + sign(ray.direction) * 0.5 + 0.5) * dda.deltaDist;
	dda.stepMask = vec3(0.0);
	return dda;
}

void StepVoxelDDA(inout VoxelDDA dda) {
	dda.stepMask = step(dda.sideDist.xyz, dda.sideDist.yzx) * step(dda.sideDist.xyz, dda.sideDist.zxy);
	dda.sideDist += dda.stepMask * dda.deltaDist;
	dda.voxelPos += dda.stepMask * dda.stepDir;
}

void RayAABBIntersectDistances(in Ray ray, in vec3 bounds[2], out float tNear, out float tFar) {
	float tyNear, tyFar, tzNear, tzFar;
	tNear = (bounds[ray.sign[0]].x - ray.origin.x) * ray.inv_direction.x;
	tFar = (bounds[1 - ray.sign[0]].x - ray.origin.x) * ray.inv_direction.x;
	tyNear = (bounds[ray.sign[1]].y - ray.origin.y) * ray.inv_direction.y;
	tyFar = (bounds[1 - ray.sign[1]].y - ray.origin.y) * ray.inv_direction.y;
	tzNear = (bounds[ray.sign[2]].z - ray.origin.z) * ray.inv_direction.z;
	tzFar = (bounds[1 - ray.sign[2]].z - ray.origin.z) * ray.inv_direction.z;
	tNear = max(max(tNear, tyNear), tzNear);
	tFar = min(min(tFar, tyFar), tzFar);
}

vec3 AABBSurfaceNormal(const vec3 boxMin, const vec3 boxMax, vec3 hitPos) {
	const float eps = 1e-05;
	vec3 center = (boxMax + boxMin) * 0.5;
	vec3 halfSize = (boxMax - boxMin) * 0.5;
	vec3 local = hitPos - center;
	vec3 normal = vec3(0.0);
	normal += vec3(sign(local.x), 0.0, 0.0) * step(abs(abs(local.x) - halfSize.x), eps);
	normal += vec3(0.0, sign(local.y), 0.0) * step(abs(abs(local.y) - halfSize.y), eps);
	normal += vec3(0.0, 0.0, sign(local.z)) * step(abs(abs(local.z) - halfSize.z), eps);
	return normalize(normal);
}

bool RayAABBIntersect(const vec3 boxMin, const vec3 boxMax, Ray ray, out vec2 tNearFar) {
	vec3 t0 = ray.inv_direction * (boxMin - ray.origin);
	vec3 t1 = ray.inv_direction * (boxMax - ray.origin);
	vec3 tMin = min(t1, t0);
	vec3 tMax = max(t1, t0);
	vec2 t = max(tMin.xx, tMin.yz);
	float tNear = max(t.x, t.y);
	t = min(tMax.xx, tMax.yz);
	float tFar = min(t.x, t.y);
	tNearFar.x = tNear;
	tNearFar.y = tFar;
	return tFar > max(tNear, 0.0);
}

bool RayAABBHitCloser(const vec3 boxMin, const vec3 boxMax, Ray ray, inout float closestT, inout vec3 hitNormal) {
	vec3 t0 = ray.inv_direction * (boxMin - 1e-05 - ray.origin);
	vec3 t1 = ray.inv_direction * (boxMax + 1e-05 - ray.origin);
	vec3 tMin = min(t1, t0);
	vec3 tMax = max(t1, t0);
	vec2 t = max(tMin.xx, tMin.yz);
	float tNear = max(t.x, t.y);
	t = min(tMax.xx, tMax.yz);
	float tFar = min(t.x, t.y);
	bool hit = tFar > max(tNear, 0.0) && max(tNear, 0.0) < closestT;
	if (hit) {
		hitNormal = AABBSurfaceNormal(boxMin - 1e-05, boxMax + 1e-05, ray.origin + ray.direction * tNear);
		closestT = tNear;
	}
	return hit;
}

vec3 SampleShadowedSunlightWithCaustics(vec3 worldPos, vec3 normal, vec3 lightDir, vec3 unused, int unusedLod) {
	if (rainStrength > 0.99) {
		return vec3(0.0);
	}
	worldPos += 1.0;
	worldPos -= Fract01(cameraPosition + 0.5);
	vec3 shadowCoord = WorldPosToShadowMapCoord(worldPos);
	float biasScale = 0.5;
	vec3 light = vec3(1.0) * shadow2DLod(shadowtex0, vec3(shadowCoord.xy, shadowCoord.z - 0.0006 * biasScale), 2).x;
	light *= saturate(dot(normal, lightDir));
	{
		vec4 underwaterData = textureLod(shadowcolor1, shadowCoord.xy - vec2(0.0, 0.5), 4);
		float waterDepth = abs(underwaterData.x * 256.0 - (worldPos.y + cameraPosition.y));
		float caustics = GetCausticsComposite(worldPos, normal, waterDepth);
		float dryShadow = shadow2DLod(shadowtex0, vec3(shadowCoord.xy - vec2(0.0, 0.5), shadowCoord.z + 1e-06), 4).x;
		light = mix(light, light * caustics, 1.0 - dryShadow);
	}
	light = TintUnderwaterDepth(light);
	return light * (1.0 - rainStrength);
}

vec3 SampleShadowedSunlightVolume(vec3 volumeCoord, vec3 normal, vec3 lightDir, vec3 unused, int unusedLod) {
	if (rainStrength > 0.99) {
		return vec3(0.0);
	}
	vec3 worldOffset = VolumeCoordToShadowWorldOffset(volumeCoord);
	vec3 shadowCoord = WorldPosToShadowMapCoord(worldOffset + lightDir * 0.99);
	float biasScale = 0.5;
	vec3 light = vec3(1.0) * shadow2DLod(shadowtex0, vec3(shadowCoord.xy, shadowCoord.z - 0.0006 * biasScale), 3).x;
	light *= saturate(dot(normal, lightDir));
	light = TintUnderwaterDepth(light);
	#ifdef GI_SUNLIGHT_STAINED_GLASS_TINT
	float glassShadow = shadow2DLod(shadowtex0, vec3(shadowCoord.xy - vec2(0.5, 0.0), shadowCoord.z - 0.0006 * biasScale), 3).x;
	vec3 glassColor = textureLod(shadowcolor, vec2(shadowCoord.xy - vec2(0.5, 0.0)), 3).xyz;
	glassColor *= glassColor;
	light = mix(light, light * glassColor, vec3(1.0 - glassShadow));
	#endif
	return light * (1.0 - rainStrength);
}

vec3 SampleShadowedSunlightStained(vec3 worldPos, vec3 normal, vec3 lightDir, vec3 unused, int unusedLod) {
	if (rainStrength > 0.99) {
		return vec3(0.0);
	}
	worldPos += 1.0;
	worldPos -= Fract01(cameraPosition + 0.5);
	vec3 shadowCoord = WorldPosToShadowMapCoord(worldPos);
	float biasScale = 0.5;
	vec3 light = vec3(1.0) * shadow2DLod(shadowtex0, vec3(shadowCoord.xy, shadowCoord.z - 0.0006 * biasScale), 2).x;
	light *= saturate(dot(normal, lightDir));
	light = TintUnderwaterDepth(light);
	#ifdef GI_SUNLIGHT_STAINED_GLASS_TINT
	float glassShadow = shadow2DLod(shadowtex0, vec3(shadowCoord.xy - vec2(0.5, 0.0), shadowCoord.z - 0.0006 * biasScale), 3).x;
	vec3 glassColor = textureLod(shadowcolor, vec2(shadowCoord.xy - vec2(0.5, 0.0)), 3).xyz;
	glassColor *= glassColor;
	light = mix(light, light * glassColor, vec3(1.0 - glassShadow));
	#endif
	return light * (1.0 - rainStrength);
}

// Temporally accumulated GI sample stored in colortex5.
struct GIBufferData {
	float depth;
	float auxB;
	float auxR;
	float auxG;
	vec3 irradiance;
};

vec4 PackGIBufferData(GIBufferData data) {
	vec4 packedData;
	data.irradiance = max(vec3(0.0), data.irradiance);
	packedData.x = data.depth;
	data.irradiance = pow(data.irradiance, vec3(0.125));
	packedData.y = PackTwo16BitTo32Bit(data.irradiance.x, data.auxR);
	packedData.z = PackTwo16BitTo32Bit(data.irradiance.y, data.auxG);
	packedData.w = PackTwo16BitTo32Bit(data.irradiance.z, data.auxB);
	return packedData;
}

GIBufferData UnpackGIBufferData(vec4 packedData) {
	GIBufferData data;
	vec2 y = UnpackTwo16BitFrom32Bit(packedData.y);
	vec2 z = UnpackTwo16BitFrom32Bit(packedData.z);
	vec2 w = UnpackTwo16BitFrom32Bit(packedData.w);
	data.depth = packedData.x;
	data.auxR = y.y;
	data.auxG = z.y;
	data.auxB = w.y;
	data.irradiance = pow(vec3(y.x, z.x, w.x), vec3(8.0));
	return data;
}

GIBufferData SampleGIBuffer(vec2 uv) {
	vec2 texel = 1.0 / vec2(viewWidth, viewHeight);
	vec2 res = vec2(viewWidth, viewHeight);
	uv = (floor(uv * res) + 0.5) * texel;
	return UnpackGIBufferData(textureLod(colortex5, uv, 0));
}

float ReflectionStrengthFromSmoothness(float smoothness, float metalness) {
	float strength = 1.0;
	#ifdef FULL_RT_REFLECTIONS
	strength = clamp(pow(smoothness, 0.125) + metalness, 0.0, 1.0);
	#else
	strength = clamp(smoothness * 10.0 - 7.0, 0.0, 1.0);
	#endif
	return strength;
}

// Trace Minecraft block collision shapes (slabs, stairs, fences, walls, …).
bool TraceBlockShape(vec3 voxelMin, float blockId, Ray ray, bool isOpaqueFull, inout float closestT, inout vec3 hitNormal) {
	bool anyHit = false;
	bool hit = false;
	#if RAYTRACE_GEOMETRY_QUALITY == 0
	if (isOpaqueFull) {
		return false;
	}
	if (blockId >= 67.0) {
		return false;
	}
	hit = RayAABBHitCloser(voxelMin, voxelMin + vec3(1.0, 1.0, 1.0), ray, closestT, hitNormal);
	anyHit = hit;
	#else
	if (blockId < 40.0) {
		hit = RayAABBHitCloser(voxelMin, voxelMin + vec3(1.0, 1.0, 1.0), ray, closestT, hitNormal);
		return hit;
	}
	if (blockId == 40.0 || blockId == 41.0 || blockId >= 43.0 && blockId <= 54.0) {
		float height = 0.5;
		if (blockId == 41.0) {
			height = 0.9375;
		}
		hit = RayAABBHitCloser(voxelMin + vec3(0.0, 0.0, 0.0), voxelMin + vec3(1.0, height, 1.0), ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId == 42.0 || blockId >= 55.0 && blockId <= 66.0) {
		hit = RayAABBHitCloser(voxelMin + vec3(0.0, 0.5, 0.0), voxelMin + vec3(1.0, 1.0, 1.0), ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId == 43.0 || blockId == 46.0 || blockId == 47.0 || blockId == 52.0 || blockId == 53.0 || blockId == 54.0
		|| blockId == 55.0 || blockId == 58.0 || blockId == 59.0 || blockId == 64.0 || blockId == 65.0 || blockId == 66.0) {
		float y0 = 0.5;
		if (blockId == 55.0 || blockId == 58.0 || blockId == 59.0 || blockId == 64.0 || blockId == 65.0 || blockId == 66.0) {
			y0 = 0.0;
		}
		hit = RayAABBHitCloser(voxelMin + vec3(0.0, y0, 0.0), voxelMin + vec3(0.5, 0.5 + y0, 0.5), ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId == 43.0 || blockId == 45.0 || blockId == 48.0 || blockId == 51.0 || blockId == 53.0 || blockId == 54.0
		|| blockId == 55.0 || blockId == 57.0 || blockId == 60.0 || blockId == 63.0 || blockId == 65.0 || blockId == 66.0) {
		float y0 = 0.5;
		if (blockId == 55.0 || blockId == 57.0 || blockId == 60.0 || blockId == 63.0 || blockId == 65.0 || blockId == 66.0) {
			y0 = 0.0;
		}
		hit = RayAABBHitCloser(voxelMin + vec3(0.5, y0, 0.0), voxelMin + vec3(1.0, 0.5 + y0, 0.5), ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId == 44.0 || blockId == 45.0 || blockId == 49.0 || blockId == 51.0 || blockId == 52.0 || blockId == 54.0
		|| blockId == 56.0 || blockId == 57.0 || blockId == 61.0 || blockId == 63.0 || blockId == 64.0 || blockId == 66.0) {
		float y0 = 0.5;
		if (blockId == 56.0 || blockId == 57.0 || blockId == 61.0 || blockId == 63.0 || blockId == 64.0 || blockId == 66.0) {
			y0 = 0.0;
		}
		hit = RayAABBHitCloser(voxelMin + vec3(0.5, y0, 0.5), voxelMin + vec3(1.0, 0.5 + y0, 1.0), ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId == 44.0 || blockId == 46.0 || blockId == 50.0 || blockId == 51.0 || blockId == 52.0 || blockId == 53.0
		|| blockId == 56.0 || blockId == 58.0 || blockId == 62.0 || blockId == 63.0 || blockId == 64.0 || blockId == 65.0) {
		float y0 = 0.5;
		if (blockId == 56.0 || blockId == 58.0 || blockId == 62.0 || blockId == 63.0 || blockId == 64.0 || blockId == 65.0) {
			y0 = 0.0;
		}
		hit = RayAABBHitCloser(voxelMin + vec3(0.0, y0, 0.5), voxelMin + vec3(0.5, 0.5 + y0, 1.0), ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId >= 67.0 && blockId <= 82.0) {
		hit = RayAABBHitCloser(voxelMin + vec3(6.0, 0.0, 6.0) / 16.0, voxelMin + vec3(10.0, 16.0, 10.0) / 16.0, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId == 68.0 || blockId == 69.0 || blockId == 70.0 || blockId == 72.0 || blockId == 73.0 || blockId == 74.0
		|| blockId == 76.0 || blockId == 77.0 || blockId == 78.0 || blockId == 80.0 || blockId == 81.0 || blockId == 82.0) {
		float x0 = 8.0;
		float x1 = 8.0;
		if (blockId == 68.0 || blockId == 70.0 || blockId == 72.0 || blockId == 74.0 || blockId == 76.0 || blockId == 78.0 || blockId == 80.0 || blockId == 82.0) {
			x0 = 0.0;
		}
		if (blockId == 69.0 || blockId == 70.0 || blockId == 73.0 || blockId == 74.0 || blockId == 77.0 || blockId == 78.0 || blockId == 81.0 || blockId == 82.0) {
			x1 = 16.0;
		}
		hit = RayAABBHitCloser(voxelMin + vec3(x0, 6.0, 7.0) / 16.0, voxelMin + vec3(x1, 9.0, 9.0) / 16.0, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
		hit = RayAABBHitCloser(voxelMin + vec3(x0, 12.0, 7.0) / 16.0, voxelMin + vec3(x1, 15.0, 9.0) / 16.0, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId >= 71.0 && blockId <= 82.0) {
		float z0 = 8.0;
		float z1 = 8.0;
		if (blockId >= 71.0 && blockId <= 74.0 || blockId >= 79.0 && blockId <= 82.0) {
			z1 = 16.0;
		}
		if (blockId >= 75.0 && blockId <= 82.0) {
			z0 = 0.0;
		}
		hit = RayAABBHitCloser(voxelMin + vec3(7.0, 6.0, z0) / 16.0, voxelMin + vec3(9.0, 9.0, z1) / 16.0, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
		hit = RayAABBHitCloser(voxelMin + vec3(7.0, 12.0, z0) / 16.0, voxelMin + vec3(9.0, 15.0, z1) / 16.0, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	#if RAYTRACE_GEOMETRY_QUALITY == 2
	if (blockId >= 83.0 && blockId <= 86.0) {
		vec3 a = vec3(0.0);
		vec3 b = vec3(0.0);
		if (blockId == 83.0) { a = vec3(0.0, 0.0, 0.0); b = vec3(16.0, 16.0, 3.0); }
		if (blockId == 84.0) { a = vec3(0.0, 0.0, 13.0); b = vec3(16.0, 16.0, 16.0); }
		if (blockId == 86.0) { a = vec3(0.0, 0.0, 0.0); b = vec3(3.0, 16.0, 16.0); }
		if (blockId == 85.0) { a = vec3(13.0, 0.0, 0.0); b = vec3(16.0, 16.0, 16.0); }
		hit = RayAABBHitCloser(voxelMin + a / 16.0, voxelMin + b / 16.0, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId >= 87.0 && blockId <= 102.0) {
		vec3 a = vec3(0.0);
		vec3 b = vec3(1.0);
		if (blockId >= 87.0 && blockId <= 94.0) {
			float y0 = 0.0;
			if (blockId >= 91.0 && blockId <= 94.0) {
				y0 = 13.0;
			}
			a = vec3(0.0, y0, 0.0) / 16.0;
			b = vec3(16.0, y0 + 3.0, 16.0) / 16.0;
		}
		if (blockId >= 95.0 && blockId <= 98.0) {
			float z0 = 13.0;
			if (blockId == 97.0 || blockId == 98.0) {
				z0 = 0.0;
			}
			a = vec3(0.0, 0.0, z0) / 16.0;
			b = vec3(16.0, 16.0, z0 + 3.0) / 16.0;
		}
		if (blockId >= 99.0 && blockId <= 102.0) {
			float x0 = 13.0;
			if (blockId == 99.0 || blockId == 100.0) {
				x0 = 0.0;
			}
			a = vec3(x0, 0.0, 0.0) / 16.0;
			b = vec3(x0 + 3.0, 16.0, 16.0) / 16.0;
		}
		hit = RayAABBHitCloser(voxelMin + a, voxelMin + b, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	if (blockId >= 103.0 && blockId <= 113.0) {
		vec3 a = vec3(0.0);
		vec3 b = vec3(1.0);
		if (blockId >= 103.0 && blockId <= 110.0) {
			float level = float(blockId) - float(103.0) + 1.0;
			b.y = level * 2.0 / 16.0;
		}
		if (blockId == 111.0) {
			b.y = 0.0625;
		}
		if (blockId == 112.0) {
			a = vec3(1.0, 0.0, 1.0) / 16.0;
			b = vec3(15.0, 1.0, 15.0) / 16.0;
		}
		if (blockId == 113.0) {
			a = vec3(1.0, 0.0, 1.0) / 16.0;
			b = vec3(15.0, 0.5, 15.0) / 16.0;
		}
		hit = RayAABBHitCloser(voxelMin + a, voxelMin + b, ray, closestT, hitNormal);
		anyHit = anyHit || hit;
	}
	#endif
	#endif
	return anyHit;
}

#endif // SEUS_VOXEL_TRACING_INC
