#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include <array>
#include <sstream>
#include <string_view>
#include <utility>
#include "net/minecraft/client/render/shaders/ConditionalState.hpp"
#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"

namespace net::minecraft::client::render {
std::string normalizePackSource(const std::string& source, const std::string& preamble) {
 PPMacroTable macros;
 seedMacrosFromDefines(preamble, macros);
 for(const std::string& extension : glShaderExtensions()) {
  PPMacro flag;
  flag.body = "1";
  macros[extension] = std::move(flag);
 }
 ConditionalState stack(ConditionalState::Flavor::Glsl);
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
  const auto emit = [&](std::string_view text = {}) {
   body += text;
   body.append(static_cast<std::size_t>(continuations + 1), '\n');
  };
  const std::string parsedLine = lineForDirectiveParse(logical);
  const std::size_t first = parsedLine.find_first_not_of(" \t\r\n");
  const std::string cleaned = first == std::string::npos ? std::string{} : parsedLine.substr(first);
  std::string keyword;
  std::string rest;
  if(parseDirective(cleaned, keyword, rest)) {
   if(keyword == "if" || keyword == "ifdef" || keyword == "ifndef") {
    bool condition = false;
    if(stack.active()) {
     if(keyword == "if") condition = evaluateIfExpression(rest, macros);
     else {
      std::size_t end = 0;
      while(end < rest.size() && isIdentChar(rest[end])) ++end;
      const bool defined = macros.contains(rest.substr(0, end));
      condition = keyword == "ifdef" ? defined : !defined;
     }
    }
    stack.push(condition);
    emit();
    continue;
   }
   if(keyword == "elif") {
    stack.elif(evaluateIfExpression(rest, macros));
    emit();
    continue;
   }
   if(keyword == "else") {
    stack.else_();
    emit();
    continue;
   }
   if(keyword == "endif") {
    stack.endif();
    emit();
    continue;
   }
   if(!stack.active()) {
    emit();
    continue;
   }
   if(keyword == "define") {
    parseDefineDirective(rest, macros);
    emit(logical);
    continue;
   }
   if(keyword == "undef") {
    std::size_t end = 0;
    while(end < rest.size() && isIdentChar(rest[end])) ++end;
    macros.erase(rest.substr(0, end));
    emit(logical);
    continue;
   }
   if(keyword == "version") {
    emit();
    continue;
   }
   if(keyword == "extension") {
    extensions += "#extension " + rest + '\n';
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
  if(stack.active()) emit(logical);
  else emit();
 }
 return extensions + body;
}

bool isCompositeStyleProgramName(const std::string& programName) {
 constexpr std::array<std::string_view, 6> prefixes = {"begin", "shadowcomp", "prepare", "deferred", "composite", "final"};
 const std::string_view name = programName;
 for(const std::string_view prefix : prefixes) {
  if(!name.starts_with(prefix)) continue;
  if(name.size() == prefix.size()) return true;
  const char next = name[prefix.size()];
  if(next == '_' || (next >= '0' && next <= '9')) return true;
 }
 return false;
}

std::string defaultCompositeVertexShader() {
 return GlslSnippets::get("default_composite.vsh");
}
std::string defaultRasterVertexShader() {
 return GlslSnippets::get("default_raster.vsh");
}
}
