#include "net/minecraft/client/render/shaders/CoreGlslTransformer.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <utility>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/EntityPatcher.hpp"
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include "net/minecraft/client/render/shaders/GlslSource.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
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
                               const CodeMask& mask,
                               const std::array<SourceDeclaration, N>& declarations) {
 for(const SourceDeclaration& declaration : declarations) {
  if(referencesToken(source, mask, declaration.name) &&
     !hasStorageDeclaration(source, mask, declaration.storage, declaration.name) &&
     !hasStorageDeclaration(source, mask, "in", declaration.name) &&
     !hasStorageDeclaration(source, mask, "out", declaration.name)) {
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
bool patchMultiTexCoord3(std::string& source, std::string& declarations, const CodeMask& mask) {
 if(!referencesToken(source, mask, "gl_MultiTexCoord3") || referencesToken(source, mask, "mc_midTexCoord") ||
    hasStorageDeclaration(source, mask, "in", "mc_midTexCoord") ||
    hasStorageDeclaration(source, mask, "uniform", "mc_midTexCoord") ||
    hasStorageDeclaration(source, mask, "const", "mc_midTexCoord")) return false;
 replaceAllToken(source, "gl_MultiTexCoord3", "mc_midTexCoord");
 declarations += "in vec4 mc_midTexCoord;\n";
 return true;
}
void replaceGlMultiTexCoordBounded(std::string& source, int minimum, int maximum) {
 for(int index = minimum; index <= maximum; ++index)
  replaceAllToken(source, "gl_MultiTexCoord" + std::to_string(index), "vec4(0.0, 0.0, 0.0, 1.0)");
}
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/SodiumTransformer.java
[[nodiscard]] bool isChunkMesherProgram(std::string_view programName) {
 return programName.starts_with("gbuffers_terrain") || programName == "gbuffers_water" ||
        programName.starts_with("clrwl_gbuffers");
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
 // SodiumTransformer.java:124 (`parameters.shadow` branch).
 if(!enabled || declared || !(name.starts_with("gbuffers_") || name.starts_with("clrwl_gbuffers") || shadow))
  return source;
 source.insert(sourceDeclarationOffset(source),
               (!shadow && isChunkMesherProgram(name)) ? GlslSnippets::get("chunk_fade_terrain_in")
                                                       : GlslSnippets::get("chunk_fade_other_const"));
 return source;
}
std::string lowerVertexSource(const std::string& programName, std::string source) {
 const bool gbufferOrShadow = isGbufferOrShadowProgramName(programName);
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
 std::string declarations; const CodeMask mask = codeMask(source);
 appendMissingDeclarations(declarations, source, mask, kVertexAttributes);
 appendMissingDeclarations(declarations, source, mask, kCompositeUniforms);
 if(gbufferOrShadow) appendMissingDeclarations(declarations, source, mask, kGbufferUniforms);
 patchMultiTexCoord3(source, declarations, mask);
 if(referencesToken(source, "iris_lightmapTextureMatrix") &&
    !hasStorageDeclaration(source, "const", "iris_lightmapTextureMatrix")) {
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
 const CodeMask mask = codeMask(source);
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
std::string_view fragmentOutputType(const PackDefinition& pack,
                                    const std::vector<int>& drawBuffers,
                                    std::size_t output) {
 if(output >= drawBuffers.size()) {
  return "vec4";
 }
 const auto target = pack.targets.find("colortex" + std::to_string(drawBuffers[output]));
 if(target == pack.targets.end()) {
  return "vec4";
 } const ColorFormat format = parseFormat(target->second.format);
 if(isSignedIntegerColorFormat(format)) {
  return "ivec4";
 }
 return isIntegerColorFormat(format) ? "uvec4" : "vec4";
}
std::array<bool, 16> rewriteFragmentOutputs(std::string& source, const PackDefinition& pack) {
 const std::vector<int> drawBuffers = [&] {
  std::vector<int> targets = parseRenderTargetIndices(source);
  return targets.empty() ? defaultRenderTargetIndices() : targets;
 }();
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
 const CodeMask mask = codeMask(source);
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
   declarations += "layout(location = " + std::to_string(output) + ") out " +
                   std::string(fragmentOutputType(pack, drawBuffers, output)) + " iris_FragData" +
                   std::to_string(output) + ";\n";
  }
 }
 if(!declarations.empty()) source.insert(sourceDeclarationOffset(source), declarations);
 return outputs;
}
void canonicalizeTextureCalls(std::string& source, ShaderStage stage) {
 constexpr std::array<std::pair<std::string_view, std::string_view>, 18> replacements = {
     std::pair<std::string_view, std::string_view>{"texture2D", "texture"},
     {"texture3D", "texture"},
     {"textureCube", "texture"},
     {"texture2DLod", "textureLod"},
     {"texture3DLod", "textureLod"},
     {"textureCubeLod", "textureLod"},
     {"texture2DProj", "textureProj"},
     {"texture3DProj", "textureProj"},
     {"texture2DGrad", "textureGrad"},
     {"texture2DGradARB", "textureGrad"},
     {"texture3DGrad", "textureGrad"},
     {"texture3DGradARB", "textureGrad"},
     {"textureCubeGrad", "textureGrad"},
     {"textureCubeGradARB", "textureGrad"},
     {"texelFetch2D", "texelFetch"},
     {"texelFetch3D", "texelFetch"},
     {"textureSize2D", "textureSize"},
     {"textureSize3D", "textureSize"}};
  for(const auto& [legacy, current] : replacements) replaceFunctionCalls(source, legacy, current);
  std::string declarations;
  if(replaceFunctionCalls(source, "shadow2D", "iris_shadow2D")) {
   declarations += "vec4 iris_shadow2D(sampler2DShadow image, vec3 coordinate) { return vec4(texture(image, coordinate)); }\n";
   const std::string biasBody = stage == ShaderStage::Fragment ? "texture(image, coordinate, bias)" : "texture(image, coordinate)";
   declarations += "vec4 iris_shadow2D(sampler2DShadow image, vec3 coordinate, float bias) { return vec4(" + biasBody + "); }\n";
  }
 if(replaceFunctionCalls(source, "shadow2DLod", "iris_shadow2DLod")) {
  declarations += "vec4 iris_shadow2DLod(sampler2DShadow image, vec3 coordinate, float lod) { return vec4(textureLod(image, coordinate, lod)); }\n";
 }
  if(!declarations.empty()) source.insert(sourceDeclarationOffset(source), declarations);
}
namespace {
bool bareIntLiteral(std::string_view text) {
 std::size_t index = 0;
 if(index < text.size() && (text[index] == '-' || text[index] == '+')) ++index;
 const std::size_t digits = index;
 while(index < text.size() && std::isdigit(static_cast<unsigned char>(text[index]))) ++index;
 if(index == digits) return false;
 if(index < text.size() && (text[index] == 'u' || text[index] == 'U')) return false;
 return index == text.size();
}
bool bareIdentifier(std::string_view text) {
 if(text.empty()) return false;
 const char first = text[0];
 if(!(std::isalpha(static_cast<unsigned char>(first)) || first == '_')) return false;
 for(std::size_t index = 1; index < text.size(); ++index) {
  const char ch = text[index];
  if(!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) return false;
 }
 return true;
}
bool floatTypedExpression(std::string_view text) {
 for(std::size_t index = 0; index < text.size(); ++index) {
  const char ch = text[index];
  if(std::isdigit(static_cast<unsigned char>(ch))) {
   if(index + 1 < text.size() && (text[index + 1] == '.' || text[index + 1] == 'f' || text[index + 1] == 'F'))
    return true;
   if(index + 2 < text.size() && (text[index + 1] == 'e' || text[index + 1] == 'E') &&
      (std::isdigit(static_cast<unsigned char>(text[index + 2])) ||
       ((text[index + 2] == '-' || text[index + 2] == '+') && index + 3 < text.size() &&
        std::isdigit(static_cast<unsigned char>(text[index + 3])))))
    return true;
   continue;
  }
  if(ch == '.' && index + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[index + 1])))
   return true;
 }
 return false;
}
bool floatLikeExpression(std::string_view text) {
 if(bareIntLiteral(text) || bareIdentifier(text)) return false;
 const auto contains = [&](std::string_view token) {
  return text.find(token) != std::string_view::npos;
 };
 if(contains("ivec") || contains("uvec") || contains("bvec") || contains("int(") || contains("uint("))
  return false;
 if(floatTypedExpression(text)) return true;
 if(!text.empty() && (std::isdigit(static_cast<unsigned char>(text[0])) || text[0] == '.')) return true;
 if(contains("min(") || contains("max(") || contains("clamp(") || contains("pow(") || contains("texture") ||
    contains("vec") || contains("mat") || contains("float"))
  return true;
 return false;
}
std::vector<std::string_view> splitTopLevelArguments(std::string_view body) {
 std::vector<std::string_view> args;
 std::size_t depth = 0;
 std::size_t start = 0;
 for(std::size_t index = 0; index < body.size(); ++index) {
  const char ch = body[index];
  if(ch == '(') ++depth;
  else if(ch == ')') depth = std::max<std::size_t>(0, depth - 1);
  else if(ch == ',' && depth == 0) {
   args.push_back(body.substr(start, index - start));
   start = index + 1;
  }
 }
 args.push_back(body.substr(start));
 return args;
}
std::string_view trimmedArgument(std::string_view text) {
 std::size_t first = text.find_first_not_of(" \t\r\n");
 if(first == std::string_view::npos) return {};
 std::size_t last = text.find_last_not_of(" \t\r\n");
 text = text.substr(first, last - first + 1);
 while(text.size() > 1 && text.back() == ')') text.remove_suffix(1);
 first = text.find_first_not_of(" \t\r\n");
 if(first == std::string_view::npos) return {};
 last = text.find_last_not_of(" \t\r\n");
 return text.substr(first, last - first + 1);
}
} // namespace
void canonicalizeBuiltinBounds(std::string& source) {
 const CodeMask mask = codeMask(source);
 constexpr std::array<std::string_view, 4> names = {"clamp", "min", "max", "pow"};
 std::vector<std::pair<std::size_t, std::size_t>> rewrites;
 for(const std::string_view name : names) {
  std::size_t at = 0;
  while((at = source.find(name, at)) != std::string::npos) {
   if(tokenAt(source, mask, at, name)) {
    std::size_t cursor = at + name.size();
    while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
    if(cursor < source.size() && mask[cursor] && source[cursor] == '(') {
     std::size_t open = cursor;
     int depth = 1;
     for(++cursor; cursor < source.size() && depth > 0; ++cursor) {
      if(!mask[cursor]) continue;
      if(source[cursor] == '(') ++depth;
      else if(source[cursor] == ')') --depth;
     }
     if(depth == 0 && cursor <= source.size() && cursor - open >= 2) {
      const std::string_view body(source.data() + open + 1, cursor - open - 2);
      std::vector<std::string_view> args = splitTopLevelArguments(body);
       const std::vector<std::string_view> trimmed = [&] {
        std::vector<std::string_view> result;
        result.reserve(args.size());
        for(const std::string_view arg : args) result.push_back(trimmedArgument(arg));
        return result;
       }();
       std::vector<std::size_t> targets;
       if(name == "clamp" && args.size() >= 3) {
        const bool firstFloat = floatTypedExpression(trimmed[0]);
        const bool secondInt = bareIntLiteral(trimmed[1]);
        const bool thirdInt = bareIntLiteral(trimmed[2]);
        const bool secondFloat = floatTypedExpression(trimmed[1]);
        const bool thirdFloat = floatTypedExpression(trimmed[2]);
        if(secondInt && thirdFloat) targets.push_back(1);
        if(thirdInt && secondFloat) targets.push_back(2);
        if(firstFloat && secondInt && thirdInt) {
         targets.push_back(1);
         targets.push_back(2);
        }
        if(secondInt && floatLikeExpression(trimmed[0]) && floatLikeExpression(trimmed[2])) targets.push_back(1);
        if(thirdInt && floatLikeExpression(trimmed[0]) && floatLikeExpression(trimmed[1])) targets.push_back(2);
       } else if(name != "clamp" && args.size() >= 2) {
        const bool firstInt = bareIntLiteral(trimmed[0]);
        const bool secondInt = bareIntLiteral(trimmed[1]);
        if(firstInt && floatLikeExpression(trimmed[1])) targets.push_back(0);
        if(secondInt && floatLikeExpression(trimmed[0])) targets.push_back(1);
       }
       for(const std::size_t target : targets) {
        const std::string_view arg = trimmed[target];
        const std::size_t argStart = arg.data() - source.data();
        rewrites.push_back({argStart, argStart + arg.size()});
       }
      }
    }
   }
   at += name.size();
  }
 }
 std::sort(rewrites.begin(), rewrites.end());
 rewrites.erase(std::unique(rewrites.begin(), rewrites.end()), rewrites.end());
 for(auto it = rewrites.rbegin(); it != rewrites.rend(); ++it) {
  const std::string_view arg(source.data() + it->first, it->second - it->first);
  source.replace(it->first, it->second - it->first, std::string(arg) + ".0");
 }
}
void shimLegacyFogGlobals(std::string& source) {
 std::string declarations;
 bool replaced = false;
 constexpr std::array<std::pair<std::string_view, std::string_view>, 4> replacements = {
     std::pair<std::string_view, std::string_view>{"gl_Fog.color", "vec4(fogColor, 1.0)"},
     {"gl_Fog.start", "fogStart"},
     {"gl_Fog.end", "fogEnd"},
     {"gl_Fog.scale", "iris_fogScale()"}};
 for(const auto& [legacy, current] : replacements) {
  if(referencesToken(source, legacy)) {
   replaceAllToken(source, legacy, current);
   replaced = true;
  }
 }
 if(!replaced) return;
 if(!hasStorageDeclaration(source, "uniform", "fogColor")) declarations += "uniform vec3 fogColor;\n";
 if(!hasStorageDeclaration(source, "uniform", "fogStart")) declarations += "uniform float fogStart;\n";
 if(!hasStorageDeclaration(source, "uniform", "fogEnd")) declarations += "uniform float fogEnd;\n";
 declarations += "float iris_fogScale() { return 1.0 / max(fogEnd - fogStart, 0.001); }\n";
 source.insert(sourceDeclarationOffset(source), declarations);
}
std::string canonicalizeVertex(const std::string& programName,
                               const PackDefinition& pack,
                               std::string source) {
 return injectChunkFadeAttribute(programName, pack, lowerVertexSource(programName, std::move(source)));
}
std::string canonicalizeFragment(const std::string& programName, const PackDefinition& pack, std::string source) {
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
 const std::array<bool, 16> outputs = rewriteFragmentOutputs(source, pack);
 const CodeMask mask = codeMask(source);
 const bool hasDepthDecl = referencesToken(source, mask, "gl_FragDepth");
 const bool hasDepthWrite = [&] {
  std::size_t at = 0;
  while((at = source.find("gl_FragDepth", at)) != std::string::npos) {
   if(tokenAt(source, mask, at, "gl_FragDepth")) {
    std::size_t cursor = at + 12;
    while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
    if(cursor < source.size() && mask[cursor] && source[cursor] == '=') return true;
   }
   at += 12;
  }
  return false;
 }();
 if(hasDepthDecl && !hasDepthWrite) appendBeforeMainClose(source, GlslSnippets::get("gl_frag_depth_passthrough"));
 if(!programGetsCompatAlphaTest(programName) || !legacyOutput || !outputs[0]) return source;
 if(source.find("alphaTestRef") == std::string::npos)
  source.insert(sourceDeclarationOffset(source), "uniform float alphaTestRef;\n");
 std::string snippet = GlslSnippets::get("alpha_test_discard");
 replaceAllToken(snippet, "ALPHA_TEST_ACCESSOR", "iris_FragData0.a");
 appendBeforeMainClose(source, snippet);
 return source;
}
} // namespace
std::string canonicalizeCoreSource(const std::string& programName,
                                   ShaderStage stage,
                                   const PackDefinition& pack,
                                   std::string source,
                                   const ShaderTransformContext& context) {
 canonicalizeTextureCalls(source, stage);
 shimLegacyFogGlobals(source);
 canonicalizeBuiltinBounds(source);
 if(stage == ShaderStage::Vertex) {
  replaceGlobalStorageQualifier(source, "attribute", "in");
  replaceGlobalStorageQualifier(source, "varying", "out");
  patchEntityInputs(source, stage, context);
  return canonicalizeVertex(programName, pack, std::move(source));
 }
 if(stage == ShaderStage::Fragment) {
  replaceGlobalStorageQualifier(source, "varying", "in");
  patchEntityInputs(source, stage, context);
  return canonicalizeFragment(programName, pack, std::move(source));
 }
 if(stage != ShaderStage::Compute) patchEntityInputs(source, stage, context);
 return source;
}
} // namespace net::minecraft::client::render
