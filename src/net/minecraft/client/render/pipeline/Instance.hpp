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
#include "net/minecraft/client/render/shaders/WorldProgramId.hpp"
namespace net::minecraft::client::resource::pack {
class ZippedTexturePack;
}
namespace net::minecraft::client::render {
enum class CompositeStage : std::uint8_t {
 Begin,
 ShadowComposite,
 Prepare,
 Deferred,
 Composite,
 Count
};
struct PackSummary {
 std::string key;
 std::string name;
 std::string error;
 bool valid = false;
 bool selected = false;
};
struct PackInstance {
 public:
 struct RuntimePass {
  std::size_t passIndex = 0;
  gl::ShaderProgram* program = nullptr;
 };
 struct RuntimeOperation {
  RuntimePass pass;
  bool compute = false;
  bool groupBegin = false;
  bool groupEnd = false;
 };
 struct WorldProgramRuntime {
  gl::ShaderProgram* program = nullptr;
  std::string resolvedKey;
 };
 ~PackInstance();
 void clearGpuResources();
 bool rebuildRuntime(std::string& error);
 bool buildExecutionPlan(std::string& error);
 void resetPrograms();
 [[nodiscard]] const std::vector<RuntimeOperation>& stagePlan(CompositeStage stage) const noexcept;
 PackSummary summary;
 std::filesystem::path path;
 bool directory = false;
 bool embedded = false;
 std::unique_ptr<net::minecraft::client::resource::pack::ZippedTexturePack> zip;
 std::vector<std::string> resources;
 PackDefinition definition;
 PackDefinition rootDefinition;
 std::string dimensionKey;
 std::unordered_map<std::string, PackSourceOption> sourceOptions;
 std::unordered_map<std::string, std::string> settings;
 CustomUniformRuntime customUniforms;
 std::unordered_map<std::string, std::string> sourceCache;
 std::unordered_map<std::string, std::unordered_map<std::string, std::string>> includedSourceCache;
 std::unordered_map<std::string, std::string> preparedSourceCache;
 struct CompiledEntry {
  gl::ShaderProgram* program = nullptr;
  bool failed = false;
 };
 std::unordered_map<std::string, CompiledEntry> compiledPrograms;
 std::unordered_map<std::string, std::vector<int>> programDrawBuffers;
 std::vector<std::size_t> postPasses;
 std::vector<std::size_t> deferredPasses;
 std::vector<std::size_t> computePasses;
 std::vector<std::size_t> beginPasses;
 std::vector<std::size_t> shadowCompositePasses;
 std::vector<std::size_t> preparePasses;
 std::vector<std::size_t> setupPasses;
 std::array<std::vector<RuntimeOperation>, static_cast<std::size_t>(CompositeStage::Count)> stagePlans;
 std::vector<RuntimePass> setupPlan;
 std::array<std::array<WorldProgramRuntime, 2>, static_cast<std::size_t>(WorldProgramId::Count)> worldPrograms;
 bool executionPlanReady = false;
 // A pass whose program failed to compile disables only the stage it belongs to.
 // One bad .csh must not take the rest of the pack's stages down with it.
 std::array<bool, static_cast<std::size_t>(CompositeStage::Count)> stagePlanValid{};
 std::unique_ptr<gl::ProgramCache> programs;
 std::shared_ptr<gl::ShaderBinaryCache> shaderBinaryCache;
 std::filesystem::path shaderCacheDirectory;
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
 std::unordered_map<std::string, int> worldTextures;
 std::unordered_map<std::string, int> worldVolumeTextures;
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
 [[nodiscard]] int opaqueDepthTexture(int fallback) const noexcept {
  return depthTextures[1] ? static_cast<int>(depthTextures[1].handle()) : fallback;
 }
 [[nodiscard]] int handDepthTexture(int fallback) const noexcept {
  return depthTextures[0] ? static_cast<int>(depthTextures[0].handle()) : opaqueDepthTexture(fallback);
 }
 std::set<std::string> logged;
 std::unordered_map<std::string, bool> programEnabledCache;
};
} // namespace net::minecraft::client::render
