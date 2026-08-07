#pragma once
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::render {
using ShaderReadText = std::function<std::string(std::string_view)>;

class IncludeResolveError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

bool isBufferFormatDirective(std::string_view trimmed);

// Directed include graph: every file is read once into an immutable content
// buffer, child inclusions resolve via depth-first traversal with a visitor
// stack for cycle detection, and the merged text for each path lands in memo.
class IncludeGraph {
 public:
  IncludeGraph(const ShaderReadText& readText, bool stripFormatDirectives)
      : readText_(readText), stripFormatDirectives_(stripFormatDirectives) {
  }
  [[nodiscard]] const std::string& resolve(const std::string& path,
                                           std::unordered_map<std::string, std::string>& memo);

 private:
  const ShaderReadText& readText_;
  bool stripFormatDirectives_;
  std::unordered_map<std::string, std::string> files_;
  std::vector<std::string> stack_;
};

[[nodiscard]] std::string resolveShaderIncludes(const ShaderReadText& readText,
                                                const std::string& path,
                                                bool stripFormatDirectives,
                                                std::unordered_map<std::string, std::string>& memo);
[[nodiscard]] std::vector<int> defaultRenderTargetIndices();
[[nodiscard]] std::vector<int> parseRenderTargetIndices(const std::string& source);
[[nodiscard]] std::vector<std::string> renderTargetOutputNames(const std::string& source);
} // namespace net::minecraft::client::render
