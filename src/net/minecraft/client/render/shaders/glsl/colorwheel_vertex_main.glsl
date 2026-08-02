 flw_vertexPos = vec4(vaPosition + chunkOffset, 1.0);
 flw_vertexColor = vaColor;
 flw_vertexTexCoord = vaUV0;
 flw_vertexOverlay = ivec2(0);
 flw_vertexLight = clamp(vaUV2 / 240.0, 0.0, 1.0);
 flw_vertexNormal = vaNormal;
 clrwl_vertexTangent = vec4(1.0, 0.0, 0.0, 1.0);
