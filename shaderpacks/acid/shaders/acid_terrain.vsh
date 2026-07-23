in vec3 aPos;
in vec2 aUV;
in vec4 aColor;
in vec3 aNormal;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform float uWorldTime;

out vec2 vUV;
out vec4 vColor;
out float vEyeDist;

void main() {
  vec4 baseViewPos = uModelView * vec4(aPos, 1.0);
  vec3 relPos = transpose(mat3(uModelView)) * baseViewPos.xyz;

  float phase = uWorldTime * ACID_SPEED;

  float warpX = sin(relPos.y * 0.8 + relPos.x * 0.3 + phase * 1.1) * ACID_TERRAIN_WARP * 0.6;
  float warpY = cos(relPos.x * 0.6 + relPos.z * 0.4 + phase * 0.9) * ACID_TERRAIN_WARP * 0.3;
  float warpZ = sin(relPos.z * 0.7 + relPos.y * 0.5 + phase * 1.3) * ACID_TERRAIN_WARP * 0.6;

  vec3 displacement = vec3(warpX, warpY, warpZ);
  vec3 displacedViewPos = baseViewPos.xyz + mat3(uModelView) * displacement;

  gl_Position = uProjection * vec4(displacedViewPos, 1.0);

  vUV = aUV;
  vColor = aColor;
  vEyeDist = length(displacedViewPos);
}
