#version 430 core
#include "/lib/common.glsl"
// see third_party/iris/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/DHTerrainTransformer.java
out vec3 vertexLight;
uniform sampler2D lightmap;
out vec4 color;
out float viewDistance;
out vec2 horizontalPosition;
out vec3 nativeViewPosition;
out float directionalShade;
void main() {
 vec4 viewPosition = gl_ModelViewMatrix * gl_Vertex;
 nativeViewPosition = viewPosition.xyz;
 gl_Position = gl_ProjectionMatrix * viewPosition;
 vec2 lmcoord = gl_MultiTexCoord1.xy;
 vertexLight = texture(lightmap, lmcoord).rgb;
 color = gl_Color;
 viewDistance = fogCoord(viewPosition.xyz);
 horizontalPosition = gl_Vertex.xz - gl_Normal.xz * 0.001;
 directionalShade = faceShade(gl_Normal);
}
