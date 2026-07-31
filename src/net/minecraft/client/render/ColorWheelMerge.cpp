#include "net/minecraft/client/render/ColorWheelMerge.hpp"
#include "net/minecraft/client/render/SourceProcessor.hpp"
#include <array>
#include <string_view>

namespace net::minecraft::client::render::glutil {
namespace {

// https://djefrey.github.io/colorwheel/reference/attributes/overview/
constexpr const char* kVertexBridge = R"GLSL(
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
)GLSL";

constexpr const char* kVertexMain = R"GLSL(
 flw_vertexPos = vec4(vaPosition + chunkOffset, 1.0);
 flw_vertexColor = vaColor;
 flw_vertexTexCoord = vaUV0;
 flw_vertexOverlay = ivec2(0);
 flw_vertexLight = clamp(vaUV2 / 240.0, 0.0, 1.0);
 flw_vertexNormal = vaNormal;
 clrwl_vertexTangent = vec4(1.0, 0.0, 0.0, 1.0);
)GLSL";

struct AttrDecl {
 std::string_view storage;
 std::string_view alternate;
 std::string_view type;
 std::string_view name;
};

constexpr std::array kVertexAttrDecls = {
    AttrDecl{"in", "attribute", "vec3", "vaPosition"},
    AttrDecl{"in", "attribute", "vec2", "vaUV0"},
    AttrDecl{"in", "attribute", "vec2", "vaUV2"},
    AttrDecl{"in", "attribute", "vec4", "vaColor"},
    AttrDecl{"in", "attribute", "vec3", "vaNormal"},
    AttrDecl{"uniform", {}, "vec3", "chunkOffset"},
};

void ensureVertexAttrs(std::string& source) {
 std::string decls;
 for(const AttrDecl& d : kVertexAttrDecls) {
  if(!referencesToken(source, d.name)) continue;
  if(hasStorageDeclaration(source, d.storage, d.alternate, d.name)) continue;
  decls += std::string(d.storage) + " " + std::string(d.type) + " " + std::string(d.name) + ";\n";
 }
 if(!decls.empty()) source.insert(sourceDeclarationOffset(source), decls);
}

// https://djefrey.github.io/colorwheel/reference/miscellaneous/functions/
std::string fragmentBridge(bool translucent) {
 const char* cutout =
     translucent ? "bool flw_discardPredicate(vec4 color) { return false; }\n"
                 : "bool flw_discardPredicate(vec4 color) { return color.a < 0.1; }\n";
 const char* discardFn =
     translucent ? "void clrwl_computeDiscard(vec4 color) {}\n"
                 : "void clrwl_computeDiscard(vec4 color) {\n"
                   " if (flw_discardPredicate(color)) discard;\n"
                   "}\n";
 return std::string(R"GLSL(
#ifndef _CLRWL_MERGE_F
#define _CLRWL_MERGE_F
struct FlwMaterial {
 bool blur;
 bool mipmap;
 bool backfaceCulling;
 bool polygonOffset;
 uint depthTest;
 uint transparency;
 uint writeMask;
 bool useOverlay;
 bool useLight;
 uint cardinalLightingMode;
 bool ambientOcclusion;
};
in ClrwlVertexData {
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
vec4 flw_sampleColor;
vec4 flw_fragColor;
ivec2 flw_fragOverlay;
vec2 flw_fragLight;
FlwMaterial flw_material;
vec4 clrwl_overlayColor = vec4(0.0);
uniform sampler2D flw_diffuseTex;
uniform sampler2D flw_overlayTex;
void flw_materialFragment() {}
void flw_shaderLight() {}
)GLSL") + cutout + discardFn + R"GLSL(
void clrwl_getDebugColor(inout vec4 color) {}
void _clrwl_materialFragment_hook() {
 flw_materialFragment();
 if (flw_material.useOverlay) {
  clrwl_overlayColor = texelFetch(flw_overlayTex, flw_fragOverlay, 0);
  clrwl_overlayColor.a = 1.0 - clrwl_overlayColor.a;
 }
}
void _clrwl_shaderLight_hook() { flw_shaderLight(); }
void clrwl_computeFragment(vec4 sampleColor, out vec4 fragColor, out vec2 fragLight, out float ao, out vec4 fragOverlay) {
 flw_material.blur = false;
 flw_material.mipmap = true;
 flw_material.backfaceCulling = true;
 flw_material.polygonOffset = false;
 flw_material.depthTest = 4u;
 flw_material.transparency = 0u;
 flw_material.writeMask = 0u;
 flw_material.useOverlay = false;
 flw_material.useLight = true;
 flw_material.cardinalLightingMode = 0u;
 flw_material.ambientOcclusion = true;
 flw_sampleColor = sampleColor;
 flw_fragColor = flw_sampleColor * flw_vertexColor;
 flw_fragLight = flw_vertexLight;
 flw_fragOverlay = flw_vertexOverlay;
 _clrwl_materialFragment_hook();
 vec4 fragColorBfLight = flw_fragColor;
 _clrwl_shaderLight_hook();
 if (flw_material.ambientOcclusion) {
  vec3 fragColorLightRatio = flw_fragColor.rgb / fragColorBfLight.rgb;
  ao = clamp(max(max(fragColorLightRatio.r, fragColorLightRatio.g), fragColorLightRatio.b), 0.0, 1.0);
 } else {
  ao = 1.0;
 }
 clrwl_computeDiscard(flw_fragColor);
 clrwl_getDebugColor(flw_fragColor);
 fragColor = flw_fragColor;
 fragLight = flw_fragLight + 1.0 / 32.0;
 fragOverlay = clrwl_overlayColor;
}
#endif
)GLSL";
}

constexpr const char* kGeometryBridge = R"GLSL(
#ifndef _CLRWL_MERGE_G
#define _CLRWL_MERGE_G
void clrwl_setVertexOut(int i) {}
#endif
)GLSL";

void insertOnce(std::string& source, std::string_view guard, const std::string& block) {
 if(source.find(guard) != std::string::npos) return;
 source.insert(sourceDeclarationOffset(source), block);
}

}

bool isColorWheelProgramName(std::string_view programName) {
 return programName.rfind("clrwl_", 0) == 0;
}

void appendColorWheelMacros(std::string& preamble) {
 preamble += "#define HAS_COLORWHEEL\n#define COLORWHEEL_VERSION ";
 preamble += colorWheelVersionMacro();
 preamble += "\n"
             "#define CLRWL_RENDER_STAGE_SOLID 110800\n"
             "#define CLRWL_RENDER_STAGE_TRANSLUCENT 110801\n"
             "#define CLRWL_RENDER_STAGE_OIT_DEPTH_RANGE 110802\n"
             "#define CLRWL_RENDER_STAGE_OIT_COEFFICIENTS 110803\n"
             "#define CLRWL_RENDER_STAGE_OIT_ACCUMULATE 110804\n"
             "#define CLRWL_RENDER_STAGE_OIT_COMPOSITE 110805\n"
             "#define CLRWL_RENDER_STAGE_CRUMBLING 110806\n";
}

std::string mergeColorWheelMaterial(const std::string& programName, ShaderStage stage,
                                    std::string source) {
 if(!isColorWheelProgramName(programName)) return source;
 switch(stage) {
  case ShaderStage::Vertex:
   insertOnce(source, "_CLRWL_MERGE_V", kVertexBridge);
   if(source.find("flw_vertexPos = vec4(vaPosition") == std::string::npos)
    appendBeforeMainClose(source, kVertexMain);
   ensureVertexAttrs(source);
   return source;
  case ShaderStage::Fragment:
   insertOnce(source, "_CLRWL_MERGE_F",
              fragmentBridge(programName.find("translucent") != std::string::npos));
   return source;
  case ShaderStage::Other:
   if(referencesToken(source, "clrwl_setVertexOut"))
    insertOnce(source, "_CLRWL_MERGE_G", kGeometryBridge);
   return source;
 }
 return source;
}
}
