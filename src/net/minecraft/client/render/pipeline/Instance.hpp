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
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
namespace net::minecraft::client::resource::pack {
class ZippedTexturePack;
}
namespace net::minecraft::client::render {
struct PackSummary {
 std::string key;
 std::string name;
 std::string error;
 bool valid = false;
 bool selected = false;
};
enum class PackProgramState { Cold,
                              Submitted,
                              Ready,
                              Failed };
class PackInstance {
 public:
 ~PackInstance();
 void clearGpuResources();
 bool rebuildRuntime(std::string& error);
 void resetPrograms();
 PackSummary summary;
 std::filesystem::path path;
 bool directory = false;
 std::unique_ptr<net::minecraft::client::resource::pack::ZippedTexturePack> zip;
 PackDefinition definition;
 PackDefinition rootDefinition;
 std::string dimensionKey;
 std::unordered_map<std::string, PackSourceOption> sourceOptions;
 std::unordered_map<std::string, std::string> settings;
 CustomUniformRuntime customUniforms;
 std::unordered_map<std::string, std::string> sourceCache;
 std::unordered_map<std::string, std::string> resolvedSourceCache;
 std::unordered_map<std::string, gl::ShaderProgram*> compiledPrograms;
 // What prewarm already worked out for each program, so the pass that links the
 // finished binaries does not have to re-run source preparation to rediscover it.
 // program name -> ProgramCache key.
 std::unordered_map<std::string, std::string> programCacheKeys;
 // ProgramCache key -> draw buffer indices parsed out of the prepared fragment.
 std::unordered_map<std::string, std::vector<int>> programDrawBuffers;
 PackProgramState programState = PackProgramState::Cold;
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
  gl::GlTexture texture;
  int width = 0;
  int height = 0;
  int depth = 0;
 };
 std::unordered_map<std::string, ImageTarget> images;
 std::unordered_map<std::string, unsigned int> customTextures;
 std::set<unsigned int> ownedCustomTextures;
 std::array<gl::GlBuffer, kMaxShaderStorageBuffers> bufferObjects;
 std::size_t bufferBytes[kMaxShaderStorageBuffers]{};
 int setupWidth = 0;
 int setupHeight = 0;
 gl::GlTexture noiseTexture;
 int noiseResolution = 0;
 std::array<gl::GlTexture, 2> depthTextures;
 int depthTextureW[2]{};
 int depthTextureH[2]{};
 std::set<std::string> logged;
 std::unordered_map<std::string, bool> programEnabledCache;
};
} // namespace net::minecraft::client::render
