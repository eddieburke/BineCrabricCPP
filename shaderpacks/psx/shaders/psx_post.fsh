in vec2 vUV;

uniform sampler2D colortex0;
uniform sampler2D depthtex0;
uniform vec2 uViewport;

out vec4 fragColor;

const mat4 bayer4x4 = mat4(
    0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
   12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
    3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
   15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
);

vec3 applyTheme(vec3 color, float lum) {
#if PSX_PALETTE_THEME == 1
  vec3 amberDark = vec3(0.05, 0.02, 0.0);
  vec3 amberLight = vec3(1.0, 0.7, 0.1);
  return mix(amberDark, amberLight, lum);
#elif PSX_PALETTE_THEME == 2
  vec3 gb0 = vec3(0.06, 0.22, 0.06);
  vec3 gb1 = vec3(0.19, 0.38, 0.19);
  vec3 gb2 = vec3(0.54, 0.67, 0.06);
  vec3 gb3 = vec3(0.61, 0.73, 0.06);
  if(lum < 0.25) return gb0;
  if(lum < 0.50) return gb1;
  if(lum < 0.75) return gb2;
  return gb3;
#elif PSX_PALETTE_THEME == 3
  vec3 sepia = vec3(lum * 1.2, lum * 0.95, lum * 0.75);
  return clamp(sepia, 0.0, 1.0);
#elif PSX_PALETTE_THEME == 4
  vec3 cyan = vec3(0.0, 0.9, 0.9);
  vec3 magenta = vec3(0.9, 0.1, 0.7);
  vec3 tint = mix(cyan, magenta, color.r);
  return mix(color, tint, 0.45);
#elif PSX_PALETTE_THEME == 5
  vec3 dark = vec3(lum * 0.3, lum * 0.35, lum * 0.4);
  vec3 blood = vec3(lum * 1.4, lum * 0.2, lum * 0.2);
  return mix(dark, blood, smoothstep(0.6, 0.9, color.r));
#else
  return color;
#endif
}

void main() {
  vec4 rawColor = texture(colortex0, clamp(vUV, 0.0, 1.0));
  vec3 color = rawColor.rgb;

  float ditherValue = 0.0;
#if PSX_DITHER_ENABLE
  ivec2 dCoord = (ivec2(floor(gl_FragCoord.xy / max(float(PSX_DITHER_SCALE), 1.0))) % 4 + 4) % 4;
  ditherValue = (bayer4x4[dCoord.x][dCoord.y] - 0.5) * PSX_DITHER_STRENGTH;
#endif

#if PSX_PALETTE_ENABLE
  float steps = max(float(PSX_COLOR_DEPTH), 2.0);
  color += vec3(ditherValue / steps);
  color = floor(color * (steps - 1.0) + 0.5) / (steps - 1.0);
  color = clamp(color, 0.0, 1.0);
#else
  color += vec3(ditherValue * 0.1);
  color = clamp(color, 0.0, 1.0);
#endif

  float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
  color = applyTheme(color, luminance);

#if PSX_CRT_SCANLINES
  float scanline = 0.92 + 0.08 * sin(gl_FragCoord.y * 3.14159265);
  color *= scanline;
#endif

  fragColor = vec4(clamp(color, 0.0, 1.0), rawColor.a);
}
