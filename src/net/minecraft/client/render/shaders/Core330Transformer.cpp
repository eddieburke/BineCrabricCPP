#include "net/minecraft/client/render/shaders/Core330Transformer.hpp"
#include <array>
#include <cctype>
#include <string_view>
#include <utility>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/EntityPatcher.hpp"
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include "net/minecraft/client/render/shaders/GlslSource.hpp"

namespace net::minecraft::client::render {
namespace {
using PackCatalog::lower;

struct SourceDeclaration {
 std::string_view storage;
 std::string_view type;
 std::string_view name;
};

template <std::size_t N>
void appendMissingDeclarations(std::string& output,
                               const std::string& source,
                               const std::array<SourceDeclaration, N>& declarations) {
 for(const SourceDeclaration& declaration : declarations) {
  if(referencesToken(source, declaration.name) &&
     !hasStorageDeclaration(source, declaration.storage, declaration.name)) {
   output += std::string(declaration.storage) + " " + std::string(declaration.type) + " " +
             std::string(declaration.name) + ";\n";
  }
 }
}

constexpr std::array kCompositeUniforms = {
    SourceDeclaration{"uniform", "mat4", "modelViewMatrix"},
    SourceDeclaration{"uniform", "mat4", "projectionMatrix"},
    SourceDeclaration{"uniform", "mat4", "modelViewProjectionMatrix"},
    SourceDeclaration{"uniform", "mat4", "textureMatrix"}};
constexpr std::array kGbufferUniforms = {
    SourceDeclaration{"uniform", "mat4", "modelViewMatrixInverse"},
    SourceDeclaration{"uniform", "mat4", "projectionMatrixInverse"},
    SourceDeclaration{"uniform", "mat3", "normalMatrix"},
    SourceDeclaration{"uniform", "vec3", "chunkOffset"}};
constexpr std::array kVertexAttributes = {
    SourceDeclaration{"in", "vec3", "vaPosition"},
    SourceDeclaration{"in", "vec2", "vaUV0"},
    SourceDeclaration{"in", "vec2", "vaUV2"},
    SourceDeclaration{"in", "vec4", "vaColor"},
    SourceDeclaration{"in", "vec3", "vaNormal"}};

bool isGbufferOrShadowProgramName(const std::string& programName) {
 const std::string_view name = programName;
 return name.starts_with("gbuffers_") || name.starts_with("clrwl_gbuffers") || name == "shadow" ||
        name.starts_with("shadow_") || name.starts_with("clrwl_shadow");
}

bool patchMultiTexCoord3(std::string& source, std::string& declarations) {
 if(!referencesToken(source, "gl_MultiTexCoord3") || referencesToken(source, "mc_midTexCoord") ||
    hasStorageDeclaration(source, "in", "mc_midTexCoord") ||
    hasStorageDeclaration(source, "uniform", "mc_midTexCoord") ||
    hasStorageDeclaration(source, "const", "mc_midTexCoord")) return false;
 replaceAllToken(source, "gl_MultiTexCoord3", "mc_midTexCoord");
 declarations += "in vec4 mc_midTexCoord;\n";
 return true;
}

void replaceGlMultiTexCoordBounded(std::string& source, int minimum, int maximum) {
 for(int index = minimum; index <= maximum; ++index)
  replaceAllToken(source, "gl_MultiTexCoord" + std::to_string(index), "vec4(0.0, 0.0, 0.0, 1.0)");
}

// Programs whose geometry comes from the chunk mesher. Iris routes exactly this set
// through SodiumTransformer, which computes a real per-chunk fade; everything else gets
// VanillaTransformer's `const float mc_chunkFade = -1.0;`.
// gbuffers_water belongs here — water and ice are chunk geometry, not a separate model
// path. Classifying it as "other" gave it the -1.0 const, so RenderPearl's
//   float16_t alpha = float16_t(mc_chunkFade);  // prog/lit.vsh, IRIS_FEATURE_FADE_VARIABLE
//   if (fluid) alpha *= float16_t(WATER_OPACITY * 0.01);
// produced a negative alpha, packed it to 0 through an out-of-range float->uint
// conversion, and `blend.gbuffers_water=SRC_ALPHA ...` then erased every water and ice
// fragment. See docs/agent-notes/HANDOFF-visual-symptoms-2026-08-02.md section B.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/SodiumTransformer.java
[[nodiscard]] bool isChunkMesherProgram(std::string_view programName) {
 return programName.starts_with("gbuffers_terrain") || programName == "gbuffers_water";
}
[[nodiscard]] bool isShadowProgramName(std::string_view programName) {
 return programName == "shadow" || programName.starts_with("shadow_") ||
        programName.starts_with("clrwl_shadow");
}
std::string injectChunkFadeAttribute(const std::string& programName,
                                     const PackDefinition& pack,
                                     std::string source) {
 const bool enabled = pack.optionalFeatures.contains("FADE_VARIABLE") ||
                      pack.requiredFeatures.contains("FADE_VARIABLE");
 const bool declared = hasStorageDeclaration(source, "in", "mc_chunkFade") ||
                       hasStorageDeclaration(source, "uniform", "mc_chunkFade") ||
                       hasStorageDeclaration(source, "const", "mc_chunkFade");
 const std::string_view name = programName;
 const bool shadow = isShadowProgramName(name);
 // Shadow programs previously fell out of this gate entirely and got no declaration at
 // all, which is a compile failure waiting for any pack that shares a vertex body between
 // gbuffers_water and shadow_water. Iris always gives the shadow pass the -1.0 const
 // (SodiumTransformer.java:124, `parameters.shadow` branch).
 if(!enabled || declared || !(name.starts_with("gbuffers_") || shadow)) return source;
 source.insert(sourceDeclarationOffset(source),
               (!shadow && isChunkMesherProgram(name)) ? GlslSnippets::get("chunk_fade_terrain_in")
                                                       : GlslSnippets::get("chunk_fade_other_const"));
 return source;
}

std::string lowerVertexSource(const std::string& programName, std::string source) {
 const bool gbufferOrShadow = isGbufferOrShadowProgramName(programName);
 const bool distantHorizons = std::string_view(programName).starts_with("dh_");
 const bool hasLegacy = source.find("ftransform") != std::string::npos ||
                        source.find("gl_Vertex") != std::string::npos ||
                        source.find("gl_Color") != std::string::npos ||
                        source.find("gl_Normal") != std::string::npos ||
                        source.find("gl_MultiTexCoord") != std::string::npos ||
                        source.find("gl_ModelView") != std::string::npos ||
                        source.find("gl_Projection") != std::string::npos ||
                        source.find("gl_TextureMatrix") != std::string::npos;
 if(hasLegacy) {
  const std::string position = gbufferOrShadow ? "vaPosition + chunkOffset" : "vaPosition";
  replaceAllToken(source, "ftransform()", "(projectionMatrix * modelViewMatrix * vec4(" + position + ", 1.0))");
  replaceAllToken(source, "gl_Vertex", "vec4(" + position + ", 1.0)");
  replaceAllToken(source, "gl_MultiTexCoord2", "gl_MultiTexCoord1");
  constexpr std::array replacements = {
      std::pair{"gl_ModelViewProjectionMatrix", "modelViewProjectionMatrix"},
      std::pair{"gl_ModelViewMatrixInverse", "modelViewMatrixInverse"},
      std::pair{"gl_ProjectionMatrixInverse", "projectionMatrixInverse"},
      std::pair{"gl_ModelViewMatrix", "modelViewMatrix"},
      std::pair{"gl_ProjectionMatrix", "projectionMatrix"},
      std::pair{"gl_NormalMatrix", "normalMatrix"},
      std::pair{"gl_TextureMatrix[1]", "iris_lightmapTextureMatrix"},
      std::pair{"gl_TextureMatrix[0]", "textureMatrix"},
      std::pair{"gl_MultiTexCoord1", "vec4(vaUV2, 0.0, 1.0)"},
      std::pair{"gl_MultiTexCoord0", "vec4(vaUV0, 0.0, 1.0)"},
      std::pair{"gl_Color", "vaColor"},
      std::pair{"gl_Normal", "vaNormal"}};
  for(const auto& [legacy, current] : replacements) replaceAllToken(source, legacy, current);
  replaceGlMultiTexCoordBounded(source, 4, 7);
 }
 std::string declarations;
 appendMissingDeclarations(declarations, source, kVertexAttributes);
 appendMissingDeclarations(declarations, source, kCompositeUniforms);
 if(gbufferOrShadow || distantHorizons) appendMissingDeclarations(declarations, source, kGbufferUniforms);
 if(distantHorizons && referencesToken(source, "dhMaterialId") &&
    !hasStorageDeclaration(source, "in", "dhMaterialId")) {
  declarations += "in int dhMaterialId;\n";
 }
 patchMultiTexCoord3(source, declarations);
 if(referencesToken(source, "iris_lightmapTextureMatrix") &&
    source.find("const mat4 iris_lightmapTextureMatrix") == std::string::npos) {
  declarations += GlslSnippets::get("iris_lightmap_matrix");
 }
 if(referencesToken(source, "gl_FogFragCoord")) {
  replaceAllToken(source, "gl_FogFragCoord", "iris_FogFragCoord");
  if(!hasStorageDeclaration(source, "out", "iris_FogFragCoord")) {
   declarations += GlslSnippets::get("iris_fog_frag_coord_vertex_out");
  }
  prependToMainBody(source, GlslSnippets::get("iris_fog_frag_coord_init_main"));
 }
 if(referencesToken(source, "gl_FrontColor")) {
  replaceAllToken(source, "gl_FrontColor", "iris_FrontColor");
  declarations += GlslSnippets::get("iris_front_color_global");
 }
 if(!declarations.empty()) source.insert(sourceDeclarationOffset(source), declarations);
 return source;
}

bool programGetsCompatAlphaTest(const std::string& programName) {
 const std::string_view name = programName;
 if(name == "shadow" || name.starts_with("shadow_") || name.starts_with("clrwl_shadow")) {
  return !name.starts_with("shadowcomp");
 }
 if(!name.starts_with("gbuffers_") && !name.starts_with("clrwl_gbuffers")) return false;
 const std::string lowerName = lower(programName);
 return lowerName.find("water") == std::string::npos && lowerName.find("translucent") == std::string::npos;
}

bool replaceFunctionCalls(std::string& source, std::string_view from, std::string_view to) {
 const std::vector<bool> mask = codeMask(source);
 std::vector<std::size_t> matches;
 std::size_t at = 0;
 while((at = source.find(from, at)) != std::string::npos) {
  if(tokenAt(source, mask, at, from)) {
   std::size_t next = at + from.size();
   while(next < source.size() && std::isspace(static_cast<unsigned char>(source[next]))) ++next;
   if(next < source.size() && mask[next] && source[next] == '(') matches.push_back(at);
  }
  at += from.size();
 }
 for(auto match = matches.rbegin(); match != matches.rend(); ++match) source.replace(*match, from.size(), to);
 return !matches.empty();
}

std::array<bool, 16> rewriteFragmentOutputs(std::string& source) {
 std::array<bool, 16> outputs{};
 if(referencesToken(source, "gl_FragColor")) {
  replaceAllToken(source, "gl_FragColor", "iris_FragData0");
  outputs[0] = true;
 }
 struct Match {
  std::size_t start;
  std::size_t length;
  int output;
 };
 const std::vector<bool> mask = codeMask(source);
 std::vector<Match> matches;
 std::size_t at = 0;
 while((at = source.find("gl_FragData", at)) != std::string::npos) {
  if(!tokenAt(source, mask, at, "gl_FragData")) {
   at += 11;
   continue;
  }
  std::size_t cursor = at + 11;
  while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
  if(cursor >= source.size() || source[cursor] != '[') {
   at += 11;
   continue;
  }
  ++cursor;
  while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
  const std::size_t digits = cursor;
  while(cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor]))) ++cursor;
  if(digits == cursor) {
   at += 11;
   continue;
  }
  int output = 0;
  for(std::size_t digit = digits; digit < cursor && output < 16; ++digit) output = output * 10 + source[digit] - '0';
  while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
  if(cursor >= source.size() || source[cursor] != ']' || output < 0 || output >= 16) {
   at += 11;
   continue;
  }
  matches.push_back({at, cursor + 1 - at, output});
  outputs[static_cast<std::size_t>(output)] = true;
  at = cursor + 1;
 }
 for(auto match = matches.rbegin(); match != matches.rend(); ++match)
  source.replace(match->start, match->length, "iris_FragData" + std::to_string(match->output));
 std::string declarations;
 for(std::size_t output = 0; output < outputs.size(); ++output) {
  if(outputs[output]) {
   declarations += "layout(location = " + std::to_string(output) + ") out vec4 iris_FragData" +
                   std::to_string(output) + ";\n";
  }
 }
 if(!declarations.empty()) source.insert(sourceDeclarationOffset(source), declarations);
 return outputs;
}
void canonicalizeTextureCalls(std::string& source) {
 constexpr std::array<std::pair<std::string_view, std::string_view>, 18> replacements = {
     std::pair<std::string_view, std::string_view>{"texture2D", "texture"},
     {"texture3D", "texture"}, {"textureCube", "texture"},
     {"texture2DLod", "textureLod"}, {"texture3DLod", "textureLod"}, {"textureCubeLod", "textureLod"},
     {"texture2DProj", "textureProj"}, {"texture3DProj", "textureProj"},
     {"texture2DGrad", "textureGrad"}, {"texture2DGradARB", "textureGrad"},
     {"texture3DGrad", "textureGrad"}, {"texture3DGradARB", "textureGrad"},
     {"textureCubeGrad", "textureGrad"}, {"textureCubeGradARB", "textureGrad"},
     {"texelFetch2D", "texelFetch"}, {"texelFetch3D", "texelFetch"},
     {"textureSize2D", "textureSize"}, {"textureSize3D", "textureSize"}};
 for(const auto& [legacy, current] : replacements) replaceFunctionCalls(source, legacy, current);
 std::string declarations;
 if(replaceFunctionCalls(source, "shadow2D", "iris_shadow2D")) {
  declarations += "vec4 iris_shadow2D(sampler2DShadow image, vec3 coordinate) { return vec4(texture(image, coordinate)); }\n";
  declarations += "vec4 iris_shadow2D(sampler2DShadow image, vec3 coordinate, float bias) { return vec4(texture(image, coordinate, bias)); }\n";
 }
 if(replaceFunctionCalls(source, "shadow2DLod", "iris_shadow2DLod")) {
  declarations += "vec4 iris_shadow2DLod(sampler2DShadow image, vec3 coordinate, float lod) { return vec4(textureLod(image, coordinate, lod)); }\n";
 }
 if(!declarations.empty()) source.insert(sourceDeclarationOffset(source), declarations);
}

std::string canonicalizeVertex(const std::string& programName,
                               const PackDefinition& pack,
                               std::string source) {
 return injectChunkFadeAttribute(programName, pack, lowerVertexSource(programName, std::move(source)));
}

std::string canonicalizeFragment(const std::string& programName, std::string source) {
 const bool legacyOutput = referencesToken(source, "gl_FragData") || referencesToken(source, "gl_FragColor");
 replaceAllToken(source, "subgroupAll(will_discard)", "will_discard");
 static const std::string targetNoAlpha = "f16vec3 color = f16vec3(texture(gtexture, v.coord).rgb);";
 replaceAllToken(source, targetNoAlpha, GlslSnippets::get("compat_alpha_check"));
 if(referencesToken(source, "gl_FogFragCoord")) {
  replaceAllToken(source, "gl_FogFragCoord", "iris_FogFragCoord");
  if(!hasStorageDeclaration(source, "in", "iris_FogFragCoord"))
   source.insert(sourceDeclarationOffset(source), GlslSnippets::get("iris_fog_frag_coord_fragment_in"));
 }
 std::string declarations;
 if(referencesToken(source, "gl_TexCoord")) {
  replaceAllToken(source, "gl_TexCoord", "irs_texCoords");
  if(!hasStorageDeclaration(source, "in", "irs_texCoords")) declarations += "in vec4 irs_texCoords[3];\n";
 }
 if(referencesToken(source, "gl_Color")) {
  replaceAllToken(source, "gl_Color", "irs_Color");
  if(!hasStorageDeclaration(source, "in", "irs_Color")) declarations += "in vec4 irs_Color;\n";
 }
 if(!declarations.empty()) source.insert(sourceDeclarationOffset(source), declarations);
 const std::array<bool, 16> outputs = rewriteFragmentOutputs(source);
 const bool hasDepthDecl = source.find("gl_FragDepth") != std::string::npos;
 const bool hasDepthWrite = source.find("gl_FragDepth =") != std::string::npos ||
                            source.find("gl_FragDepth=") != std::string::npos;
 if(hasDepthDecl && !hasDepthWrite) appendBeforeMainClose(source, GlslSnippets::get("gl_frag_depth_passthrough"));
 if(!programGetsCompatAlphaTest(programName) || !legacyOutput || !outputs[0]) return source;
 if(source.find("alphaTestRef") == std::string::npos)
  source.insert(sourceDeclarationOffset(source), "uniform float alphaTestRef;\n");
 std::string snippet = GlslSnippets::get("alpha_test_discard");
 replaceAllToken(snippet, "ALPHA_TEST_ACCESSOR", "iris_FragData0.a");
 appendBeforeMainClose(source, snippet);
 return source;
}

}

std::string canonicalizeCoreSource(const std::string& programName,
                                   ShaderStage stage,
                                   const PackDefinition& pack,
                                   std::string source,
                                   const ShaderTransformContext& context) {
 canonicalizeTextureCalls(source);
 if(stage == ShaderStage::Vertex) {
  replaceGlobalStorageQualifier(source, "attribute", "in");
  replaceGlobalStorageQualifier(source, "varying", "out");
  patchEntityInputs(source, stage, context);
  return canonicalizeVertex(programName, pack, std::move(source));
 }
 if(stage == ShaderStage::Fragment) {
  replaceGlobalStorageQualifier(source, "varying", "in");
  patchEntityInputs(source, stage, context);
  return canonicalizeFragment(programName, std::move(source));
 }
 if(stage != ShaderStage::Compute) patchEntityInputs(source, stage, context);
 return source;
}
}
