#pragma once
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace net::minecraft::client::render {
inline constexpr std::array<std::string_view, 6> kCompositeStagePrefixes = {
    "begin", "shadowcomp", "prepare", "deferred", "composite", "final"};
[[nodiscard]] bool isCompositeStageName(const std::string& name) noexcept;
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
void indexPackPasses(const PackDefinition& definition,
                     const std::unordered_map<std::string, std::string>& settings,
                     PackPassBuckets& buckets,
                     ProgramEnabledCache& cache);
[[nodiscard]] std::string irisShadowProgramForGbuffers(const std::string& gbuffersKey);
[[nodiscard]] std::string programFallbackKey(const std::string& programKey);
[[nodiscard]] std::string resolveProgramKey(const PackDefinition& definition,
                                            const std::unordered_map<std::string, std::string>& settings,
                                            const std::string& requested,
                                            ProgramEnabledCache& cache);
} // namespace net::minecraft::client::render
