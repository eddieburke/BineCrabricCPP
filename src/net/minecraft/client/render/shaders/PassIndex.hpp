#pragma once
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
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
struct PackProgramId {
 std::string_view name;
 std::string_view fallback;
};
[[nodiscard]] std::span<const PackProgramId> packProgramIds();
[[nodiscard]] bool isProgramEnabled(const PackDefinition& definition,
                                    const std::unordered_map<std::string, std::string>& settings,
                                    const std::string& programName);
using ProgramEnabledCache = std::unordered_map<std::string, bool>;
[[nodiscard]] bool isProgramEnabledCached(const PackDefinition& definition,
                                          const std::unordered_map<std::string, std::string>& settings,
                                          const std::string& programName,
                                          ProgramEnabledCache& cache);
// Caller supplies the enabled-state cache: call sites use the instance's
// programEnabledCache (cleared before re-indexing), so bucket indexing and the
// per-frame program resolution share ONE cache instead of two.
void indexPackPasses(const PackDefinition& definition,
                     const std::unordered_map<std::string, std::string>& settings,
                     PackPassBuckets& buckets,
                     ProgramEnabledCache& cache);

// https://shaders.properties/current/reference/programs/shadow/
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisPipelines.java
// gbuffers key -> shadow ProgramId (IrisPipelines coreShaderMapShadow); empty means
// the render type has no shadow mapping in Iris (sky/clouds/hand/gui) or is clrwl_*.
[[nodiscard]] std::string irisShadowProgramForGbuffers(const std::string& gbuffersKey);
[[nodiscard]] std::string programFallbackKey(const std::string& programKey);
[[nodiscard]] std::string resolveProgramKey(const PackDefinition& definition,
                                            const std::unordered_map<std::string, std::string>& settings,
                                            const std::string& requested,
                                            ProgramEnabledCache& cache);
}
