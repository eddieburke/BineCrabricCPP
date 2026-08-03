#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <set>
#include <sstream>

namespace net::minecraft::client::render {
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

std::string resolveShaderIncludes(const ShaderReadText& readText,
                                  const std::string& path,
                                  bool stripFormatDirectives) {
 std::set<std::string> stack;
 std::function<std::string(const std::string&)> resolve = [&](const std::string& current) -> std::string {
  if(!stack.insert(current).second) {
   throw IncludeResolveError("include cycle: " + current);
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
   std::size_t q1 = trimmed.find('"', 8);
   std::size_t q2 = q1 == std::string::npos ? std::string::npos : trimmed.find('"', q1 + 1);
   if(q1 == std::string::npos) {
    q1 = trimmed.find('<', 8);
    q2 = q1 == std::string::npos ? std::string::npos : trimmed.find('>', q1 + 1);
   }
   if(q2 == std::string::npos) {
    throw IncludeResolveError("malformed #include in " + current + ": " + trimmed);
   }
   const std::string include = trimmed.substr(q1 + 1, q2 - q1 - 1);
   const std::filesystem::path includePath = include.starts_with('/')
                                                 ? std::filesystem::path("shaders") / include.substr(1)
                                                 : std::filesystem::path(current).parent_path() / include;
   const std::string includeResolved = includePath.lexically_normal().generic_string();
   const std::string included = resolve(includeResolved);
   if(included.empty()) {
    throw IncludeResolveError("missing or empty #include \"" + include + "\" from " + current +
                              " (resolved " + includeResolved + ")");
   }
   result += included;
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
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ProgramDirectives.java#L73-L76
 return {0};
}

std::vector<int> parseRenderTargetIndices(const std::string& source) {
 std::vector<int> lastRenderTargets;
 std::vector<int> lastDrawBuffers;
 std::istringstream stream(source);
 std::string line;
 while(std::getline(stream, line)) {
  const std::size_t lineComment = line.find("//");
  if(lineComment != std::string::npos) {
   scanDirectiveComment(std::string_view(line).substr(lineComment), lastRenderTargets, lastDrawBuffers);
  }
 }
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
}
