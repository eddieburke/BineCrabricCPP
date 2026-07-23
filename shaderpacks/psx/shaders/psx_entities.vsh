in vec3 aPos;
in vec2 aUV;
in vec4 aColor;
in vec3 aNormal;

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec2 vUV;
out vec4 vColor;
out vec3 vNormal;
out vec3 vViewPos;
out float vAffineW;

void main() {
  vec4 viewPos = uModelView * vec4(aPos, 1.0);
  vec4 clipPos = uProjection * viewPos;

#if PSX_VERTEX_SNAP
  if(clipPos.w > 0.0) {
    vec3 ndc = clipPos.xyz / clipPos.w;
    vec2 grid = vec2(PSX_SNAP_GRID);
    vec2 snapped = floor(ndc.xy * grid + 0.5) / grid;
    ndc.xy = mix(ndc.xy, snapped, clamp(float(PSX_JITTER_STRENGTH), 0.0, 1.0));
    clipPos.xyz = ndc * clipPos.w;
  }
#endif

  gl_Position = clipPos;
#if defined(PSX_AFFINE_WARP)
  float affineFactor = clamp(float(PSX_AFFINE_WARP), 0.0, 1.0);
  vAffineW = mix(1.0, max(clipPos.w, 0.001), affineFactor);
#else
  vAffineW = max(clipPos.w, 0.001);
#endif
  vUV = aUV * vAffineW;
  vColor = aColor;
  vNormal = mat3(uModelView) * aNormal;
  vViewPos = viewPos.xyz;
}
