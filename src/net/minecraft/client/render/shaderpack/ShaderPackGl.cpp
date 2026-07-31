#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCatalog.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
namespace net::minecraft::client::render::shaderpack::glutil {
using ShaderPackCatalog::lower;
unsigned int samplerObject(bool compare) {
 static unsigned int samplers[2]{};
 if(!gl::GLCore::samplerObjectsSupported) return 0;
 const int index = compare ? 1 : 0;
 if(samplers[index] == 0) {
  gl::GLCore::genSamplers(1, &samplers[index]);
  gl::GLCore::samplerParameteri(samplers[index], 0x2801, compare ? 0x2601 : 0x2600);
  gl::GLCore::samplerParameteri(samplers[index], 0x2800, compare ? 0x2601 : 0x2600);
  gl::GLCore::samplerParameteri(samplers[index], 0x2802, 0x812F);
  gl::GLCore::samplerParameteri(samplers[index], 0x2803, 0x812F);
  gl::GLCore::samplerParameteri(samplers[index], 0x884C, compare ? 0x884E : 0);
  if(compare) gl::GLCore::samplerParameteri(samplers[index], 0x884D, 0x0203);
 }
 return samplers[index];
}
static int g_highestSamplerUnit = -1;
void bindSamplers(gl::ShaderProgram& program,
                  const std::unordered_map<std::string, int>& textures,
                  const std::unordered_map<std::string, int>& volumeTextures,
                  int maxUnits) {
 int unit = 0;
 for(const std::string& name : program.declaredSamplers()) {
  if(unit >= maxUnits) break;
  if(program.location(name) < 0) continue;
  const auto kind = program.samplerKind(name);
  const bool volume = kind == gl::ShaderProgram::SamplerKind::Volume;
  const auto& source = volume ? volumeTextures : textures;
  const auto it = source.find(name);
  const unsigned int tex = it != source.end() && it->second > 0 ? static_cast<unsigned int>(it->second) : 0;
  core::activeTexture(gl::tex::Texture0 + unit);
  if(volume) {
   ::glBindTexture(kTexture3D, tex);
  } else {
   core::bindTexture(kTexture2D, static_cast<int>(tex));
  }
  if(gl::GLCore::bindSampler != nullptr) gl::GLCore::bindSampler(static_cast<unsigned int>(unit), samplerObject(kind == gl::ShaderProgram::SamplerKind::Shadow));
  program.set1i(name, unit);
  g_highestSamplerUnit = std::max(g_highestSamplerUnit, unit);
  ++unit;
 }
}
void releaseSamplers(int maxUnits) {
 if(gl::GLCore::bindSampler == nullptr) return;
 const int limit = std::min(maxUnits, g_highestSamplerUnit + 1);
 for(int unit = 0; unit < limit; ++unit) {
  gl::GLCore::bindSampler(static_cast<unsigned int>(unit), 0);
 }
 g_highestSamplerUnit = -1;
}
bool isBufferFormatDirective(const std::string& trimmed) {
 if(trimmed.rfind("const ", 0) != 0) return false;
 const std::size_t equals = trimmed.find('=');
 const std::size_t marker = trimmed.find("Format");
 if(equals == std::string::npos || marker == std::string::npos || marker > equals) return false;
 const std::string name = trimmed.substr(0, marker);
 if(name.find("colortex") == std::string::npos && name.find("shadowcolor") == std::string::npos) return false;
 const std::size_t semicolon = trimmed.find(';', equals + 1);
 const std::string value =
     trimmed.substr(equals + 1, (semicolon == std::string::npos ? trimmed.size() : semicolon) - equals - 1);
 const std::size_t first = value.find_first_not_of(" \t");
 return first != std::string::npos && (std::isalpha(static_cast<unsigned char>(value[first])) != 0);
}
void refreshTextureAliases(std::unordered_map<std::string, int>& textures) {
 static constexpr std::array aliases = {
     std::pair{"gcolor", "colortex0"},     std::pair{"gdepth", "colortex1"},
     std::pair{"gnormal", "colortex2"},   std::pair{"composite", "colortex3"},
     std::pair{"gaux1", "colortex4"},     std::pair{"gaux2", "colortex5"},
     std::pair{"gaux3", "colortex6"},     std::pair{"gaux4", "colortex7"},
     std::pair{"depthtex", "depthtex0"},  std::pair{"gdepthtex", "depthtex0"},
     std::pair{"shadow", "shadowtex0"},   std::pair{"watershadow", "shadowtex1"},
     std::pair{"shadowcolor", "shadowcolor0"}};
 for(const auto& [alias, canonical] : aliases) {
  const auto found = textures.find(canonical);
  if(found != textures.end()) textures[std::string(alias)] = found->second;
 }
}
unsigned int bindColorImages(gl::ShaderProgram& program,
                             const std::unordered_map<std::string, int>& colorTextures,
                             const ShaderPackDefinition* definition) {
 if(gl::GLCore::bindImageTexture == nullptr) return 0;
 unsigned int unit = 0;
 const auto bindPrefix = [&](const char* imagePrefix, const char* bufferPrefix, int count) {
  for(int index = 0; index < count && unit < 16; ++index) {
   const std::string imageName = std::string(imagePrefix) + std::to_string(index);
   if(program.location(imageName) < 0) continue;
   const std::string bufferName = std::string(bufferPrefix) + std::to_string(index);
   const auto found = colorTextures.find(bufferName);
   if(found == colorTextures.end() || found->second <= 0) continue;
   unsigned int format = 0x8058;
   if(definition != nullptr) {
    const auto target = definition->targets.find(bufferName);
    if(target != definition->targets.end()) format = internalFormat(target->second.format);
   }
   gl::GLCore::bindImageTexture(unit, static_cast<unsigned int>(found->second), 0, 0, 0, 0x88BA, format);
   program.set1i(imageName, static_cast<int>(unit));
   ++unit;
  }
 };
 bindPrefix("colorimg", "colortex", 32);
 bindPrefix("shadowcolorimg", "shadowcolor", 8);
 return unit;
}
bool featureSupported(const std::string& feature);
static int glVersionMacro() {
 const char* text = reinterpret_cast<const char*>(::glGetString(0x1F02));
 if(text == nullptr) return 330;
 int major = 0;
 int minor = 0;
 std::sscanf(text, "%d.%d", &major, &minor);
 return major > 0 ? major * 100 + minor * 10 : 330;
}
static int maxColorBuffers() {
 static int buffers = 0;
 if(buffers == 0) {
  ::glGetIntegerv(0x8CDF, &buffers);
  buffers = std::clamp(buffers, 1, render::kMaxColorAttachments);
 }
 return buffers;
}
static std::string driverPreamble() {
 const auto text = [](unsigned int name) {
  const char* value = reinterpret_cast<const char*>(::glGetString(name));
  return lower(value == nullptr ? std::string{} : std::string(value));
 };
 const std::string vendor = text(0x1F00);
 const std::string renderer = text(0x1F01);
 const auto define = [](std::string_view value, std::string_view prefix,
                        const auto& choices) {
  for(const auto& [needle, name] : choices)
   if(value.find(needle) != std::string_view::npos)
    return "#define " + std::string(prefix) + std::string(name) + "\n";
  return "#define " + std::string(prefix) + "OTHER\n";
 };
 static constexpr std::array vendorNames = {
     std::pair{"nvidia", "NVIDIA"}, std::pair{"intel", "INTEL"},
     std::pair{"ati", "ATI"},       std::pair{"amd", "AMD"},
     std::pair{"mesa", "MESA"}};
 static constexpr std::array rendererNames = {
     std::pair{"geforce", "GEFORCE"}, std::pair{"quadro", "QUADRO"},
     std::pair{"radeon", "RADEON"},   std::pair{"intel", "INTEL"},
     std::pair{"gallium", "GALLIUM"}, std::pair{"mesa", "MESA"}};
 return define(vendor, "MC_GL_VENDOR_", vendorNames) +
        define(renderer, "MC_GL_RENDERER_", rendererNames);
}
namespace {
std::string trimCopy(std::string_view value) {
 const std::size_t first = value.find_first_not_of(" \t\r\n");
 if(first == std::string_view::npos) return {};
 const std::size_t last = value.find_last_not_of(" \t\r\n");
 return std::string(value.substr(first, last - first + 1));
}
template <typename T, std::size_t N>
T lookup(std::string value,
         const std::array<std::pair<std::string_view, T>, N>& entries,
         T fallback) {
 value = lower(std::move(value));
 const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
  return entry.first == value;
 });
 return found == entries.end() ? fallback : found->second;
}
template <std::size_t N>
void appendIndexedDefines(std::string& result,
                          std::string_view prefix,
                          const std::array<std::string_view, N>& names) {
 for(std::size_t index = 0; index < names.size(); ++index)
  result += "#define " + std::string(prefix) + std::string(names[index]) + " " +
            std::to_string(index) + "\n";
}
std::size_t sourceDeclarationOffset(const std::string& source) {
 std::size_t offset = 0;
 std::istringstream stream(source);
 for(std::string line; std::getline(stream, line);) {
  const std::string trimmed = trimCopy(line);
  if(!trimmed.empty() && !trimmed.starts_with("#")) break;
  offset += line.size() + 1;
 }
 return offset;
}
struct PPMacro {
 bool functionLike = false;
 std::vector<std::string> params;
 std::string body;
};
using PPMacroTable = std::unordered_map<std::string, PPMacro>;
bool isIdentStart(char c) {
 return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
bool isIdentChar(char c) {
 return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}
std::string lineForDirectiveParse(const std::string& line) {
 std::string out;
 out.reserve(line.size());
 for(std::size_t i = 0; i < line.size();) {
  if(line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') break;
  if(line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*') {
   const std::size_t close = line.find("*/", i + 2);
   if(close == std::string::npos) break;
   i = close + 2;
   out += ' ';
   continue;
  }
  out += line[i++];
 }
 return out;
}
class PPExpressionEval {
 public:
 explicit PPExpressionEval(std::string_view s) : s_(s) {}
 bool eval(double& out) {
  pos_ = 0;
  ok_ = true;
  out = ternary();
  skipWs();
  if(pos_ < s_.size()) ok_ = false;
  return ok_;
 }

 private:
 std::string_view s_;
 std::size_t pos_ = 0;
 bool ok_ = true;
 void skipWs() {
  while(pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_;
 }
 char peek() {
  skipWs();
  return pos_ < s_.size() ? s_[pos_] : '\0';
 }
 bool match(const char* tok) {
  skipWs();
  const std::size_t n = std::strlen(tok);
  if(s_.compare(pos_, n, tok) == 0) {
   pos_ += n;
   return true;
  }
  return false;
 }
 static long long toInt(double v) {
  return static_cast<long long>(v);
 }
 double ternary() {
  double c = lor();
  if(peek() == '?') {
   ++pos_;
   const double a = ternary();
   if(peek() == ':')
    ++pos_;
   else
    ok_ = false;
   const double b = ternary();
   return c != 0.0 ? a : b;
  }
  return c;
 }
 double lor() {
  double v = land();
  while(true) {
   skipWs();
   if(s_.compare(pos_, 2, "||") == 0) {
    pos_ += 2;
    const double r = land();
    v = (v != 0.0 || r != 0.0) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double land() {
  double v = bitor_();
  while(true) {
   skipWs();
   if(s_.compare(pos_, 2, "&&") == 0) {
    pos_ += 2;
    const double r = bitor_();
    v = (v != 0.0 && r != 0.0) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double bitor_() {
  double v = bitxor_();
  while(true) {
   skipWs();
   if(pos_ < s_.size() && s_[pos_] == '|' && (pos_ + 1 >= s_.size() || s_[pos_ + 1] != '|')) {
    ++pos_;
    v = static_cast<double>(toInt(v) | toInt(bitxor_()));
   } else
    break;
  }
  return v;
 }
 double bitxor_() {
  double v = bitand_();
  while(peek() == '^') {
   ++pos_;
   v = static_cast<double>(toInt(v) ^ toInt(bitand_()));
  }
  return v;
 }
 double bitand_() {
  double v = equality();
  while(true) {
   skipWs();
   if(pos_ < s_.size() && s_[pos_] == '&' && (pos_ + 1 >= s_.size() || s_[pos_ + 1] != '&')) {
    ++pos_;
    v = static_cast<double>(toInt(v) & toInt(equality()));
   } else
    break;
  }
  return v;
 }
 double equality() {
  double v = relational();
  while(true) {
   if(match("==")) {
    v = (v == relational()) ? 1.0 : 0.0;
   } else if(match("!=")) {
    v = (v != relational()) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double relational() {
  double v = shift();
  while(true) {
   skipWs();
   if(s_.compare(pos_, 2, "<=") == 0) {
    pos_ += 2;
    v = (v <= shift()) ? 1.0 : 0.0;
   } else if(s_.compare(pos_, 2, ">=") == 0) {
    pos_ += 2;
    v = (v >= shift()) ? 1.0 : 0.0;
   } else if(pos_ < s_.size() && s_[pos_] == '<' && s_.compare(pos_, 2, "<<") != 0) {
    ++pos_;
    v = (v < shift()) ? 1.0 : 0.0;
   } else if(pos_ < s_.size() && s_[pos_] == '>' && s_.compare(pos_, 2, ">>") != 0) {
    ++pos_;
    v = (v > shift()) ? 1.0 : 0.0;
   } else
    break;
  }
  return v;
 }
 double shift() {
  double v = additive();
  while(true) {
   if(match("<<")) {
    v = static_cast<double>(toInt(v) << toInt(additive()));
   } else if(match(">>")) {
    v = static_cast<double>(toInt(v) >> toInt(additive()));
   } else
    break;
  }
  return v;
 }
 double additive() {
  double v = multiplicative();
  while(true) {
   const char c = peek();
   if(c == '+') {
    ++pos_;
    v += multiplicative();
   } else if(c == '-') {
    ++pos_;
    v -= multiplicative();
   } else
    break;
  }
  return v;
 }
 double multiplicative() {
  double v = unary();
  while(true) {
   const char c = peek();
   if(c == '*') {
    ++pos_;
    v *= unary();
   } else if(c == '/') {
    ++pos_;
    const double r = unary();
    v = r != 0.0 ? v / r : 0.0;
   } else if(c == '%') {
    ++pos_;
    const long long r = toInt(unary());
    v = static_cast<double>(r != 0 ? toInt(v) % r : 0);
   } else
    break;
  }
  return v;
 }
 double unary() {
  const char c = peek();
  if(c == '!') {
   ++pos_;
   return unary() == 0.0 ? 1.0 : 0.0;
  }
  if(c == '~') {
   ++pos_;
   return static_cast<double>(~toInt(unary()));
  }
  if(c == '-') {
   ++pos_;
   return -unary();
  }
  if(c == '+') {
   ++pos_;
   return unary();
  }
  return primary();
 }
 double primary() {
  skipWs();
  if(pos_ >= s_.size()) {
   ok_ = false;
   return 0.0;
  }
  const char c = s_[pos_];
  if(c == '(') {
   ++pos_;
   const double v = ternary();
   if(peek() == ')')
    ++pos_;
   else
    ok_ = false;
   return v;
  }
  if(std::isdigit(static_cast<unsigned char>(c)) ||
     (c == '.' && pos_ + 1 < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_ + 1])))) {
   char* end = nullptr;
   const std::string text(s_.substr(pos_));
   const double v = std::strtod(text.c_str(), &end);
   pos_ += static_cast<std::size_t>(end - text.c_str());
   while(pos_ < s_.size() &&
         (s_[pos_] == 'u' || s_[pos_] == 'U' || s_[pos_] == 'l' || s_[pos_] == 'L' || s_[pos_] == 'f' ||
          s_[pos_] == 'F'))
    ++pos_;
   return v;
  }
  if(isIdentStart(c)) {
   std::size_t end = pos_;
   while(end < s_.size() && isIdentChar(s_[end])) ++end;
   const std::string_view ident = s_.substr(pos_, end - pos_);
   pos_ = end;
   if(ident == "true") return 1.0;
   return 0.0;
  }
  ok_ = false;
  ++pos_;
  return 0.0;
 }
};
std::string resolveDefinedOperators(const std::string& expr, const PPMacroTable& macros) {
 std::string out;
 out.reserve(expr.size());
 for(std::size_t i = 0; i < expr.size();) {
  if(isIdentStart(expr[i])) {
   std::size_t end = i;
   while(end < expr.size() && isIdentChar(expr[end])) ++end;
   const std::string ident = expr.substr(i, end - i);
   if(ident == "defined") {
    std::size_t p = end;
    while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
    bool paren = false;
    if(p < expr.size() && expr[p] == '(') {
     paren = true;
     ++p;
     while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
    }
    std::size_t nameEnd = p;
    while(nameEnd < expr.size() && isIdentChar(expr[nameEnd])) ++nameEnd;
    const std::string name = expr.substr(p, nameEnd - p);
    p = nameEnd;
    if(paren) {
     while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
     if(p < expr.size() && expr[p] == ')') ++p;
    }
    out += macros.count(name) > 0 ? '1' : '0';
    i = p;
    continue;
   }
   out += ident;
   i = end;
   continue;
  }
  out += expr[i++];
 }
 return out;
}
std::string expandIfExpression(std::string expr, const PPMacroTable& macros) {
 expr = resolveDefinedOperators(expr, macros);
 for(int pass = 0; pass < 64; ++pass) {
  bool changed = false;
  std::string out;
  out.reserve(expr.size());
  for(std::size_t i = 0; i < expr.size();) {
   if(!isIdentStart(expr[i])) {
    out += expr[i++];
    continue;
   }
   std::size_t end = i;
   while(end < expr.size() && isIdentChar(expr[end])) ++end;
   const std::string ident = expr.substr(i, end - i);
   const auto found = macros.find(ident);
   if(found == macros.end()) {
    out += ident;
    i = end;
    continue;
   }
   const PPMacro& macro = found->second;
   if(!macro.functionLike) {
    out += '(';
    out += macro.body.empty() ? "1" : macro.body;
    out += ')';
    i = end;
    changed = true;
    continue;
   }
   std::size_t p = end;
   while(p < expr.size() && std::isspace(static_cast<unsigned char>(expr[p]))) ++p;
   if(p >= expr.size() || expr[p] != '(') {
    out += ident;
    i = end;
    continue;
   }
   ++p;
   std::vector<std::string> args;
   std::string current;
   int depth = 1;
   while(p < expr.size() && depth > 0) {
    const char c = expr[p];
    if(c == '(')
     ++depth;
    else if(c == ')') {
     --depth;
     if(depth == 0) break;
    } else if(c == ',' && depth == 1) {
     args.push_back(trimCopy(current));
     current.clear();
     ++p;
     continue;
    }
    current += c;
    ++p;
   }
   args.push_back(trimCopy(current));
   if(p < expr.size()) ++p;
   std::string substituted;
   const std::string& body = macro.body;
   for(std::size_t b = 0; b < body.size();) {
    if(!isIdentStart(body[b])) {
     substituted += body[b++];
     continue;
    }
    std::size_t be = b;
    while(be < body.size() && isIdentChar(body[be])) ++be;
    const std::string bodyIdent = body.substr(b, be - b);
    bool replacedParam = false;
    for(std::size_t a = 0; a < macro.params.size() && a < args.size(); ++a) {
     if(macro.params[a] == bodyIdent) {
      substituted += '(' + args[a] + ')';
      replacedParam = true;
      break;
     }
    }
    if(!replacedParam) substituted += bodyIdent;
    b = be;
   }
   out += '(' + substituted + ')';
   i = p;
   changed = true;
  }
  expr = std::move(out);
  if(!changed) break;
 }
 return expr;
}
bool evaluateIfExpression(const std::string& rawExpr, const PPMacroTable& macros) {
 const std::string expanded = expandIfExpression(rawExpr, macros);
 PPExpressionEval eval(expanded);
 double value = 0.0;
 if(!eval.eval(value)) return false;
 return value != 0.0;
}
void parseDefineDirective(const std::string& afterKeyword, PPMacroTable& macros) {
 std::size_t i = 0;
 while(i < afterKeyword.size() && std::isspace(static_cast<unsigned char>(afterKeyword[i]))) ++i;
 std::size_t end = i;
 while(end < afterKeyword.size() && isIdentChar(afterKeyword[end])) ++end;
 if(end == i) return;
 const std::string name = afterKeyword.substr(i, end - i);
 PPMacro macro;
 if(end < afterKeyword.size() && afterKeyword[end] == '(') {
  macro.functionLike = true;
  std::size_t p = end + 1;
  std::string param;
  while(p < afterKeyword.size() && afterKeyword[p] != ')') {
   const char c = afterKeyword[p];
   if(c == ',') {
    if(!trimCopy(param).empty()) macro.params.push_back(trimCopy(param));
    param.clear();
   } else {
    param += c;
   }
   ++p;
  }
  if(!trimCopy(param).empty()) macro.params.push_back(trimCopy(param));
  end = p < afterKeyword.size() ? p + 1 : p;
 }
 macro.body = trimCopy(afterKeyword.substr(end));
 macros[name] = std::move(macro);
}
bool parseDirective(const std::string& trimmed, std::string& keyword, std::string& rest) {
 if(trimmed.empty() || trimmed[0] != '#') return false;
 std::size_t i = 1;
 while(i < trimmed.size() && (trimmed[i] == ' ' || trimmed[i] == '\t')) ++i;
 std::size_t end = i;
 while(end < trimmed.size() && std::isalpha(static_cast<unsigned char>(trimmed[end]))) ++end;
 keyword = trimmed.substr(i, end - i);
 rest = trimCopy(trimmed.substr(end));
 return true;
}
void seedMacrosFromDefines(const std::string& text, PPMacroTable& macros) {
 std::istringstream stream(text);
 std::string line;
 while(std::getline(stream, line)) {
  const std::string trimmed = trimCopy(lineForDirectiveParse(line));
  std::string keyword, rest;
  if(!parseDirective(trimmed, keyword, rest)) continue;
  if(keyword == "define") parseDefineDirective(rest, macros);
  if(keyword == "version") {
   std::istringstream directive(rest);
   int version = 0;
   std::string profile;
   directive >> version >> profile;
   if(version > 0) {
    PPMacro macro;
    macro.body = std::to_string(version);
    macros["__VERSION__"] = std::move(macro);
   }
   PPMacro flag;
   flag.body = "1";
   if(profile == "core")
    macros["GL_core_profile"] = flag;
   else if(profile == "compatibility")
    macros["GL_compatibility_profile"] = flag;
  }
 }
}
const std::vector<std::string>& supportedGlExtensions() {
 static std::vector<std::string> extensions;
 static bool initialized = false;
 if(!initialized && hasGlContext()) {
  initialized = true;
  int count = 0;
  ::glGetIntegerv(0x821D, &count);
  if(count > 0 && gl::GLCore::getStringi != nullptr) {
   extensions.reserve(static_cast<std::size_t>(count));
   for(int i = 0; i < count; ++i) {
    const unsigned char* name = gl::GLCore::getStringi(0x1F03, static_cast<unsigned>(i));
    if(name != nullptr) {
     extensions.emplace_back(reinterpret_cast<const char*>(name));
    }
   }
  }
  if(extensions.empty()) {
   const char* names = reinterpret_cast<const char*>(::glGetString(0x1F03));
   if(names != nullptr) {
    std::istringstream stream(names);
    for(std::string name; stream >> name;) {
     extensions.push_back(std::move(name));
    }
   }
  }
  std::erase_if(extensions, [](const std::string& extension) {
   return !extension.starts_with("GL_") ||
          !std::all_of(extension.begin(), extension.end(), [](unsigned char ch) {
           return std::isalnum(ch) != 0 || ch == '_';
          });
  });
  std::sort(extensions.begin(), extensions.end());
  extensions.erase(std::unique(extensions.begin(), extensions.end()), extensions.end());
 }
 return extensions;
}
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
  const std::string cleaned = trimCopy(lineForDirectiveParse(logical));
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
                                            const ShaderPackDefinition& pack,
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
bool isGbufferOrShadowProgramName(const std::string& programName) {
 const std::string_view name = programName;
 return name.starts_with("gbuffers_") || name == "shadow" ||
        name.starts_with("shadow_");
}
constexpr const char* kIrisLightmapTextureMatrixDecl =
    "const mat4 iris_lightmapTextureMatrix = mat4("
    "vec4(0.00390625, 0.0, 0.0, 0.0), "
    "vec4(0.0, 0.00390625, 0.0, 0.0), "
    "vec4(0.0, 0.0, 0.00390625, 0.0), "
    "vec4(0.03125, 0.03125, 0.03125, 1.0));\n";
static std::string lowerVertexSource(const std::string& programName, std::string vertexSource) {
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
std::string versionPreamble(const ShaderPackDefinition& pack, const std::string& source, bool compute) {
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
 std::string result = "#version " + std::to_string(version) + (profile.empty() ? "\n" : " " + profile + "\n") +
                      "#define IS_IRIS\n"
                      "#define IRIS_VERSION 10902\n#define MC_VERSION 10703\n#define MC_GL_VERSION " +
                      std::to_string(glVersionMacro()) + "\n"
                                                         "#define MC_GLSL_VERSION " +
                      std::to_string(version) + "\n"
                                                "#define MAX_COLOR_BUFFERS " +
                      std::to_string(maxColorBuffers()) +
                      "\n#define MC_OS_WINDOWS\n#define MC_HAND_DEPTH 0.125\n#define MC_MIPMAP_LEVEL " +
                      std::to_string(std::max(0, pack.mcMipmapLevel)) + "\n";
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
 return result;
}
int maxTextureUnits() {
 static int units = 0;
 if(units == 0) {
  int queried = 0;
  ::glGetIntegerv(0x8872, &queried);
  units = queried > 0 ? queried : 16;
 }
 return units;
}
namespace {
struct FormatInfo {
 std::string_view name;
 render::ColorFormat format;
 unsigned int internal;
};
static constexpr std::array kFormats = {
    FormatInfo{"rgba", render::ColorFormat::Rgba8, 0x8058},
    FormatInfo{"rgba8", render::ColorFormat::Rgba8, 0x8058},
    FormatInfo{"r8", render::ColorFormat::R8, 0x8229},
    FormatInfo{"r16", render::ColorFormat::R16, 0x8058},
    FormatInfo{"r16f", render::ColorFormat::R16F, 0x822D},
    FormatInfo{"r32f", render::ColorFormat::R32F, 0x822E},
    FormatInfo{"rg8", render::ColorFormat::Rg8, 0x822B},
    FormatInfo{"rg16", render::ColorFormat::Rg16, 0x8058},
    FormatInfo{"rg16f", render::ColorFormat::Rg16F, 0x822F},
    FormatInfo{"rg32f", render::ColorFormat::Rg32F, 0x8230},
    FormatInfo{"rgb8", render::ColorFormat::Rgb8, 0x8058},
    FormatInfo{"rgb16", render::ColorFormat::Rgb16, 0x8058},
    FormatInfo{"rgb16f", render::ColorFormat::Rgb16F, 0x8058},
    FormatInfo{"rgb32f", render::ColorFormat::Rgb32F, 0x8058},
    FormatInfo{"r11f_g11f_b10f", render::ColorFormat::R11G11B10F, 0x8058},
    FormatInfo{"rgb10_a2", render::ColorFormat::Rgb10A2, 0x8058},
    FormatInfo{"rgba16", render::ColorFormat::Rgba16, 0x8058},
    FormatInfo{"rgba16f", render::ColorFormat::Rgba16F, 0x881A},
    FormatInfo{"rgba32f", render::ColorFormat::Rgba32F, 0x8814},
    FormatInfo{"r8ui", render::ColorFormat::R8Ui, 0x8232},
    FormatInfo{"r16ui", render::ColorFormat::R16Ui, 0x8234},
    FormatInfo{"r32ui", render::ColorFormat::R32Ui, 0x8236},
    FormatInfo{"rg8ui", render::ColorFormat::Rg8Ui, 0x8058},
    FormatInfo{"rg16ui", render::ColorFormat::Rg16Ui, 0x8058},
    FormatInfo{"rg32ui", render::ColorFormat::Rg32Ui, 0x8058},
    FormatInfo{"rgba8ui", render::ColorFormat::Rgba8Ui, 0x8D7C},
    FormatInfo{"rgba16ui", render::ColorFormat::Rgba16Ui, 0x8D76},
    FormatInfo{"rgba32ui", render::ColorFormat::Rgba32Ui, 0x8D70},
    FormatInfo{"r8i", render::ColorFormat::R8I, 0x8058},
    FormatInfo{"r16i", render::ColorFormat::R16I, 0x8058},
    FormatInfo{"r32i", render::ColorFormat::R32I, 0x8058},
    FormatInfo{"rg8i", render::ColorFormat::Rg8I, 0x8058},
    FormatInfo{"rg16i", render::ColorFormat::Rg16I, 0x8058},
    FormatInfo{"rg32i", render::ColorFormat::Rg32I, 0x8058},
    FormatInfo{"rgba8i", render::ColorFormat::Rgba8I, 0x8058},
    FormatInfo{"rgba16i", render::ColorFormat::Rgba16I, 0x8058},
    FormatInfo{"rgba32i", render::ColorFormat::Rgba32I, 0x8058}};
const FormatInfo* findFormat(std::string value) {
 value = lower(std::move(value));
 const auto found = std::find_if(kFormats.begin(), kFormats.end(), [&](const FormatInfo& info) {
  return info.name == value;
 });
 return found == kFormats.end() ? nullptr : &*found;
}
}
render::ColorFormat parseFormat(const std::string& format) {
 const FormatInfo* info = findFormat(format);
 return info == nullptr ? render::ColorFormat::Rgba8 : info->format;
}
std::string resolveShaderIncludes(const ShaderReadText& readText,
                                  const std::string& path,
                                  bool stripFormatDirectives) {
 std::set<std::string> stack;
 std::function<std::string(const std::string&)> resolve = [&](const std::string& current) -> std::string {
  if(!stack.insert(current).second) {
   return {};
  }
  const std::string source = readText(current);
  std::string result;
  std::istringstream stream(source);
  std::string line;
  while(std::getline(stream, line)) {
   const std::size_t start = line.find_first_not_of(" \t");
   const std::string trimmed = start == std::string::npos ? std::string{} : line.substr(start);
   if(stripFormatDirectives && isBufferFormatDirective(trimmed)) {
    result += '\n';
    continue;
   }
   if(trimmed.rfind("#include", 0) != 0) {
    result += line + '\n';
    continue;
   }
   const std::size_t q1 = trimmed.find('"', 8);
   const std::size_t q2 = q1 == std::string::npos ? std::string::npos : trimmed.find('"', q1 + 1);
   if(q2 == std::string::npos) {
    result += line + '\n';
    continue;
   }
   const std::string include = trimmed.substr(q1 + 1, q2 - q1 - 1);
   const std::filesystem::path includePath = include.starts_with('/')
                                                 ? std::filesystem::path("shaders") / include.substr(1)
                                                 : std::filesystem::path(current).parent_path() / include;
   const std::string included = resolve(includePath.lexically_normal().generic_string());
   if(!included.empty()) {
    result += included;
   }
  }
  stack.erase(current);
  return result;
 };
 return resolve(path);
}
namespace {
std::vector<int> parseRenderTargetsList(std::string_view list) {
 const std::size_t close = list.find("*/");
 if(close != std::string::npos) {
  list = list.substr(0, close);
 }
 std::string normalized(list);
 std::replace(normalized.begin(), normalized.end(), ',', ' ');
 std::istringstream values(normalized);
 std::vector<int> indices;
 int index = -1;
 while(values >> index) {
  if(index >= 0 && index < 32) {
   indices.push_back(index);
  }
 }
 return indices;
}
std::vector<int> parseDrawBuffersList(std::string_view list) {
 const std::size_t close = list.find("*/");
 if(close != std::string::npos) {
  list = list.substr(0, close);
 }
 std::vector<int> indices;
 for(char ch : list) {
  if(ch >= '0' && ch <= '9') {
   indices.push_back(ch - '0');
  }
 }
 return indices;
}
void scanDirectiveComment(std::string_view block,
                          std::vector<int>& lastRenderTargets,
                          std::vector<int>& lastDrawBuffers) {
 const std::size_t renderTargets = block.find("RENDERTARGETS:");
 if(renderTargets != std::string::npos) {
  const std::vector<int> parsed = parseRenderTargetsList(block.substr(renderTargets + 14));
  if(!parsed.empty()) {
   lastRenderTargets = parsed;
   return;
  }
 }
 const std::size_t drawBuffers = block.find("DRAWBUFFERS:");
 if(drawBuffers != std::string::npos) {
  const std::vector<int> parsed = parseDrawBuffersList(block.substr(drawBuffers + 12));
  if(!parsed.empty()) {
   lastDrawBuffers = parsed;
  }
 }
}
}
std::vector<int> defaultRenderTargetIndices() {
 return {0, 1, 2, 3, 4, 5, 6, 7};
}
std::vector<int> parseRenderTargetIndices(const std::string& source) {
 std::vector<int> lastRenderTargets;
 std::vector<int> lastDrawBuffers;
 for(std::size_t i = 0; i + 1 < source.size(); ++i) {
  if(source[i] != '/' || source[i + 1] != '*') {
   continue;
  }
  const std::size_t close = source.find("*/", i + 2);
  if(close == std::string::npos) {
   break;
  }
  scanDirectiveComment(std::string_view(source).substr(i, close + 2 - i), lastRenderTargets, lastDrawBuffers);
  i = close + 1;
 }
 if(!lastRenderTargets.empty()) {
  return lastRenderTargets;
 }
 return lastDrawBuffers;
}
std::vector<std::string> renderTargetOutputNames(const std::string& source) {
 std::vector<int> indices = parseRenderTargetIndices(source);
 if(indices.empty()) {
  indices = defaultRenderTargetIndices();
 }
 std::vector<std::string> outputs;
 outputs.reserve(indices.size());
 for(int index : indices) {
  outputs.push_back("colortex" + std::to_string(index));
 }
 return outputs;
}
unsigned int pixelFormat(std::string value) {
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 10> entries = {
     std::pair<std::string_view, unsigned int>{"red", 0x1903},
     {"r", 0x1903}, {"rg", 0x8227}, {"rgb", 0x1907},
     {"red_integer", 0x8D94}, {"redinteger", 0x8D94},
     {"rg_integer", 0x8228}, {"rginteger", 0x8228},
     {"rgba_integer", 0x8D99}, {"rgbainteger", 0x8D99}};
 return lookup(std::move(value), entries, 0x1908u);
}
unsigned int pixelType(std::string value) {
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 10> entries = {
     std::pair<std::string_view, unsigned int>{"byte", 0x1400},
     {"short", 0x1402}, {"unsigned_short", 0x1403}, {"unsignedshort", 0x1403},
     {"int", 0x1404}, {"unsigned_int", 0x1405}, {"unsignedint", 0x1405},
     {"float", 0x1406}, {"half_float", 0x140B}, {"halffloat", 0x140B}};
 return lookup(std::move(value), entries, 0x1401u);
}
unsigned int internalFormat(std::string value) {
 const FormatInfo* info = findFormat(std::move(value));
 return info == nullptr ? 0x8058 : info->internal;
}
unsigned int textureTarget(std::string value, std::size_t dimensions) {
 value = lower(std::move(value));
 if(value.find("3d") != std::string::npos || dimensions == 3) return kTexture3D;
 if(value.find("1d") != std::string::npos || dimensions == 1) return 0x0DE0;
 return kTexture2D;
}
unsigned int blendFactor(std::string value) {
 static constexpr std::array<std::pair<std::string_view, unsigned int>, 18> entries = {
     std::pair<std::string_view, unsigned int>{"zero", 0},
     {"one", 1}, {"srccolor", 0x0300}, {"src_color", 0x0300},
     {"oneminussrccolor", 0x0301}, {"one_minus_src_color", 0x0301},
     {"srcalpha", 0x0302}, {"src_alpha", 0x0302},
     {"oneminussrcalpha", 0x0303}, {"one_minus_src_alpha", 0x0303},
     {"dstalpha", 0x0304}, {"dst_alpha", 0x0304},
     {"oneminusdstalpha", 0x0305}, {"one_minus_dst_alpha", 0x0305},
     {"dstcolor", 0x0306}, {"dst_color", 0x0306},
     {"oneminusdstcolor", 0x0307}, {"one_minus_dst_color", 0x0307}};
 return lookup(std::move(value), entries, 1u);
}
void applyBufferBlends(const ShaderPackDefinition& pack, const std::string& program) {
 int drawFbo = 0;
 ::glGetIntegerv(0x8CA6, &drawFbo);
 const bool indexedOk = drawFbo != 0 && gl::GLCore::perBufferBlendingSupported &&
                        gl::GLCore::blendFunci != nullptr;
 if(indexedOk) {
  int maxDrawBuffers = 1;
  ::glGetIntegerv(0x8824, &maxDrawBuffers);
  maxDrawBuffers = std::max(1, maxDrawBuffers);
  const bool enabled = core::blendEnabled();
  for(int buffer = 0; buffer < maxDrawBuffers; ++buffer) {
   if(enabled)
    gl::GLCore::enablei(0x0BE2, static_cast<unsigned int>(buffer));
   else
    gl::GLCore::disablei(0x0BE2, static_cast<unsigned int>(buffer));
  }
 }
 for(const BufferBlend& blend : pack.bufferBlends) {
  if(blend.program != program) continue;
  const unsigned int source = blendFactor(blend.source);
  const unsigned int destination = blendFactor(blend.destination);
  const unsigned int sourceAlpha = blendFactor(blend.sourceAlpha);
  const unsigned int destinationAlpha = blendFactor(blend.destinationAlpha);
  if(blend.buffer >= 0) {
   if(!indexedOk) continue;
   if(blend.enabled) {
    gl::GLCore::enablei(0x0BE2, static_cast<unsigned int>(blend.buffer));
    if(gl::GLCore::blendFuncSeparatei != nullptr)
     gl::GLCore::blendFuncSeparatei(static_cast<unsigned int>(blend.buffer), source, destination,
                                    sourceAlpha, destinationAlpha);
    else
     gl::GLCore::blendFunci(static_cast<unsigned int>(blend.buffer), source, destination);
   } else {
    gl::GLCore::disablei(0x0BE2, static_cast<unsigned int>(blend.buffer));
   }
  } else if(blend.enabled) {
   core::enableBlend();
   if(gl::GLCore::blendFuncSeparate != nullptr) {
    gl::GLCore::blendFuncSeparate(source, destination, sourceAlpha, destinationAlpha);
   } else {
    core::blendFunc(static_cast<int>(source), static_cast<int>(destination));
   }
  } else {
   core::disableBlend();
  }
 }
}
void applyAlphaTest(const ShaderPackDefinition& pack, const std::string& program) {
 for(const AlphaTestDirective& directive : pack.alphaTests) {
  if(directive.program != program) continue;
  if(!directive.enabled) {
   core::setAlphaTestRef(0.0f);
   return;
  }
  const std::string func = lower(directive.func);
  if(func == "always" || func == "gl_always") {
   core::setAlphaTestRef(0.0f);
  } else {
   core::setAlphaTestRef(directive.ref);
  }
  return;
 }
}
namespace {
bool programGetsCompatAlphaTest(const std::string& programName) {
 const std::string_view name = programName;
 if(name == "shadow" || name.starts_with("shadow_")) {
  return !name.starts_with("shadowcomp");
 }
 if(!name.starts_with("gbuffers_")) return false;
 const std::string lowerName = lower(programName);
 return lowerName.find("water") == std::string::npos && lowerName.find("translucent") == std::string::npos;
}
bool fragmentWritesLegacyFragOutput(const std::string& source) {
 return referencesToken(source, "gl_FragData") || referencesToken(source, "gl_FragColor");
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
}
static std::string lowerFragmentSource(const std::string& programName, std::string fragmentSource) {
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
std::string prepareSource(const std::string& programName,
                          ShaderStage stage,
                          const ShaderPackDefinition& pack,
                          const std::string& source,
                          const std::string& preamble) {
 std::string prepared = normalizePackSource(source, preamble);
 if(stage == ShaderStage::Vertex)
  return injectChunkFadeAttribute(programName, pack,
                                  lowerVertexSource(programName, std::move(prepared)));
 if(stage == ShaderStage::Fragment)
  return lowerFragmentSource(programName, std::move(prepared));
 return prepared;
}
bool normalizeSettingValue(const PackSetting& setting, const std::string& input, std::string& output) {
 if(setting.type == SettingType::Bool) {
  const std::string normalized = lower(input);
  if(normalized == "1" || normalized == "true" || normalized == "on") {
   output = "1";
   return true;
  }
  if(normalized == "0" || normalized == "false" || normalized == "off") {
   output = "0";
   return true;
  }
  return false;
 }
 char* end = nullptr;
 const double parsed = std::strtod(input.c_str(), &end);
 if(end == input.c_str() || *end != '\0' || !std::isfinite(parsed)) {
  return false;
 }
 double value = std::clamp(parsed, setting.minimum, setting.maximum);
 value = setting.minimum + std::round((value - setting.minimum) / setting.step) * setting.step;
 value = std::clamp(value, setting.minimum, setting.maximum);
 if(setting.type == SettingType::Int) {
  output = std::to_string(static_cast<int>(std::lround(value)));
 } else {
  output = std::to_string(value);
 }
 return true;
}
bool hasGlContext() {
#ifdef _WIN32
 return wglGetCurrentContext() != nullptr;
#else
 return gl::GLCore::activeTexture != nullptr;
#endif
}
bool featureSupported(const std::string& feature) {
 if(feature == "COMPUTE_SHADERS") return gl::GLCore::computeSupported;
 if(feature == "SSBO") return gl::GLCore::ssboSupported;
 if(feature == "CUSTOM_IMAGES") return gl::GLCore::bindImageTexture != nullptr;
 if(feature == "SEPARATE_HARDWARE_SAMPLERS") return gl::GLCore::samplerObjectsSupported;
 if(feature == "PER_BUFFER_BLENDING") return gl::GLCore::perBufferBlendingSupported;
 if(feature == "TESSELLATION_SHADERS" || feature == "TESSELATION_SHADERS")
  return glVersionMacro() >= 400 && gl::GLCore::patchParameteri != nullptr;
 if(feature == "ENTITY_TRANSLUCENT" || feature == "HIGHER_SHADOWCOLOR" || feature == "REVERSED_CULLING" ||
    feature == "BLOCK_EMISSION_ATTRIBUTE" || feature == "CAN_DISABLE_WEATHER" || feature == "FADE_VARIABLE" ||
    feature == "TEXTURE_FILTERING")
  return true;
 return false;
}
}
