#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <system_error>
#include <unordered_map>
#include <utility>

namespace net::minecraft::client::render {
bool isBufferFormatDirective(std::string_view trimmed) {
 if(!trimmed.starts_with("const ")) return false;
 const std::size_t equals = trimmed.find('=');
 const std::size_t marker = trimmed.find("Format");
 if(equals == std::string_view::npos || marker == std::string_view::npos || marker > equals) return false;
 const std::string_view name = trimmed.substr(0, marker);
 if(name.find("colortex") == std::string_view::npos && name.find("shadowcolor") == std::string_view::npos) return false;
 const std::size_t semicolon = trimmed.find(';', equals + 1);
 const std::string_view value =
     trimmed.substr(equals + 1, (semicolon == std::string_view::npos ? trimmed.size() : semicolon) - equals - 1);
 const std::size_t first = value.find_first_not_of(" \t");
 return first != std::string_view::npos && (std::isalpha(static_cast<unsigned char>(value[first])) != 0);
}

const std::string& IncludeGraph::resolve(const std::string& path,
                                         std::unordered_map<std::string, std::string>& memo) {
 if(const auto cached = memo.find(path); cached != memo.end()) {
  return cached->second;
 }
 for(const std::string& active : stack_) {
  if(active == path) {
   throw IncludeResolveError("include cycle: " + path);
  }
 }
 stack_.push_back(path);
 const auto fileIt = files_.find(path);
 if(fileIt == files_.end()) {
  files_.emplace(path, readText_(path));
 }
 const std::string& source = files_.at(path);
 std::string result;
 result.reserve(source.size());
 std::string_view srcView(source);
 std::size_t lineStart = 0;
 while(lineStart < srcView.size()) {
  std::size_t lineEnd = srcView.find('\n', lineStart);
  std::string_view line = lineEnd == std::string_view::npos ? srcView.substr(lineStart) : srcView.substr(lineStart, lineEnd - lineStart);
  if(lineEnd != std::string_view::npos) lineStart = lineEnd + 1;
  else lineStart = srcView.size();
  if(!line.empty() && line.back() == '\r') line.remove_suffix(1);
  const std::size_t start = line.find_first_not_of(" \t");
  const std::string_view trimmed = start == std::string_view::npos ? std::string_view{} : line.substr(start);
  if(stripFormatDirectives_ && isBufferFormatDirective(trimmed)) {
   result += '\n';
   continue;
  }
  if(!trimmed.starts_with("#include")) {
   result.append(line.data(), line.size());
   result += '\n';
   continue;
  }
  std::size_t q1 = trimmed.find('"', 8);
  std::size_t q2 = q1 == std::string_view::npos ? std::string_view::npos : trimmed.find('"', q1 + 1);
  if(q1 == std::string_view::npos) {
   q1 = trimmed.find('<', 8);
   q2 = q1 == std::string_view::npos ? std::string_view::npos : trimmed.find('>', q1 + 1);
  }
  if(q2 == std::string_view::npos) {
   throw IncludeResolveError("malformed #include in " + path + ": " + std::string(trimmed));
  }
  const std::string_view include = trimmed.substr(q1 + 1, q2 - q1 - 1);
  const std::filesystem::path includePath = include.starts_with('/')
                                                ? std::filesystem::path("shaders") / include.substr(1)
                                                : std::filesystem::path(path).parent_path() / include;
  const std::string includeResolved = includePath.lexically_normal().generic_string();
  const std::string& included = resolve(includeResolved, memo);
  if(included.empty()) {
   throw IncludeResolveError("missing or empty #include \"" + std::string(include) + "\" from " + path +
                             " (resolved " + includeResolved + ")");
  }
  result += included;
 }
 stack_.pop_back();
 return memo.emplace(path, std::move(result)).first->second;
}

std::string resolveShaderIncludes(const ShaderReadText& readText,
                                  const std::string& path,
                                  bool stripFormatDirectives,
                                  std::unordered_map<std::string, std::string>& memo) {
 IncludeGraph graph(readText, stripFormatDirectives);
 return graph.resolve(path, memo);
}

namespace {
std::vector<int> parseRenderTargetsList(std::string_view list) {
 const std::size_t close = list.find("*/");
 if(close != std::string::npos) {
  list = list.substr(0, close);
 }
 std::vector<int> indices;
 std::size_t i = 0;
 while(i < list.size()) {
  while(i < list.size() && (list[i] == ',' || std::isspace(static_cast<unsigned char>(list[i])))) ++i;
  if(i >= list.size()) break;
  const std::size_t start = i;
  while(i < list.size() && list[i] != ',' && !std::isspace(static_cast<unsigned char>(list[i]))) ++i;
  int index = -1;
  const auto [ptr, error] = std::from_chars(list.data() + start, list.data() + i, index);
  if(error != std::errc()) break;
  if(index >= 0 && index < 32) {
   indices.push_back(index);
  }
  if(ptr != list.data() + i) break;
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
 for(std::size_t i = 0;;) {
  i = source.find("//", i);
  if(i == std::string::npos) break;
  const std::size_t eol = source.find('\n', i);
  scanDirectiveComment(
      std::string_view(source).substr(i, eol == std::string::npos ? std::string::npos : eol - i),
      lastRenderTargets, lastDrawBuffers);
  i = eol == std::string::npos ? source.size() : eol + 1;
 }
 for(std::size_t i = 0;;) {
  i = source.find("/*", i);
  if(i == std::string::npos) break;
  const std::size_t close = source.find("*/", i + 2);
  if(close == std::string::npos) break;
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
