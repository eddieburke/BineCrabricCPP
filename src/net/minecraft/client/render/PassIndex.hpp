#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/Pack.hpp"
namespace net::minecraft::client::render {
struct PackPassBuckets {
 std::vector<std::size_t> postPasses;
 std::vector<std::size_t> deferredPasses;
 std::vector<std::size_t> computePasses;
 std::vector<std::size_t> beginPasses;
 std::vector<std::size_t> shadowCompositePasses;
 std::vector<std::size_t> preparePasses;
 std::vector<std::size_t> setupPasses;
};
[[nodiscard]] bool isProgramEnabled(const PackDefinition& definition,
                                    const std::unordered_map<std::string, std::string>& settings,
                                    const std::string& programName);
using ProgramEnabledCache = std::unordered_map<std::string, bool>;
[[nodiscard]] bool isProgramEnabledCached(const PackDefinition& definition,
                                          const std::unordered_map<std::string, std::string>& settings,
                                          const std::string& programName,
                                          ProgramEnabledCache& cache);
void indexPackPasses(const PackDefinition& definition,
                       const std::unordered_map<std::string, std::string>& settings,
                       PackPassBuckets& buckets);

// https://shaders.properties/current/reference/programs/shadow/
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
// clrwl_* returns empty (ColorWheel shadow programs are separate).
[[nodiscard]] std::string irisShadowProgramForGbuffers(const std::string& gbuffersKey);
[[nodiscard]] std::string resolveIrisShadowProgramKey(
    const std::string& gbuffersKey, const std::unordered_map<std::string, PackProgramSource>& programs);
}
