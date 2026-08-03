#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/GlslSource.hpp"
#include "net/minecraft/client/render/shaders/ShaderTransform.hpp"

namespace net::minecraft::client::render {

void captureGlShaderSnapshot();
int glVersionMacro();
int glslVersionMacro();
int maxColorBuffers();
std::string driverPreamble();
std::vector<std::string> glShaderExtensions();
// https://shaders.properties/current/reference/macros/iris_version/
// https://shaders.properties/current/reference/macros/mc_version/
// https://github.com/IrisShaders/Iris/blob/37c02037/common/src/main/java/net/irisshaders/iris/gl/shader/StandardMacros.java
[[nodiscard]] std::string formatVersion122(std::string_view semver);
std::string versionPreamble(const PackDefinition& pack, const std::string& source, bool compute = false);
std::string versionPreambleForStages(const PackDefinition& pack,
                                     const std::vector<std::string_view>& sources,
                                     int minimumVersion = 330);
std::string normalizePackSource(const std::string& source, const std::string& preamble);

[[nodiscard]] std::string prepareSource(const std::string& programName,
                                        ShaderStage stage,
                                        const PackDefinition& pack,
                                        const std::string& source,
                                        const std::string& preamble,
                                        ShaderTransformContext context = {});

[[nodiscard]] bool isCompositeStyleProgramName(const std::string& programName);
[[nodiscard]] std::string defaultCompositeVertexShader();
[[nodiscard]] std::string defaultRasterVertexShader();

}
