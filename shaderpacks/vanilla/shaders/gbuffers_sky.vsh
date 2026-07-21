in vec3 aPos;
in vec2 aUV;
in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec2 vUV;
out vec4 vColor;
out vec3 vViewPos;

void main() {
  vec4 viewPos = uModelView * vec4(aPos, 1.0);
  gl_Position = uProjection * viewPos;
  vUV = aUV;
  vColor = aColor;
  vViewPos = viewPos.xyz;
}
