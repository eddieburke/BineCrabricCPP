in vec2 vUV;
in vec4 vColor;
in float vEyeDist;

uniform sampler2D uTexture;
uniform int uAlphaTest;
uniform float uAlphaRef;
uniform int uFogMode;
uniform vec4 uFogColor;
uniform float uFogDensity;
uniform float uFogStart;
uniform float uFogEnd;
uniform float uWorldTime;

out vec4 fragColor;

void main() {
  vec4 texColor = texture(uTexture, vUV);
  vec4 color = texColor * vColor;

  if(uAlphaTest == 1 && color.a < uAlphaRef) {
    discard;
  }

  float fog = 0.0;
  if(uFogMode == 1) {
    fog = clamp((vEyeDist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
  } else if(uFogMode == 2) {
    fog = 1.0 - exp(-uFogDensity * vEyeDist);
    fog = clamp(fog, 0.0, 1.0);
  }

  color.rgb = mix(color.rgb, uFogColor.rgb, fog);

  fragColor = color;
}
