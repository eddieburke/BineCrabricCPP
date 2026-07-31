#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackLoader.hpp"
namespace net::minecraft::client::resource::pack {
class ZippedTexturePack;
}
namespace net::minecraft::client::render::shaderpack {
struct ShaderPackSummary {
 std::string key;
 std::string name;
 std::string version;
 std::string error;
 bool valid = false;
 bool selected = false;
};
class ShaderPackInstance {
 public:
 ~ShaderPackInstance();
 void clearGpuResources();
 ShaderPackSummary summary;
 std::filesystem::path path;
 bool directory = false;
 std::unique_ptr<net::minecraft::client::resource::pack::ZippedTexturePack> zip;
 ShaderPackDefinition definition;
 ShaderPackDefinition rootDefinition;
 std::string dimensionKey;
 std::unordered_map<std::string, ShaderSourceOption> sourceOptions;
 std::unordered_map<std::string, std::string> settings;
 CustomUniformRuntime customUniforms;
 std::unordered_map<std::string, std::string> sourceCache;
 std::unordered_map<std::string, gl::ShaderProgram*> compiledPrograms;
 std::vector<std::size_t> postPasses;
 std::vector<std::size_t> deferredPasses;
 std::vector<std::size_t> computePasses;
 std::vector<std::size_t> beginPasses;
 std::vector<std::size_t> shadowCompositePasses;
 std::vector<std::size_t> preparePasses;
 std::vector<std::size_t> setupPasses;
 std::unique_ptr<gl::ProgramCache> programs;
 render::ColorTargets colorTargets;
 std::unordered_map<std::string, int> publishedTextures;
 struct ImageTarget {
  unsigned int texture = 0;
  int width = 0;
  int height = 0;
  int depth = 0;
 };
 std::unordered_map<std::string, ImageTarget> images;
 std::unordered_map<std::string, unsigned int> customTextures;
 std::set<unsigned int> ownedCustomTextures;
 unsigned int bufferObjects[9]{};
 std::size_t bufferBytes[9]{};
 int setupWidth = 0;
 int setupHeight = 0;
 unsigned int noiseTexture = 0;
 int noiseResolution = 0;
 unsigned int depthTextures[2]{};
 // Allocated size of depthTextures[] — used so deferred/composite can
 // glCopyTexSubImage2D instead of reallocating with glCopyTexImage2D each frame.
 int depthTextureW[2]{};
 int depthTextureH[2]{};
 std::set<std::string> logged;
};
} // namespace net::minecraft::client::render::shaderpack
