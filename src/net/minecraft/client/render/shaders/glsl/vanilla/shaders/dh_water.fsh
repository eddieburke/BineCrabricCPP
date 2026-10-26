#version 430 core
#include "/lib/common.glsl"
in vec2 lmcoord;
in vec4 color;
in float viewDistance;
in vec2 horizontalPosition;
in vec3 nativeViewPosition;
uniform sampler2D lightmap;
uniform sampler2D depthtex1;
uniform mat4 gbufferProjection;
uniform float clipDistance;
/* RENDERTARGETS: 0 */
layout(location = 0) out vec4 outColor;
void main() {
 if(length(horizontalPosition) < clipDistance) discard;
 vec4 nativeClip = gbufferProjection * vec4(nativeViewPosition, 1.0);
 float nativeDepth = nativeClip.z / nativeClip.w * 0.5 + 0.5;
 if(nativeDepth >= texelFetch(depthtex1, ivec2(gl_FragCoord.xy), 0).r) discard;
 vec3 surface = color.rgb * texture(lightmap, lmcoord).rgb;
 outColor = vec4(applyFog(surface, viewDistance), color.a);
}
