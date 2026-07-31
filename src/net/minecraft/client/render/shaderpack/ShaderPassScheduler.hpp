#pragma once
// Pass indexing lives in PassIndex; keep ShaderPassBuckets / indexShaderPasses names.
#include "net/minecraft/client/render/PassIndex.hpp"

namespace net::minecraft::client::render::shaderpack {
using ShaderPassBuckets = render::PackPassBuckets;

inline bool isProgramEnabled(const ShaderPackDefinition& definition,
                             const std::unordered_map<std::string, std::string>& settings,
                             const std::string& programName) {
 return render::isProgramEnabled(definition, settings, programName);
}

inline void indexShaderPasses(const ShaderPackDefinition& definition,
                              const std::unordered_map<std::string, std::string>& settings,
                              ShaderPassBuckets& buckets) {
 render::indexPackPasses(definition, settings, buckets);
}
} // namespace net::minecraft::client::render::shaderpack
