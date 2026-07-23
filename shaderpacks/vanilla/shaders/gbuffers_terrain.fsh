in vec2 vUV;
in vec4 vColor;
in vec3 vNormal;
in vec3 vViewPos;

uniform sampler2D uTexture;
uniform sampler2D uLightMap;
uniform vec4 uConstColor;
uniform int uAlphaTest;
uniform float uAlphaRef;

uniform float uBrightness;

uniform int uFogMode;
uniform vec4 uFogColor;
uniform float uFogDensity;
uniform float uFogStart;
uniform float uFogEnd;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 base = uConstColor * vColor * texture(uTexture, vUV);
  if(uAlphaTest == 1 && base.a <= uAlphaRef) {
    discard;
  }
  if(uFogMode > 0) {
    float dist = length(vViewPos);
    float f;
    if(uFogMode == 1) {
      f = (uFogEnd - dist) / max(uFogEnd - uFogStart, 1e-4);
    } else if(uFogMode == 2) {
      f = exp(-uFogDensity * dist);
    } else {
      float fd = uFogDensity * dist;
      f = exp(-(fd * fd));
    }
    f = clamp(f, 0.0, 1.0);
    base.rgb = mix(uFogColor.rgb, base.rgb, f);
  }
  base.rgb = mix(base.rgb, vec3(1.0), uBrightness);
  fragColor = base;
}
