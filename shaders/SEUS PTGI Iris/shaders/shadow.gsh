#version 330 compatibility

/*
 * Iris dual-draw for SEUS-style shadow map + GI voxel volume.
 *
 * OptiFine E9 used countInstances/instanceId to draw shadow geometry twice without a
 * geometry shader. Iris does not support that const (Iris ShaderDoc: Instance count ❌).
 * E10/E11 restored this geometry-shader approach; keep it for Iris.
 *
 * Pass 0: sun/moon shadow map (shadowPosition, isVolumePass=0)
 * Pass 1: camera-centered voxel atlas (volumePosition, isVolumePass=1)
 *
 * Pair with shaders.properties: shadow.culling=reversed + const float voxelDistance
 * so near geometry is not frustum-culled out of the voxel volume.
 */

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in vec4 vTexcoord[];
in vec4 vColor[];
in vec4 vViewPos[];
in float vMaterialIDs[];

in float invalidForVolume[];
in vec2 vMidTexCoord[];
in float vFragDepth[];
in float vMCEntity[];
in float vIsWater[];

in vec4 volumePosition[];
in vec4 shadowPosition[];

out vec4 color;
out vec4 texcoord;
out vec4 viewPos;
out float materialIDs;

out float isVolumePass;
out float isWater;
out vec2 midTexCoord;
out float fragDepth;
out float mcEntity;

void emitShadowVertex(int i, float volumePass) {
	color = vColor[i];
	texcoord = vTexcoord[i];
	viewPos = vViewPos[i];
	materialIDs = vMaterialIDs[i];
	midTexCoord = vMidTexCoord[i];
	fragDepth = vFragDepth[i];
	mcEntity = vMCEntity[i];
	isWater = vIsWater[i];
	isVolumePass = volumePass;
	EmitVertex();
}

void main() {
	int i;

	// --- Pass 0: directional shadow map ---
	for (i = 0; i < 3; i++) {
		gl_Position = shadowPosition[i];
		emitShadowVertex(i, 0.0);
	}
	EndPrimitive();

	// --- Pass 1: GI voxel volume (skip invalid / degenerate projected tris) ---
	bool valid = !(invalidForVolume[0] > 0.5 || invalidForVolume[1] > 0.5 || invalidForVolume[2] > 0.5);
	if (!valid) {
		return;
	}

	bool tooLarge =
		distance(volumePosition[0].xy, volumePosition[1].xy) > 1.0 / 1024.0 ||
		distance(volumePosition[0].xy, volumePosition[2].xy) > 1.0 / 1024.0 ||
		distance(volumePosition[1].xy, volumePosition[2].xy) > 1.0 / 1024.0;
	if (tooLarge) {
		return;
	}

	for (i = 0; i < 3; i++) {
		gl_Position = volumePosition[i];
		emitShadowVertex(i, 1.0);
	}
	EndPrimitive();
}
