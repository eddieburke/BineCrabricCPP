#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/GlslSource.hpp"
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"
#include "net/minecraft/client/render/shaders/ShaderTransform.hpp"
namespace net::minecraft::client::render {
// One engine-defined macro. `value` empty means a bare flag (`#define NAME`).
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
// Every macro the engine defines for `pack`, as data. versionPreambleForStages()
// renders it to `#define` text for the GLSL compiler; seedEngineMacros() loads it
// straight into the preprocessor table. Neither restates the list.
[[nodiscard]] std::vector<ShaderMacro> engineMacros(const PackDefinition& pack);
// https://shaders.properties/current/reference/macros/iris_version/
// https://shaders.properties/current/reference/macros/mc_version/
// https://github.com/IrisShaders/Iris/blob/37c02037/common/src/main/java/net/irisshaders/iris/gl/shader/StandardMacros.java
[[nodiscard]] std::string formatVersion122(std::string_view semver);
std::string versionPreamble(const PackDefinition& pack, const std::string& source, bool compute = false);
std::string versionPreambleForStages(const PackDefinition& pack,
                                     const std::vector<std::string_view>& sources,
                                     int minimumVersion = 330);
// THE engine macro environment for `pack`: every macro the engine defines, plus the
// real extension macros the GLSL compiler will define for itself once the pack's
// `#extension` lines are honoured. Every stage that evaluates a pack `#if` — const
// scanning, source normalization, and the compiled preamble — must seed from this and
// only this.
//
// It exists because there used to be two hand-maintained descriptions of the same
// environment: versionPreambleForStages() emitting the queried GL version and
// featureSupported()-gated IRIS_FEATURE_* into the shader, and a block of literals in
// seedMacrosFromDefines() asserting MC_GL_VERSION 460, every IRIS_FEATURE on, and two
// ARB extensions present regardless of the driver. Where they disagreed, a pack's
// `#if MC_GL_VERSION >= n` selected one branch while its constants were scanned under
// another — so the engine's idea of shadowDistance/shadowNearPlane could come from a
// branch the GPU never compiled.
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
