#pragma once
#include <string>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/ShaderTransform.hpp"

namespace net::minecraft::client::render {
std::string canonicalizeCoreSource(const std::string& programName,
                                   ShaderStage stage,
                                   const PackDefinition& pack,
                                   std::string source,
                                   const ShaderTransformContext& context);
}
