#ifndef _CLRWL_MERGE_V
#define _CLRWL_MERGE_V
out ClrwlVertexData {
 vec4 flw_vertexPos;
 vec4 flw_vertexColor;
 vec2 flw_vertexTexCoord;
 flat ivec2 flw_vertexOverlay;
 vec2 flw_vertexLight;
 vec3 flw_vertexNormal;
 vec4 clrwl_vertexTangent;
};
#define clrwl_vertexPos flw_vertexPos
#define clrwl_vertexColor flw_vertexColor
#define clrwl_vertexTexCoord flw_vertexTexCoord
#define clrwl_vertexOverlay flw_vertexOverlay
#define clrwl_vertexLight flw_vertexLight
#define clrwl_vertexNormal flw_vertexNormal
#endif
