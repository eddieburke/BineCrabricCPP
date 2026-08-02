uniform sampler2D readImage;
in vec2 uv;
out vec4 outColor;

vec3 EOTF_Curve(vec3 LinearCV, const float LinearFactor, const float Exponent, const float Alpha, const float Beta) {
 return mix(LinearCV * LinearFactor, clamp(Alpha * pow(LinearCV, vec3(Exponent)) - (Alpha - 1.0), 0.0, 1.0), step(Beta, LinearCV));
}
vec3 EOTF_IEC61966(vec3 LinearCV) {
 return EOTF_Curve(LinearCV, 12.92, 1.0 / 2.4, 1.055, 0.0031308);
}
vec3 InverseEOTF_IEC61966(vec3 DisplayCV) {
 return max(mix(DisplayCV / 12.92, pow(0.947867 * DisplayCV + 0.0521327, vec3(2.4)), step(0.04045, DisplayCV)), 0.0);
}
vec3 EOTF_BT709(vec3 LinearCV) {
 return EOTF_Curve(LinearCV, 4.5, 0.45, 1.099, 0.018);
}
vec3 EOTF_P3DCI(vec3 LinearCV) {
 return pow(LinearCV, vec3(1.0 / 2.6));
}
vec3 EOTF_Adobe(vec3 LinearCV) {
 return pow(LinearCV, vec3(1.0 / 2.2));
}

const mat3 sRGB_XYZ = mat3(
 0.4124564, 0.3575761, 0.1804375,
 0.2126729, 0.7151522, 0.0721750,
 0.0193339, 0.1191920, 0.9503041
);
const mat3 XYZ_P3D65 = mat3(
 2.4933963, -0.9313459, -0.4026945,
 -0.8294868, 1.7626597, 0.0236246,
 0.0358507, -0.0761827, 0.9570140
);
const mat3 XYZ_REC2020 = mat3(
 1.7166511880, -0.3556707838, -0.2533662814,
 -0.6666843518, 1.6164812366, 0.0157685458,
 0.0176398574, -0.0427706133, 0.9421031212
);
const mat3 XYZ_AdobeRGB = mat3(
 2.04158790381075, -0.56500697427886, -0.34473135077833,
 -0.96924363628088, 1.87596750150772, 0.0415550574071756,
 0.0134442806320311, -0.118362392231018, 1.01517499439121
);
const mat3 D65_DCI = mat3(
 1.02449672775258, 0.0151635410224164, 0.0196885223342068,
 0.0256121933371582, 0.972586305624413, 0.00471635229242733,
 0.00638423065008769, -0.0122680827367302, 1.14794244517368
);
const mat3 sRGB_to_P3DCI = ((sRGB_XYZ) * XYZ_P3D65) * D65_DCI;
const mat3 sRGB_to_P3D65 = sRGB_XYZ * XYZ_P3D65;
const mat3 sRGB_to_REC2020 = sRGB_XYZ * XYZ_REC2020;
const mat3 sRGB_to_AdobeRGB = sRGB_XYZ * XYZ_AdobeRGB;

void main() {
#if CURRENT_COLOR_SPACE == SRGB
 outColor = texture(readImage, uv);
#else
 vec4 SourceColor = texture(readImage, uv);
 SourceColor.rgb = InverseEOTF_IEC61966(SourceColor.rgb);
 vec3 TargetColor = SourceColor.rgb;
#if CURRENT_COLOR_SPACE == DCI_P3
 TargetColor = TargetColor * sRGB_to_P3DCI;
 TargetColor = EOTF_P3DCI(TargetColor);
#elif CURRENT_COLOR_SPACE == DISPLAY_P3
 TargetColor = TargetColor * sRGB_to_P3D65;
 TargetColor = EOTF_IEC61966(TargetColor);
#elif CURRENT_COLOR_SPACE == REC2020
 TargetColor = TargetColor * sRGB_to_REC2020;
 TargetColor = EOTF_BT709(TargetColor);
#elif CURRENT_COLOR_SPACE == ADOBE_RGB
 TargetColor = TargetColor * sRGB_to_AdobeRGB;
 TargetColor = EOTF_Adobe(TargetColor);
#endif
 outColor = vec4(TargetColor, SourceColor.a);
#endif
}
