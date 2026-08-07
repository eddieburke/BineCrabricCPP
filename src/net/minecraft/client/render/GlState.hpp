#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
enum class ColorFormat;
class ColorTargets;
constexpr unsigned int kTexture3D = 0x806F;
// https://shaders.properties/current/reference/buffers/shadowtex/
// https://shaders.properties/current/guides/your-first-shaderpack/4_shadows/
unsigned int samplerObject(bool compare, bool linear = true, bool mipmap = false);
void shadowSampleMode(std::string_view name, bool sampler2DShadow, const PackDefinition& definition,
                      bool& compare, bool& linear, bool& mipmap);
void bindSamplers(gl::ShaderProgram& program,
                  const std::unordered_map<std::string, int>& textures,
                  const std::unordered_map<std::string, int>& volumeTextures,
                  int maxUnits,
                  const PackDefinition& definition);
void releaseSamplers(int maxUnits);
// https://shaders.properties/current/reference/buffers/shadowtex/
void refreshTextureAliases(std::unordered_map<std::string, int>& textures,
                           bool waterShadowPresent = false);
// Fills shadowtex0/1 (+ HW), shadowcolorN, and waterShadow / shadow aliases.
void putShadowTextures(std::unordered_map<std::string, int>& textures,
                       int shadowtex0,
                       int shadowtex1,
                       const int* shadowColorTextures,
                       int shadowColorCount,
                       const PackDefinition& definition);
[[nodiscard]] unsigned int bindColorImages(gl::ShaderProgram& program,
                                           const std::unordered_map<std::string, int>& colorTextures,
                                           const PackDefinition& definition,
                                           const ColorTargets* colorTargets = nullptr);
bool featureSupported(const std::string& feature);
[[nodiscard]] bool featureEnabled(const PackDefinition& pack, const std::string& feature);
int maxTextureUnits();
ColorFormat parseFormat(const std::string& format);
[[nodiscard]] const char* colorFormatName(ColorFormat format);
// https://shaders.properties/current/reference/constants/buffer_format/
unsigned int pixelFormat(std::string value);
unsigned int pixelType(std::string value);
unsigned int internalFormat(std::string value);
unsigned int internalFormat(ColorFormat format);
unsigned int textureTarget(std::string value, std::size_t dimensions);
unsigned int blendFactor(std::string value);
void applyBufferBlends(const PackDefinition& pack, const std::string& program,
                       const std::vector<int>& rendertargets);
[[nodiscard]] int colortexToDrawBufferIndex(const std::vector<int>& rendertargets, int colortexIndex);
void applyAlphaTest(const PackDefinition& pack, const std::string& program);
bool normalizeSettingValue(const PackSetting& setting, const std::string& input, std::string& output);
bool hasGlContext();
std::vector<std::string> supportedGlExtensions();
} // namespace net::minecraft::client::render
