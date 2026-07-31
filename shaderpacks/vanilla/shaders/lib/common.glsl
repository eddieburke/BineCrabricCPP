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

// GL_LINEAR / GL_EXP / GL_EXP2 as Iris reports them. fogMode 0 disables fog.
float fogFactor(float viewDistance) {
 if(fogMode == 1) return clamp((fogEnd - viewDistance) / max(fogEnd - fogStart, 0.0001), 0.0, 1.0);
 if(fogMode == 2) return clamp(exp(-fogDensity * viewDistance), 0.0, 1.0);
 if(fogMode == 3) {
  float scaled = fogDensity * viewDistance;
  return clamp(exp(-scaled * scaled), 0.0, 1.0);
 }
 return 1.0;
}

vec3 applyFog(vec3 surface, float viewDistance) {
 return mix(fogColor, surface, fogFactor(viewDistance));
}

// b1.7.3 face orientation shade — pack-owned (Iris oldLighting=false for world).
float faceShade(vec3 worldNormal) {
 vec3 n = abs(normalize(worldNormal));
 if(n.y > n.x && n.y > n.z) {
  return worldNormal.y >= 0.0 ? 1.0 : 0.5;
 }
 if(n.z > n.x) {
  return 0.8;
 }
 return 0.6;
}

// Iris separateAo: AO lives in vaColor.a for terrain; rgb stays tint-only.
// When separateAo=false, engine premultiplies AO into rgb and sets a=1 — rgb*a
// is still correct. Never use color.a for cutout discard (that punches holes).
// https://shaders.properties/current/reference/attributes/gl_color/
vec3 tintAndAo(vec4 vertexColor) {
 return vertexColor.rgb * vertexColor.a;
}
