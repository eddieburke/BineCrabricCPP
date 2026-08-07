#ifndef _CLRWL_MERGE_G
#define _CLRWL_MERGE_G
in ClrwlVertexData {
 vec4 flw_vertexPos;
 vec4 flw_vertexColor;
 vec2 flw_vertexTexCoord;
 flat ivec2 flw_vertexOverlay;
 vec2 flw_vertexLight;
 vec3 flw_vertexNormal;
 vec4 clrwl_vertexTangent;
} clrwl_in[3];
out ClrwlVertexData {
 vec4 flw_vertexPos;
 vec4 flw_vertexColor;
 vec2 flw_vertexTexCoord;
 flat ivec2 flw_vertexOverlay;
 vec2 flw_vertexLight;
 vec3 flw_vertexNormal;
 vec4 clrwl_vertexTangent;
} clrwl_out;
void clrwl_setVertexOut(int i) {
 flw_vertexPos = clrwl_in[i].flw_vertexPos;
 flw_vertexColor = clrwl_in[i].flw_vertexColor;
 flw_vertexTexCoord = clrwl_in[i].flw_vertexTexCoord;
 flw_vertexOverlay = clrwl_in[i].flw_vertexOverlay;
 flw_vertexLight = clrwl_in[i].flw_vertexLight;
 flw_vertexNormal = clrwl_in[i].flw_vertexNormal;
 clrwl_vertexTangent = clrwl_in[i].clrwl_vertexTangent;
}
#endif
