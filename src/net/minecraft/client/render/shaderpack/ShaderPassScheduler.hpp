#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
namespace net::minecraft::client::render::shaderpack {
struct ShaderPassBuckets {
 std::vector<std::size_t> postPasses;
 std::vector<std::size_t> deferredPasses;
 std::vector<std::size_t> computePasses;
 std::vector<std::size_t> beginPasses;
 std::vector<std::size_t> shadowCompositePasses;
 std::vector<std::size_t> preparePasses;
 std::vector<std::size_t> setupPasses;
};
[[nodiscard]] bool isProgramEnabled(const ShaderPackDefinition& definition,
                                    const std::unordered_map<std::string, std::string>& settings,
                                    const std::string& programName);
void indexShaderPasses(const ShaderPackDefinition& definition,
                       const std::unordered_map<std::string, std::string>& settings,
                       ShaderPassBuckets& buckets);
} // namespace net::minecraft::client::render::shaderpack
