#pragma once
#include <string>
#include <string_view>
#include "net/minecraft/client/render/shaders/ShaderTransform.hpp"

namespace net::minecraft::client::render {

// https://djefrey.github.io/colorwheel/
// https://djefrey.github.io/colorwheel/guides/adapt-your-shaderpack/1_programs/
[[nodiscard]] bool isColorWheelProgramName(std::string_view programName);

[[nodiscard]] constexpr const char* colorWheelVersionMacro() { return "10209"; }
void appendColorWheelMacros(std::string& preamble);

[[nodiscard]] std::string mergeColorWheelMaterial(const std::string& programName, ShaderStage stage,
                                                  std::string source);
}
