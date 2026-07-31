#pragma once
#include <functional>
#include <string>
#include <unordered_map>
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render::shaderpack {
class ShaderPackInstance;
class ShaderPackResources {
 public:
 static bool ensure(ShaderPackInstance& pack, int width, int height, unsigned int lightmapTexture,
                    const std::function<std::string(const ShaderPackInstance&, const std::string&)>& readText);
 static void bind(ShaderPackInstance& pack, gl::ShaderProgram& program, unsigned int imageUnitStart = 0);
 static void addTextures(ShaderPackInstance& pack, const std::string& stage,
                         std::unordered_map<std::string, int>& textures,
                         std::unordered_map<std::string, int>& volumes);
};
} // namespace net::minecraft::client::render::shaderpack
