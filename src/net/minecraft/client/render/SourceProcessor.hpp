#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/client/render/Pack.hpp"

namespace net::minecraft::client::render::glutil {
enum class ShaderStage { Vertex, Fragment, Other };

int glVersionMacro();
int maxColorBuffers();
std::string driverPreamble();
// Iris StandardMacros.formatVersionString — 122 encode (1 major, 2 minor, 2 release).
// Accepts arbitrary semver-shaped input (suffixes/junk ignored), same as Iris getFormattedIrisVersion.
// https://shaders.properties/current/reference/macros/iris_version/
// https://shaders.properties/current/reference/macros/mc_version/
// https://github.com/IrisShaders/Iris/blob/37c02037/common/src/main/java/net/irisshaders/iris/gl/shader/StandardMacros.java
[[nodiscard]] std::string formatVersion122(std::string_view semver);
std::string versionPreamble(const PackDefinition& pack, const std::string& source, bool compute = false);
std::string normalizePackSource(const std::string& source, const std::string& preamble);

[[nodiscard]] std::string prepareSource(const std::string& programName,
                                        ShaderStage stage,
                                        const PackDefinition& pack,
                                        const std::string& source,
                                        const std::string& preamble);

[[nodiscard]] bool isCompositeStyleProgramName(const std::string& programName);
[[nodiscard]] const char* defaultCompositeVertexShader();

std::vector<bool> codeMask(const std::string& source);
bool tokenAt(const std::string& source,
             const std::vector<bool>& mask,
             std::size_t at,
             std::string_view token);
void replaceAllToken(std::string& source, std::string_view from, std::string_view to);
bool referencesToken(const std::string& source, std::string_view token);
bool hasStorageDeclaration(const std::string& source,
                           std::string_view storage,
                           std::string_view alternateStorage,
                           std::string_view name);

std::size_t sourceDeclarationOffset(const std::string& source);
bool appendBeforeMainClose(std::string& source, const std::string& snippet);
}
