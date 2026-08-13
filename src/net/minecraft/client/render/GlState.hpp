#pragma once
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
inline constexpr std::array<std::string_view, 13> kIrisFeatureNames = {
    "SEPARATE_HARDWARE_SAMPLERS", "HIGHER_SHADOWCOLOR", "CUSTOM_IMAGES", "PER_BUFFER_BLENDING",
    "COMPUTE_SHADERS", "TESSELLATION_SHADERS", "ENTITY_TRANSLUCENT", "REVERSED_CULLING",
    "BLOCK_EMISSION_ATTRIBUTE", "CAN_DISABLE_WEATHER", "SSBO", "FADE_VARIABLE", "TEXTURE_FILTERING"};
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
int bindAvailableSamplers(gl::ShaderProgram& program,
                          const std::unordered_map<std::string, int>& textures,
                          const std::unordered_map<std::string, int>& volumeTextures,
                          int firstUnit,
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
bool featureSupported(std::string_view feature);
[[nodiscard]] bool featureEnabled(const PackDefinition& pack, std::string_view feature);
int maxTextureUnits();
unsigned int maxImageUnits();
ColorFormat parseFormat(const std::string& format);
[[nodiscard]] const char* colorFormatName(ColorFormat format);
[[nodiscard]] std::string_view canonicalFormatName(std::string_view format);
// https://shaders.properties/current/reference/constants/buffer_format/
unsigned int pixelFormat(std::string value);
unsigned int pixelType(std::string value);
unsigned int internalFormat(std::string value);
unsigned int internalFormat(ColorFormat format);
[[nodiscard]] bool integerInternalFormat(std::string value);
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
