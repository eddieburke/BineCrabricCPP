// Shared by both stages of the base pack. No `discard` lives here — that is only
// legal in a fragment shader and this file is included by the vertex stage too.
// Every name is part of the Iris/OptiFine contract, so a pack that replaces a
// program sees exactly the inputs it expects.
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 chunkOffset;
// ShaderDoc: fogColor / skyColor are vec3.
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;
uniform int fogMode;
// Zero when the engine has the alpha test off, so a program can always compare
// against it — Iris has no "is the test enabled" uniform.
uniform float alphaTestRef;

// Beta's fog coordinate is fixed-function GL_FRAGMENT_DEPTH: the eye-PLANE distance,
// not the radial one. Radial made the wall 1.74x closer at the screen corners than at
// the centre (70 deg FOV, 16:9), which reads as a wall curving in rather than as haze.
// Not modern Minecraft's fogShape sphere/cylinder either — b1.7.3 predates both, and
// cylinder still inflates the corner by 1.59x.
float fogCoord(vec3 viewPosition) {
 return abs(viewPosition.z);
}

// GL_LINEAR / GL_EXP2 as the GL fog-mode constants. fogMode 0 disables fog.
float fogFactor(float viewDistance) {
 if(fogMode == 9729) return clamp((fogEnd - viewDistance) / max(fogEnd - fogStart, 0.0001), 0.0, 1.0);
 if(fogMode == 2049) {
  float scaled = fogDensity * viewDistance;
  return clamp(exp(-scaled * scaled), 0.0, 1.0);
 }
 return 1.0;
}

vec3 applyFog(vec3 surface, float viewDistance) {
 return mix(fogColor, surface, fogFactor(viewDistance));
}

// b1.7.3 face orientation shade — pack-owned (Iris oldLighting=false for world).
//
// Vanilla keys this off a face *Direction*, never off a normal: the game applies it in
// CardinalLighting.byFace while baking quads, and Iris gates the whole effect on the
// oldLighting directive (IrisRenderingPipeline.shouldDisableDirectionalShading returns
// !oldLighting, and MixinClientLevel forces Direction.UP when that is true). A Direction
// only exists for axis-aligned quads, so vanilla leaves everything else unshaded — cross
// and crop plants, torches, rails, ladders, fire and redstone all render at full
// brightness. Reconstructing that from an interpolated normal therefore needs an explicit
// "no dominant axis" case, which is what the final return is.
//
// Without it, cross-plant quads strobed. Their normal is (±0.7071, 0, ∓0.7071), so |n.x|
// and |n.z| are mathematically EQUAL and the old `n.z > n.x` test sat exactly on its own
// tie-break. The normal arrives here via
//   vaNormal (normalized signed byte) -> mat3(modelViewMatrix) -> normalize
//     -> interpolate across the primitive -> mat3(gbufferModelViewInverse) -> normalize
// and every step injects round-off of ~1e-7 whose SIGN depends on the camera matrix.
// The camera matrix changes each frame (view bobbing), so the branch flipped per frame
// and per fragment, swinging plants between 0.8 and 0.6.
//
// axisEpsilon sits far above that round-off and far below any real margin — an
// axis-aligned normal wins its axis by 1.0, and even a flowing-water top still wins Y by
// ~0.9 — so it only ever fires on a genuine tie.
float faceShade(vec3 worldNormal) {
 const float axisEpsilon = 1.0 / 64.0;
 vec3 n = abs(normalize(worldNormal));
 if(n.y >= max(n.x, n.z) + axisEpsilon) {
  return worldNormal.y >= 0.0 ? 1.0 : 0.5;
 }
 if(n.z >= n.x + axisEpsilon) {
  return 0.8;
 }
 if(n.x >= n.z + axisEpsilon) {
  return 0.6;
 }
 return 1.0;
}

// Iris separateAo: AO lives in vaColor.a for terrain; rgb stays tint-only.
// When separateAo=false, engine premultiplies AO into rgb and sets a=1 — rgb*a
// is still correct. Never use color.a for cutout discard (that punches holes).
// https://shaders.properties/current/reference/attributes/gl_color/
vec3 tintAndAo(vec4 vertexColor) {
 return vertexColor.rgb * vertexColor.a;
}
