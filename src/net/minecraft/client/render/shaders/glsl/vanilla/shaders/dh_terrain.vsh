#version 430 core
#include "/lib/common.glsl"
// see third_party/iris/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/DHTerrainTransformer.java
out vec2 lmcoord;
out vec4 color;
out float viewDistance;
out float horizontalDistance;
void main() {
 vec4 viewPosition = gl_ModelViewMatrix * gl_Vertex;
 gl_Position = gl_ProjectionMatrix * viewPosition;
 lmcoord = gl_MultiTexCoord1.xy;
 color = gl_Color;
 viewDistance = fogCoord(viewPosition.xyz);
 horizontalDistance = length(gl_Vertex.xz);
}
