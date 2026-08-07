#pragma once
#include <functional>
#include <string>
#include <unordered_map>
namespace net::minecraft::client::gl {
class ShaderProgram;
class GlTexture;
}
namespace net::minecraft::client::render {
class PackInstance;
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ShaderProperties.java
inline constexpr int kMaxShaderStorageBuffers = 13;
bool ensurePackResources(PackInstance& pack, int width, int height, const gl::GlTexture& lightmapTexture,
                         const std::function<std::string(const PackInstance&, const std::string&)>& readText);
void bindPackResources(PackInstance& pack, gl::ShaderProgram& program, unsigned int imageUnitStart = 0);
void addPackTextures(PackInstance& pack, const std::string& stage,
                     std::unordered_map<std::string, int>& textures,
                     std::unordered_map<std::string, int>& volumes);
} // namespace net::minecraft::client::render
