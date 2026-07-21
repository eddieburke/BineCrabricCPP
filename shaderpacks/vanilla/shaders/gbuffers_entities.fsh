in vec2 vUV;
in vec4 vColor;
in vec3 vNormal;
in vec3 vViewPos;

uniform sampler2D uTexture;
uniform int uUseTexture;
uniform vec4 uConstColor;

uniform int uLighting;
uniform vec3 uSunDirectionView;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform vec3 uFillDirectionView;
uniform float uFillIntensity;
uniform vec3 uAmbient;
uniform float uBrightness;

uniform int uFogMode;
uniform vec4 uFogColor;
uniform float uFogDensity;
uniform float uFogStart;
uniform float uFogEnd;

layout(location = 0) out vec4 fragColor;

void main() {
  vec4 base = uConstColor * vColor;
  if(uUseTexture == 1) {
    base *= texture(uTexture, vUV);
  }
  if(base.a <= 0.1) {
    discard;
  }
  float nLen = length(vNormal);
  if(uLighting == 1 && nLen > 1e-4) {
    vec3 n = vNormal / nLen;
    float d = max(dot(n, normalize(uSunDirectionView)), 0.0);
    vec3 lit = uAmbient + uSunColor * (d * uSunIntensity);
    if(uFillIntensity > 0.0) {
      float df = max(dot(n, normalize(uFillDirectionView)), 0.0);
      lit += uSunColor * (df * uFillIntensity);
    }
    base.rgb *= clamp(lit, 0.0, 1.0);
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
