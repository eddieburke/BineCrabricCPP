#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include <utility>
#include "net/minecraft/client/render/shaders/ColorWheelMerge.hpp"
#include "net/minecraft/client/render/shaders/Core330Transformer.hpp"

namespace net::minecraft::client::render {
std::string prepareSource(const std::string& programName,
                          ShaderStage stage,
                          const PackDefinition& pack,
                          const std::string& source,
                          const std::string& preamble,
                          ShaderTransformContext context) {
 std::string prepared = normalizePackSource(source, preamble);
 prepared = canonicalizeCoreSource(programName, stage, pack, std::move(prepared), context);
 return mergeColorWheelMaterial(programName, stage, std::move(prepared));
}
}
