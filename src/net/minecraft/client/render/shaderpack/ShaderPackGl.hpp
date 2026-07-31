#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
enum class ColorFormat;
}
namespace net::minecraft::client::render::shaderpack {
struct ShaderPackDefinition;
struct PackSetting;
namespace glutil {
unsigned int samplerObject(bool compare);
void bindSamplers(gl::ShaderProgram& program,
                  const std::unordered_map<std::string, int>& textures,
                  const std::unordered_map<std::string, int>& volumeTextures,
                  int maxUnits);
void releaseSamplers(int maxUnits);
using ShaderReadText = std::function<std::string(std::string_view)>;
[[nodiscard]] std::string resolveShaderIncludes(const ShaderReadText& readText,
                                                const std::string& path,
                                                bool stripFormatDirectives = false);
bool isBufferFormatDirective(const std::string& trimmed);
void refreshTextureAliases(std::unordered_map<std::string, int>& textures);
[[nodiscard]] unsigned int bindColorImages(gl::ShaderProgram& program,
                                           const std::unordered_map<std::string, int>& colorTextures,
                                           const ShaderPackDefinition* definition = nullptr);
bool featureSupported(const std::string& feature);
int maxTextureUnits();
std::string versionPreamble(const ShaderPackDefinition& pack, const std::string& source, bool compute = false);
std::string normalizePackSource(const std::string& source, const std::string& preamble);
enum class ShaderStage { Vertex, Fragment, Other };
[[nodiscard]] std::string prepareSource(const std::string& programName,
                                        ShaderStage stage,
                                        const ShaderPackDefinition& pack,
                                        const std::string& source,
                                        const std::string& preamble);
[[nodiscard]] bool isCompositeStyleProgramName(const std::string& programName);
[[nodiscard]] const char* defaultCompositeVertexShader();
render::ColorFormat parseFormat(const std::string& format);
[[nodiscard]] std::vector<int> parseRenderTargetIndices(const std::string& source);
[[nodiscard]] std::vector<int> defaultRenderTargetIndices();
[[nodiscard]] std::vector<std::string> renderTargetOutputNames(const std::string& source);
unsigned int pixelFormat(std::string value);
unsigned int pixelType(std::string value);
unsigned int internalFormat(std::string value);
unsigned int textureTarget(std::string value, std::size_t dimensions);
unsigned int blendFactor(std::string value);
void applyBufferBlends(const ShaderPackDefinition& pack, const std::string& program);
void applyAlphaTest(const ShaderPackDefinition& pack, const std::string& program);
bool normalizeSettingValue(const PackSetting& setting, const std::string& input, std::string& output);
bool hasGlContext();
constexpr unsigned int kTexture2D = 0x0DE1;
constexpr unsigned int kTexture3D = 0x806F;
}
}
