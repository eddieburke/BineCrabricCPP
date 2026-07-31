#pragma once
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft::client::render::glutil {
using ShaderReadText = std::function<std::string(std::string_view)>;

class IncludeResolveError : public std::runtime_error {
 public:
 using std::runtime_error::runtime_error;
};

bool isBufferFormatDirective(const std::string& trimmed);
// Throws IncludeResolveError on cycle, missing/empty include, or malformed #include.
[[nodiscard]] std::string resolveShaderIncludes(const ShaderReadText& readText,
                                                const std::string& path,
                                                bool stripFormatDirectives = false);
[[nodiscard]] std::vector<int> defaultRenderTargetIndices();
[[nodiscard]] std::vector<int> parseRenderTargetIndices(const std::string& source);
[[nodiscard]] std::vector<std::string> renderTargetOutputNames(const std::string& source);
}
