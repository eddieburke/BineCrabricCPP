#include "net/minecraft/client/render/SourceProcessor.hpp"
#include "net/minecraft/client/render/ColorWheelMerge.hpp"
#include "net/minecraft/client/render/Pack.hpp"
#include "net/minecraft/client/render/Catalog.hpp"
#include "net/minecraft/client/render/PreProcessor.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace net::minecraft::client::render::glutil {
using pack_catalog::lower;

int glVersionMacro() {
 const char* text = reinterpret_cast<const char*>(::glGetString(0x1F02));
 if(text == nullptr) return 330;
 int major = 0;
 int minor = 0;
 std::sscanf(text, "%d.%d", &major, &minor);
 return major > 0 ? major * 100 + minor * 10 : 330;
}

int maxColorBuffers() {
 static int buffers = 0;
 if(buffers == 0) {
  ::glGetIntegerv(0x8CDF, &buffers);
  buffers = std::clamp(buffers, 1, render::kMaxColorAttachments);
 }
 return buffers;
}

std::string formatVersion122(std::string_view semver) {
 // Iris StandardMacros.getFormattedIrisVersion + formatVersionString:
 // pull major.minor[.bugfix] out of any string, ignore the rest, then 122-pad.
 // https://github.com/IrisShaders/Iris/blob/37c02037/common/src/main/java/net/irisshaders/iris/gl/shader/StandardMacros.java
 // https://shaders.properties/current/reference/macros/iris_version/
 // https://shaders.properties/current/reference/macros/mc_version/
 std::size_t i = 0;
 while(i < semver.size() && (semver[i] < '0' || semver[i] > '9')) ++i;
 if(i >= semver.size()) return {};

 auto readDigits = [&](std::string& out) {
  out.clear();
  while(i < semver.size() && semver[i] >= '0' && semver[i] <= '9') out.push_back(semver[i++]);
 };

 std::string major;
 readDigits(major);
 if(major.empty() || i >= semver.size() || semver[i] != '.') return {};
 ++i;
 std::string minor;
 readDigits(minor);
 if(minor.empty()) return {};

 std::string bugFix = "0";
 if(i < semver.size() && semver[i] == '.') {
  ++i;
  std::string patch;
  readDigits(patch);
  if(!patch.empty()) bugFix = std::move(patch);
 }

 if(minor.size() == 1) minor.insert(minor.begin(), '0');
 if(bugFix.size() == 1) bugFix.insert(bugFix.begin(), '0');
 return major + minor + bugFix;
}

std::string driverPreamble() {
 // Iris StandardMacros.getVendor / getRenderer — startsWith on lowercased GL strings.
 const auto text = [](unsigned int name) {
  const char* value = reinterpret_cast<const char*>(::glGetString(name));
  return lower(value == nullptr ? std::string{} : std::string(value));
 };
 const std::string vendor = text(0x1F00);
 const std::string renderer = text(0x1F01);
 const auto starts = [](std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
 };
 std::string vendorMacro = "MC_GL_VENDOR_OTHER";
 if(starts(vendor, "ati"))
  vendorMacro = "MC_GL_VENDOR_ATI";
 else if(starts(vendor, "intel"))
  vendorMacro = "MC_GL_VENDOR_INTEL";
 else if(starts(vendor, "nvidia"))
  vendorMacro = "MC_GL_VENDOR_NVIDIA";
 else if(starts(vendor, "amd"))
  vendorMacro = "MC_GL_VENDOR_AMD";
 else if(starts(vendor, "x.org"))
  vendorMacro = "MC_GL_VENDOR_XORG";

 std::string rendererMacro = "MC_GL_RENDERER_OTHER";
 if(starts(renderer, "amd") || starts(renderer, "ati") || starts(renderer, "radeon"))
  rendererMacro = "MC_GL_RENDERER_RADEON";
 else if(starts(renderer, "gallium"))
  rendererMacro = "MC_GL_RENDERER_GALLIUM";
 else if(starts(renderer, "intel"))
  rendererMacro = "MC_GL_RENDERER_INTEL";
 else if(starts(renderer, "geforce") || starts(renderer, "nvidia"))
  rendererMacro = "MC_GL_RENDERER_GEFORCE";
 else if(starts(renderer, "quadro") || starts(renderer, "nvs"))
  rendererMacro = "MC_GL_RENDERER_QUADRO";
 else if(starts(renderer, "mesa"))
  rendererMacro = "MC_GL_RENDERER_MESA";
 else if(starts(renderer, "apple"))
  rendererMacro = "MC_GL_RENDERER_APPLE";

 return "#define " + vendorMacro + "\n#define " + rendererMacro + "\n";
}

namespace {
template <std::size_t N>
void appendIndexedDefines(std::string& result,
                          std::string_view prefix,
                          const std::array<std::string_view, N>& names) {
 for(std::size_t index = 0; index < names.size(); ++index)
  result += "#define " + std::string(prefix) + std::string(names[index]) + " " +
            std::to_string(index) + "\n";
}

}

std::size_t sourceDeclarationOffset(const std::string& source) {
 std::size_t offset = 0;
 std::istringstream stream(source);
 for(std::string line; std::getline(stream, line);) {
  const std::string trimmed = lineForDirectiveParse(line);
  const std::size_t first = trimmed.find_first_not_of(" \t\r\n");
  const std::string nonws = first == std::string::npos ? std::string{} : trimmed.substr(first);
  if(!nonws.empty() && !nonws.starts_with("#")) break;
  offset += line.size() + 1;
 }
 return offset;
}

bool appendBeforeMainClose(std::string& source, const std::string& snippet) {
 const std::vector<bool> mask = codeMask(source);
 std::size_t mainId = 0;
 while((mainId = source.find("main", mainId)) != std::string::npos) {
  if(tokenAt(source, mask, mainId, "main")) break;
  mainId += 4;
 }
 if(mainId == std::string::npos) return false;
 std::size_t openBrace = mainId + 4;
 while(openBrace < source.size() && (!mask[openBrace] || source[openBrace] != '{')) ++openBrace;
 if(openBrace == source.size()) return false;
 int depth = 1;
 for(std::size_t index = openBrace + 1; index < source.size(); ++index) {
  if(!mask[index]) continue;
  if(source[index] == '{')
   ++depth;
  else if(source[index] == '}' && --depth == 0) {
   source.insert(index, snippet);
   return true;
  }
 }
 return false;
}

std::string normalizePackSource(const std::string& source, const std::string& preamble) {
 PPMacroTable macros;
 seedMacrosFromDefines(preamble, macros);
 for(const std::string& extension : supportedGlExtensions()) {
  PPMacro flag;
  flag.body = "1";
  macros[extension] = std::move(flag);
 }
 struct CondFrame {
  bool parentActive = true;
  bool taken = false;
  bool active = true;
 };
 std::vector<CondFrame> stack;
 auto active = [&stack]() {
  return stack.empty() || stack.back().active;
 };
 std::string extensions;
 std::string body;
 std::istringstream stream(source);
 std::string physical;
 while(std::getline(stream, physical)) {
  if(!physical.empty() && physical.back() == '\r') physical.pop_back();
  std::string logical = physical;
  int continuations = 0;
  while(!logical.empty() && logical.back() == '\\') {
   logical.pop_back();
   std::string next;
   if(!std::getline(stream, next)) break;
   if(!next.empty() && next.back() == '\r') next.pop_back();
   logical += next;
   ++continuations;
  }
  auto emit = [&](std::string_view text = {}) {
   body += text;
   body.append(static_cast<std::size_t>(continuations + 1), '\n');
  };
  const std::string parsedLine = lineForDirectiveParse(logical);
  const std::size_t first = parsedLine.find_first_not_of(" \t\r\n");
  const std::string cleaned = first == std::string::npos ? std::string{} : parsedLine.substr(first);
  std::string keyword, rest;
  if(parseDirective(cleaned, keyword, rest)) {
   if(keyword == "if" || keyword == "ifdef" || keyword == "ifndef") {
    const bool parent = active();
    bool condition = false;
    if(parent) {
     if(keyword == "if")
      condition = evaluateIfExpression(rest, macros);
     else {
      std::size_t e = 0;
      while(e < rest.size() && isIdentChar(rest[e])) ++e;
      const bool defined = macros.count(rest.substr(0, e)) > 0;
      condition = keyword == "ifdef" ? defined : !defined;
     }
    }
    stack.push_back({parent, parent && condition, parent && condition});
    emit();
    continue;
   }
   if(keyword == "elif") {
    if(!stack.empty()) {
     CondFrame& frame = stack.back();
     if(frame.parentActive && !frame.taken) {
      const bool condition = evaluateIfExpression(rest, macros);
      frame.active = condition;
      frame.taken = frame.taken || condition;
     } else {
      frame.active = false;
     }
    }
    emit();
    continue;
   }
   if(keyword == "else") {
    if(!stack.empty()) {
     CondFrame& frame = stack.back();
     frame.active = frame.parentActive && !frame.taken;
     frame.taken = true;
    }
    emit();
    continue;
   }
   if(keyword == "endif") {
    if(!stack.empty()) stack.pop_back();
    emit();
    continue;
   }
   if(!active()) {
    emit();
    continue;
   }
   if(keyword == "define") {
    parseDefineDirective(rest, macros);
    emit(logical);
    continue;
   }
   if(keyword == "undef") {
    std::size_t e = 0;
    while(e < rest.size() && isIdentChar(rest[e])) ++e;
    macros.erase(rest.substr(0, e));
    emit(logical);
    continue;
   }
   if(keyword == "version") {
    emit();
    continue;
   }
   if(keyword == "extension") {
    extensions += "#extension ";
    extensions += rest;
    extensions += '\n';
    emit();
    continue;
   }
   if(keyword == "include" || keyword == "warning" || keyword == "custom" || keyword == "moj_import") {
    emit();
    continue;
   }
   emit(logical);
   continue;
  }
  if(active())
   emit(logical);
  else
   emit();
 }
 return extensions + body;
}

bool isCompositeStyleProgramName(const std::string& programName) {
 static constexpr std::array<std::string_view, 6> prefixes = {
     "begin", "shadowcomp", "prepare", "deferred", "composite", "final"};
 const std::string_view name = programName;
 for(const std::string_view prefix : prefixes) {
  if(!name.starts_with(prefix)) continue;
  if(name.size() == prefix.size()) return true;
  const char next = name[prefix.size()];
  if(next == '_' || (next >= '0' && next <= '9')) return true;
 }
 return false;
}

std::vector<bool> codeMask(const std::string& source) {
 std::vector<bool> mask(source.size(), true);
 bool lineComment = false;
 bool blockComment = false;
 bool quoted = false;
 char quote = '\0';
 for(std::size_t index = 0; index < source.size(); ++index) {
  const char ch = source[index];
  const char next = index + 1 < source.size() ? source[index + 1] : '\0';
  if(lineComment) {
   mask[index] = false;
   if(ch == '\n') lineComment = false;
   continue;
  }
  if(blockComment) {
   mask[index] = false;
   if(ch == '*' && next == '/') {
    mask[index + 1] = false;
    blockComment = false;
    ++index;
   }
   continue;
  }
  if(quoted) {
   mask[index] = false;
   if(ch == '\\' && next != '\0') {
    mask[index + 1] = false;
    ++index;
   } else if(ch == quote) {
    quoted = false;
   }
   continue;
  }
  if(ch == '/' && next == '/') {
   mask[index] = false;
   mask[index + 1] = false;
   lineComment = true;
   ++index;
  } else if(ch == '/' && next == '*') {
   mask[index] = false;
   mask[index + 1] = false;
   blockComment = true;
   ++index;
  } else if(ch == '"' || ch == '\'') {
   mask[index] = false;
   quoted = true;
   quote = ch;
  }
 }
 return mask;
}

bool tokenAt(const std::string& source,
             const std::vector<bool>& mask,
             std::size_t at,
             std::string_view token) {
 const std::size_t end = at + token.size();
 const bool left = at == 0 || !isIdentChar(source[at - 1]);
 const bool right = end >= source.size() || !isIdentChar(source[end]);
 return left && right && end <= mask.size() &&
        std::all_of(mask.begin() + static_cast<std::ptrdiff_t>(at),
                    mask.begin() + static_cast<std::ptrdiff_t>(end),
                    [](bool value) { return value; });
}

void replaceAllToken(std::string& source, std::string_view from, std::string_view to) {
 if(from.empty()) return;
 const std::vector<bool> mask = codeMask(source);
 std::vector<std::size_t> matches;
 std::size_t at = 0;
 while((at = source.find(from, at)) != std::string::npos) {
  if(tokenAt(source, mask, at, from)) matches.push_back(at);
  at += from.size();
 }
 for(auto it = matches.rbegin(); it != matches.rend(); ++it) source.replace(*it, from.size(), to);
}

bool referencesToken(const std::string& source, std::string_view token) {
 if(token.empty()) return false;
 const std::vector<bool> mask = codeMask(source);
 std::size_t at = 0;
 while((at = source.find(token, at)) != std::string::npos) {
  if(tokenAt(source, mask, at, token)) return true;
  at += token.size();
 }
 return false;
}

bool hasStorageDeclaration(const std::string& source,
                           std::string_view storage,
                           std::string_view alternateStorage,
                           std::string_view name) {
 const std::vector<bool> mask = codeMask(source);
 int braceDepth = 0;
 int parenDepth = 0;
 bool declaration = false;
 for(std::size_t index = 0; index < source.size();) {
  if(!mask[index]) {
   ++index;
   continue;
  }
  const char ch = source[index];
  if(isIdentStart(ch)) {
   const std::size_t start = index++;
   while(index < source.size() && mask[index] && isIdentChar(source[index])) ++index;
   const std::string_view token(source.data() + start, index - start);
   if(!declaration && braceDepth == 0 && parenDepth == 0 &&
      (token == storage || (!alternateStorage.empty() && token == alternateStorage))) {
    declaration = true;
   } else if(declaration && braceDepth == 0 && parenDepth == 0 && token == name) {
    return true;
   }
   continue;
  }
  if(ch == '{') {
   ++braceDepth;
  } else if(ch == '}') {
   braceDepth = std::max(0, braceDepth - 1);
  } else if(ch == '(') {
   ++parenDepth;
  } else if(ch == ')') {
   parenDepth = std::max(0, parenDepth - 1);
  } else if(ch == ';' && braceDepth == 0 && parenDepth == 0) {
   declaration = false;
  }
  ++index;
 }
 return false;
}

static std::string injectChunkFadeAttribute(const std::string& programName,
                                            const PackDefinition& pack,
                                            std::string source) {
 const bool enabled =
     pack.optionalFeatures.contains("FADE_VARIABLE") ||
     pack.requiredFeatures.contains("FADE_VARIABLE");
 const bool declared = hasStorageDeclaration(source, "in", "attribute", "mc_chunkFade") ||
                       hasStorageDeclaration(source, "uniform", {}, "mc_chunkFade") ||
                       hasStorageDeclaration(source, "const", {}, "mc_chunkFade");
 if(!enabled || declared) return source;
 const std::string_view name = programName;
 if(!name.starts_with("gbuffers_")) return source;
 const bool terrain = name.starts_with("gbuffers_terrain");
 source.insert(sourceDeclarationOffset(source),
               terrain ? "in float mc_chunkFade;\n"
                       : "const float mc_chunkFade = -1.0;\n");
 return source;
}

struct SourceDeclaration {
 std::string_view storage;
 std::string_view alternateStorage;
 std::string_view type;
 std::string_view name;
};

template <std::size_t N>
void appendMissingDeclarations(std::string& output,
                               const std::string& source,
                               const std::array<SourceDeclaration, N>& declarations) {
 for(const SourceDeclaration& declaration : declarations) {
  if(referencesToken(source, declaration.name) &&
     !hasStorageDeclaration(source, declaration.storage, declaration.alternateStorage,
                            declaration.name)) {
   output += std::string(declaration.storage) + " " + std::string(declaration.type) + " " +
             std::string(declaration.name) + ";\n";
  }
 }
}

static constexpr std::array kCompositeUniforms = {
    SourceDeclaration{"uniform", {}, "mat4", "modelViewMatrix"},
    SourceDeclaration{"uniform", {}, "mat4", "projectionMatrix"},
    SourceDeclaration{"uniform", {}, "mat4", "modelViewProjectionMatrix"},
    SourceDeclaration{"uniform", {}, "mat4", "textureMatrix"}};
static constexpr std::array kGbufferUniforms = {
    SourceDeclaration{"uniform", {}, "mat4", "modelViewMatrixInverse"},
    SourceDeclaration{"uniform", {}, "mat4", "projectionMatrixInverse"},
    SourceDeclaration{"uniform", {}, "mat3", "normalMatrix"},
    SourceDeclaration{"uniform", {}, "vec3", "chunkOffset"}};
static constexpr std::array kVertexAttributes = {
    SourceDeclaration{"in", "attribute", "vec3", "vaPosition"},
    SourceDeclaration{"in", "attribute", "vec2", "vaUV0"},
    SourceDeclaration{"in", "attribute", "vec2", "vaUV2"},
    SourceDeclaration{"in", "attribute", "vec4", "vaColor"},
    SourceDeclaration{"in", "attribute", "vec3", "vaNormal"}};

const char* defaultCompositeVertexShader() {
 return "in vec3 vaPosition;\n"
        "in vec2 vaUV0;\n"
        "uniform mat4 modelViewMatrix;\n"
        "uniform mat4 projectionMatrix;\n"
        "out vec2 texcoord;\n"
        "void main() {\n"
        " gl_Position = projectionMatrix * modelViewMatrix * vec4(vaPosition, 1.0);\n"
        " texcoord = vaUV0;\n"
        "}\n";
}

namespace {
bool isGbufferOrShadowProgramName(const std::string& programName) {
 const std::string_view name = programName;
 return name.starts_with("gbuffers_") || name.starts_with("clrwl_gbuffers") || name == "shadow" ||
        name.starts_with("shadow_") || name.starts_with("clrwl_shadow");
}

constexpr const char* kIrisLightmapTextureMatrixDecl =
    "const mat4 iris_lightmapTextureMatrix = mat4("
    "vec4(0.00390625, 0.0, 0.0, 0.0), "
    "vec4(0.0, 0.00390625, 0.0, 0.0), "
    "vec4(0.0, 0.0, 0.00390625, 0.0), "
    "vec4(0.0, 0.0, 0.0, 1.0));\n";

std::string lowerVertexSource(const std::string& programName, std::string vertexSource) {
 const bool composite = isCompositeStyleProgramName(programName);
 const bool gbufferOrShadow = isGbufferOrShadowProgramName(programName);
 if(!composite && !gbufferOrShadow) {
  return vertexSource;
 }
 const bool hasLegacy = vertexSource.find("ftransform") != std::string::npos ||
                        vertexSource.find("gl_Vertex") != std::string::npos ||
                        vertexSource.find("gl_Color") != std::string::npos ||
                        vertexSource.find("gl_Normal") != std::string::npos ||
                        vertexSource.find("gl_MultiTexCoord") != std::string::npos ||
                        vertexSource.find("gl_ModelView") != std::string::npos ||
                        vertexSource.find("gl_Projection") != std::string::npos ||
                        vertexSource.find("gl_TextureMatrix") != std::string::npos;
 if(hasLegacy) {
  const std::string position = composite ? "vaPosition" : "vaPosition + chunkOffset";
  replaceAllToken(vertexSource, "ftransform()",
                  "(projectionMatrix * modelViewMatrix * vec4(" + position + ", 1.0))");
  replaceAllToken(vertexSource, "gl_Vertex", "vec4(" + position + ", 1.0)");
  static constexpr std::array replacements = {
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
  for(const auto& [legacy, current] : replacements)
   replaceAllToken(vertexSource, legacy, current);
 }
 std::string decls;
 appendMissingDeclarations(decls, vertexSource, kVertexAttributes);
 appendMissingDeclarations(decls, vertexSource, kCompositeUniforms);
 if(gbufferOrShadow) appendMissingDeclarations(decls, vertexSource, kGbufferUniforms);
 if(referencesToken(vertexSource, "iris_lightmapTextureMatrix") &&
    vertexSource.find("const mat4 iris_lightmapTextureMatrix") == std::string::npos) {
  decls += kIrisLightmapTextureMatrixDecl;
 }
 if(decls.empty()) return vertexSource;
 vertexSource.insert(sourceDeclarationOffset(vertexSource), decls);
 return vertexSource;
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

bool fragmentWritesLegacyFragOutput(const std::string& source) {
 return referencesToken(source, "gl_FragData") || referencesToken(source, "gl_FragColor");
}

std::string lowerFragmentSource(const std::string& programName, std::string fragmentSource) {
 replaceAllToken(fragmentSource, "subgroupAll(will_discard)", "will_discard");
 static const std::string targetNoAlpha = "f16vec3 color = f16vec3(texture(gtexture, v.coord).rgb);";
 static const std::string replacementAlphaCheck =
     "f16vec4 tex_sample_rgba = texture(gtexture, v.coord);\n\t\t\tif (tex_sample_rgba.a < float16_t(0.1)) { discard; }\n\t\t\tf16vec3 color = f16vec3(tex_sample_rgba.rgb);";
 replaceAllToken(fragmentSource, targetNoAlpha, replacementAlphaCheck);
 const bool hasDepthDecl = fragmentSource.find("gl_FragDepth") != std::string::npos;
 const bool hasDepthWrite = fragmentSource.find("gl_FragDepth =") != std::string::npos ||
                            fragmentSource.find("gl_FragDepth=") != std::string::npos;
 if(hasDepthDecl && !hasDepthWrite) {
  appendBeforeMainClose(fragmentSource, "\tgl_FragDepth = gl_FragCoord.z;\n");
 }
 if(!programGetsCompatAlphaTest(programName) || !fragmentWritesLegacyFragOutput(fragmentSource)) {
  return fragmentSource;
 }
 if(fragmentSource.find("alphaTestRef") == std::string::npos) {
  fragmentSource.insert(sourceDeclarationOffset(fragmentSource), "uniform float alphaTestRef;\n");
 }
 const char* accessor =
     referencesToken(fragmentSource, "gl_FragData") ? "gl_FragData[0].a" : "gl_FragColor.a";
 const std::string snippet =
     std::string("\tif (!(") + accessor + " > alphaTestRef)) {\n\t\tdiscard;\n\t}\n";
 if(!appendBeforeMainClose(fragmentSource, snippet)) {
  return fragmentSource;
 }
 return fragmentSource;
}
}

namespace {
const char* hostOsMacro() {
#if defined(_WIN32)
 return "MC_OS_WINDOWS";
#elif defined(__APPLE__)
 return "MC_OS_MAC";
#elif defined(__linux__)
 return "MC_OS_LINUX";
#else
 return "MC_OS_UNKNOWN";
#endif
}
}

std::string versionPreamble(const PackDefinition& pack, const std::string& source, bool compute) {
 int version = compute ? 430 : 120;
 std::string profile;
 if(const std::size_t marker = source.find("#version"); marker != std::string::npos) {
  const std::size_t end = source.find('\n', marker);
  std::istringstream directive(source.substr(marker + 8, end == std::string::npos ? end : end - marker - 8));
  directive >> version >> profile;
 }
 if(compute) version = std::max(version, 430);
 if(profile == "compatibility") {
  profile = "core";
 }
 if(profile != "core") {
  profile.clear();
 }
 // Iris StandardMacros.createStandardEnvironmentDefines — IS_IRIS / IRIS_VERSION / MC_VERSION.
 // https://github.com/IrisShaders/Iris/blob/37c02037/common/src/main/java/net/irisshaders/iris/gl/shader/StandardMacros.java
 // https://shaders.properties/current/reference/macros/is_iris/
 // https://shaders.properties/current/reference/macros/iris_version/
 // https://shaders.properties/current/reference/macros/mc_version/
 // Versions are arbitrary semvers run through formatVersion122 (Iris 122 encode); no game-specific layout.
 constexpr const char* kMcSemver = "1.7.3";
 constexpr const char* kIrisApiSemver = "1.9.2";
 const std::string mcVersion = formatVersion122(kMcSemver);
 const std::string irisVersion = formatVersion122(kIrisApiSemver);
 std::string result = "#version " + std::to_string(version) + (profile.empty() ? "\n" : " " + profile + "\n");
 result += "#define MC_VERSION " + mcVersion + "\n";
 result += "#define MC_GL_VERSION " + std::to_string(glVersionMacro()) + "\n";
 result += "#define MC_GLSL_VERSION " + std::to_string(version) + "\n";
 result += "#define IS_IRIS\n";
 result += "#define IRIS_VERSION " + irisVersion + "\n";
 result += "#define MAX_COLOR_BUFFERS " + std::to_string(maxColorBuffers()) + "\n";
 result += "#define " + std::string(hostOsMacro()) + "\n";
 result += "#define MC_HAND_DEPTH 0.125\n";
 result += "#define MC_MIPMAP_LEVEL " + std::to_string(std::max(0, pack.mcMipmapLevel)) + "\n";
 static constexpr std::array<std::string_view, 3> kPrecipitation = {"NONE", "RAIN", "SNOW"};
 static constexpr std::array<std::string_view, 17> kCategories = {
     "NONE", "TAIGA", "EXTREME_HILLS", "JUNGLE", "MESA", "PLAINS", "SAVANNA",
     "ICY", "THE_END", "BEACH", "FOREST", "OCEAN", "DESERT", "RIVER", "SWAMP",
     "MUSHROOM", "NETHER"};
 static constexpr std::array<std::string_view, 13> kBiomes = {
     "RAINFOREST", "SWAMP", "SEASONAL_FOREST", "FOREST", "SAVANNA", "SHRUBLAND",
     "TAIGA", "DESERT", "PLAINS", "ICE_DESERT", "TUNDRA", "NETHER_WASTES", "THE_END"};
 appendIndexedDefines(result, "PPT_", kPrecipitation);
 appendIndexedDefines(result, "CAT_", kCategories);
 appendIndexedDefines(result, "BIOME_", kBiomes);
 result += driverPreamble();
 if(pack.labPbr || pack.labPbr13) result += "#define MC_TEXTURE_FORMAT_LAB_PBR\n";
 if(pack.labPbr13) result += "#define MC_TEXTURE_FORMAT_LAB_PBR_1_3\n";
 if(pack.labPbr || pack.labPbr13) {
  result += "#define MC_NORMAL_MAP\n#define MC_SPECULAR_MAP\n";
 }
 static constexpr std::array<std::string_view, 24> kRenderStages = {
     "NONE",          "SKY",                 "SUNSET",         "CUSTOM_SKY",
     "SUN",           "MOON",                "STARS",          "VOID",
     "TERRAIN_SOLID", "TERRAIN_CUTOUT_MIPPED", "TERRAIN_CUTOUT", "ENTITIES",
     "BLOCK_ENTITIES", "DESTROY",             "OUTLINE",        "DEBUG",
     "HAND_SOLID",    "TERRAIN_TRANSLUCENT", "TRIPWIRE",       "PARTICLES",
     "CLOUDS",        "RAIN_SNOW",           "WORLD_BORDER",   "HAND_TRANSLUCENT"};
 static_assert(static_cast<int>(core::RenderStage::HandTranslucent) + 1 ==
               static_cast<int>(kRenderStages.size()));
 appendIndexedDefines(result, "MC_RENDER_STAGE_", kRenderStages);
 for(const std::string& feature : pack.requiredFeatures)
  if(featureSupported(feature)) result += "#define IRIS_FEATURE_" + feature + "\n";
 for(const std::string& feature : pack.optionalFeatures)
  if(featureSupported(feature)) result += "#define IRIS_FEATURE_" + feature + "\n";
 for(const std::string& extension : supportedGlExtensions()) result += "#define MC_" + extension + "\n";
 if(pack.colorWheel.present) appendColorWheelMacros(result);
 return result;
}

std::string prepareSource(const std::string& programName,
                          ShaderStage stage,
                          const PackDefinition& pack,
                          const std::string& source,
                          const std::string& preamble) {
 std::string prepared = normalizePackSource(source, preamble);
 if(stage == ShaderStage::Vertex) {
  prepared = injectChunkFadeAttribute(programName, pack,
                                      lowerVertexSource(programName, std::move(prepared)));
 } else if(stage == ShaderStage::Fragment) {
  prepared = lowerFragmentSource(programName, std::move(prepared));
 }
 return mergeColorWheelMaterial(programName, stage, std::move(prepared));
}
}
