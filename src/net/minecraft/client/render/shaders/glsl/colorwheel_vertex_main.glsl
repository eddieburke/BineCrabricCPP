 flw_vertexPos = vec4(vaPosition + chunkOffset, 1.0);
 flw_vertexColor = vaColor;
 flw_vertexTexCoord = vaUV0;
 flw_vertexOverlay = iris_UV1;
 flw_vertexLight = clamp(vaUV2 / 256.0, 0.0, 0.9375);
 flw_vertexNormal = vaNormal;
 clrwl_vertexTangent = at_tangent;
