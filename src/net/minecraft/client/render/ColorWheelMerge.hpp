#pragma once
#include <string>
#include <string_view>

namespace net::minecraft::client::render::glutil {
enum class ShaderStage;

// https://djefrey.github.io/colorwheel/
// https://djefrey.github.io/colorwheel/guides/adapt-your-shaderpack/1_programs/
[[nodiscard]] bool isColorWheelProgramName(std::string_view programName);

[[nodiscard]] constexpr const char* colorWheelVersionMacro() { return "10209"; }
void appendColorWheelMacros(std::string& preamble);

[[nodiscard]] std::string mergeColorWheelMaterial(const std::string& programName, ShaderStage stage,
                                                  std::string source);
}
