#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/GlslSource.hpp"
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"
#include "net/minecraft/client/render/shaders/ShaderTransform.hpp"
namespace net::minecraft::client::render {
struct ShaderMacro {
 std::string name;
 std::string value;
};
void captureGlShaderSnapshot();
int glVersionMacro();
int glslVersionMacro();
int maxColorBuffers();
std::string vendorMacroName();
std::string rendererMacroName();
std::vector<std::string> glShaderExtensions();
[[nodiscard]] std::vector<ShaderMacro> engineMacros(const PackDefinition& pack);
// https://shaders.properties/current/reference/macros/iris_version/
// https://shaders.properties/current/reference/macros/mc_version/
// https://github.com/IrisShaders/Iris/blob/37c02037/common/src/main/java/net/irisshaders/iris/gl/shader/StandardMacros.java
[[nodiscard]] std::string formatVersion122(std::string_view semver);
std::string versionPreamble(const PackDefinition& pack, const std::string& source, bool compute = false);
std::string versionPreambleForStages(const PackDefinition& pack,
                                     const std::vector<std::string_view>& sources,
                                     int minimumVersion = 330);
void seedEngineMacros(const PackDefinition& pack, PPMacroTable& macros);
std::string normalizePackSource(const PackDefinition& pack, const std::string& source);
[[nodiscard]] std::string prepareSource(const std::string& programName,
                                        ShaderStage stage,
                                        const PackDefinition& pack,
                                        const std::string& source,
                                        ShaderTransformContext context = {});
[[nodiscard]] std::string defaultCompositeVertexShader();
[[nodiscard]] std::string defaultRasterVertexShader();
} // namespace net::minecraft::client::render
