#version 430 core
#include "/lib/common.glsl"
// see third_party/iris/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/DHTerrainTransformer.java
out vec2 lmcoord;
out vec4 color;
out float viewDistance;
out vec2 horizontalPosition;
out vec3 nativeViewPosition;
void main() {
 vec4 viewPosition = gl_ModelViewMatrix * gl_Vertex;
 nativeViewPosition = viewPosition.xyz;
 gl_Position = gl_ProjectionMatrix * viewPosition;
 lmcoord = gl_MultiTexCoord1.xy;
 color = gl_Color;
 viewDistance = fogCoord(viewPosition.xyz);
 horizontalPosition = gl_Vertex.xz;
}
