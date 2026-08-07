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

bool isBufferFormatDirective(const std::string& trimmed);
[[nodiscard]] std::string resolveShaderIncludes(const ShaderReadText& readText,
                                                const std::string& path,
                                                bool stripFormatDirectives,
                                                std::unordered_map<std::string, std::string>& memo);
[[nodiscard]] std::vector<int> defaultRenderTargetIndices();
[[nodiscard]] std::vector<int> parseRenderTargetIndices(const std::string& source);
[[nodiscard]] std::vector<std::string> renderTargetOutputNames(const std::string& source);
}
