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
// Iris reserves SSBO indices 0..12 ("SSBO's cannot use buffer numbers higher than 12, they're reserved!").
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ShaderProperties.java
inline constexpr int kMaxShaderStorageBuffers = 13;
class PackResources {
 public:
 static bool ensure(PackInstance& pack, int width, int height, const gl::GlTexture& lightmapTexture,
                    const std::function<std::string(const PackInstance&, const std::string&)>& readText);
 static void bind(PackInstance& pack, gl::ShaderProgram& program, unsigned int imageUnitStart = 0);
 static void addTextures(PackInstance& pack, const std::string& stage,
                         std::unordered_map<std::string, int>& textures,
                          std::unordered_map<std::string, int>& volumes);
};
} // namespace net::minecraft::client::render
