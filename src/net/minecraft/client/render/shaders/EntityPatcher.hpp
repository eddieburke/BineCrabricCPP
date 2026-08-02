#pragma once
#include <string>
#include "net/minecraft/client/render/shaders/ShaderTransform.hpp"

namespace net::minecraft::client::render {
void patchEntityInputs(std::string& source,
                       ShaderStage stage,
                       const ShaderTransformContext& context);
}
