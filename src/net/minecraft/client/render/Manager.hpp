#pragma once
#include <atomic>
#include <filesystem>
#include <chrono>
#include <memory>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>
#include "net/minecraft/client/render/Pipeline.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackInstance.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::render {
enum class ColorFormat;
}
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::render::shaderpack {
// Lifecycle shell: pack discovery/settings/watcher. Frame GL path is render::Pipeline.
using WorldPipelinePhase = render::WorldPipelinePhase;
class ShaderPackManager {
 public:
 ShaderPackManager(std::filesystem::path gameDirectory, option::GameOptions* options);
 ~ShaderPackManager();
 void reload();
 void poll();
 bool select(const std::string& key);
 void reloadWorldMeshes();
 [[nodiscard]] bool active() const noexcept {
  return true;
 }
 [[nodiscard]] bool usingUserPack() const noexcept {
  return activeIndex_ < packs_.size();
 }
 bool setSetting(const std::string& key, std::string value);
 [[nodiscard]] std::string settingValue(const std::string& key) const;
 [[nodiscard]] const std::vector<ShaderPackSummary>& available() const noexcept {
  return summaries_;
 }
 [[nodiscard]] const ShaderPackDefinition* activeDefinition() const noexcept;
 [[nodiscard]] const ShaderPackDefinition* meshDefinition() const noexcept;
 [[nodiscard]] bool activeHasPostProcess() const;
 [[nodiscard]] bool hasDeferredPasses() const;
 [[nodiscard]] int shadowMapResolution() const;
 [[nodiscard]] int shadowColorBuffers() const;
 [[nodiscard]] std::vector<render::ColorFormat> sceneColorFormats() const;
 [[nodiscard]] bool ensureSceneTargets(int width, int height);
 void bindScene();
 void endScene();
 void destroyScene();
 [[nodiscard]] int sceneColorCount() const;
 [[nodiscard]] unsigned int sceneDepthTexture() const;
 void clearScene(float fogR = 0.0f, float fogG = 0.0f, float fogB = 0.0f);
 void resetPresentState();
 void setPipelinePhase(WorldPipelinePhase phase) noexcept {
  pipeline_.setPipelinePhase(phase);
 }
 [[nodiscard]] WorldPipelinePhase pipelinePhase() const noexcept {
  return pipeline_.pipelinePhase();
 }
 [[nodiscard]] bool interfaceProgramsActive() const noexcept {
  return pipeline_.interfaceProgramsActive();
 }
 gl::ShaderProgram* worldProgram(const std::string& key);
 class PipelinePhaseScope {
public:
  PipelinePhaseScope(ShaderPackManager* manager, WorldPipelinePhase phase)
      : manager_(manager), previous_(manager != nullptr ? manager->pipelinePhase() : WorldPipelinePhase::None) {
   if(manager_ != nullptr) {
    manager_->setPipelinePhase(phase);
   }
  }
  ~PipelinePhaseScope() {
   if(manager_ != nullptr) {
    manager_->setPipelinePhase(previous_);
   }
  }
  PipelinePhaseScope(const PipelinePhaseScope&) = delete;
  PipelinePhaseScope& operator=(const PipelinePhaseScope&) = delete;

private:
  ShaderPackManager* manager_ = nullptr;
  WorldPipelinePhase previous_ = WorldPipelinePhase::None;
 };
 void prepareFrame(net::minecraft::World* world);
 void refreshLightmap(net::minecraft::World* world);
 void setFrameUniforms(const FrameUniformSet& frame);
 bool renderBegin();
 bool renderPreWorld(int shadowDepthTextureId,
                     int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds,
                     int shadowColorTextureCount);
 bool renderDeferred(int shadowDepthTextureId,
                     int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds,
                     int shadowColorTextureCount);
 bool renderPostProcess(int shadowDepthTextureId,
                        int shadowOpaqueDepthTextureId,
                        const int* shadowColorTextureIds,
                        int shadowColorTextureCount);
 void sampleCenterDepth();
 void captureOpaqueDepth();
 void captureHandDepth();

 private:
 void logOnce(ShaderPackInstance& pack, const std::string& message) const;
 [[nodiscard]] ShaderPackInstance* activePack() noexcept;
 [[nodiscard]] const ShaderPackInstance* activePack() const noexcept;
 std::unique_ptr<ShaderPackInstance> loadDirectoryPack(const std::filesystem::path& path);
 void addDirectoryPack(const std::filesystem::path& path);
 void addZipPack(const std::filesystem::path& path);
 void refreshSummaries();
 void warmBasePrograms();
 void startDirectoryWatcher();
 void stopDirectoryWatcher();
 void directoryWatchLoop(const std::stop_token& stop);
 std::filesystem::path gameDirectory_;
 option::GameOptions* options_ = nullptr;
 render::Pipeline pipeline_;
 std::vector<std::unique_ptr<ShaderPackInstance>> packs_;
 std::unique_ptr<ShaderPackInstance> basePack_;
 std::vector<ShaderPackSummary> summaries_;
 static constexpr std::size_t kNoActivePack = static_cast<std::size_t>(-1);
 std::size_t activeIndex_ = kNoActivePack;
 std::uint64_t packDirectoryStamp_ = 0;
 std::atomic<std::uint64_t> watchedStamp_{0};
 std::atomic<bool> directoryChanged_{false};
 std::jthread directoryWatcher_;
};
} // namespace net::minecraft::client::render::shaderpack
