#pragma once
#include <atomic>
#include <filesystem>
#include <chrono>
#include <memory>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft::client::gl {
class ShaderCompileService;
class ShaderProgram;
}
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::render {
enum class ColorFormat;
namespace shadowmap {
struct ShadowTargets;
}
} // namespace net::minecraft::client::render
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::render {
class PackManager {
 public:
 PackManager(std::filesystem::path gameDirectory, option::GameOptions* options, gl::ShaderCompileService& compiler);
 ~PackManager();
 void reload();
 void poll();
 void advancePackActivation();
 bool select(const std::string& key);
 void reloadWorldMeshes();
 [[nodiscard]] bool usingUserPack() const noexcept {
  return activeIndex_ < packs_.size();
 }
 render::Pipeline& pipeline() noexcept {
  return pipeline_;
 }
 bool setSetting(const std::string& key, std::string value);
 bool setSettings(const std::vector<std::pair<std::string, std::string>>& values);
 [[nodiscard]] std::string settingValue(const std::string& key) const;
 [[nodiscard]] const std::vector<PackSummary>& available() const noexcept {
  return summaries_;
 }
 [[nodiscard]] const PackDefinition& activeDefinition() const noexcept;
 [[nodiscard]] const PackDefinition& meshDefinition() const noexcept;
 [[nodiscard]] bool hasActivePack() const noexcept;
 [[nodiscard]] const PackDefinition* selectedDefinition() const noexcept;
 [[nodiscard]] bool hasDeferredPasses() const;
 [[nodiscard]] bool ensureSceneTargets(int width, int height);
 void bindScene();
 void endScene();
 [[nodiscard]] int sceneColorCount() const;
 void clearScene(float fogR = 0.0f, float fogG = 0.0f, float fogB = 0.0f);
 gl::ShaderProgram* worldProgram(std::string_view key);
 class PipelinePhaseScope {
public:
  PipelinePhaseScope(PackManager* manager, WorldPipelinePhase phase)
      : manager_(manager), previous_(manager != nullptr ? manager->pipeline().pipelinePhase()
                                                        : WorldPipelinePhase::None) {
   if(manager_ != nullptr) {
    manager_->pipeline().setPipelinePhase(phase);
   }
  }
  ~PipelinePhaseScope() {
   if(manager_ != nullptr) {
    manager_->pipeline().setPipelinePhase(previous_);
   }
  }
  PipelinePhaseScope(const PipelinePhaseScope&) = delete;
  PipelinePhaseScope& operator=(const PipelinePhaseScope&) = delete;

private:
  PackManager* manager_ = nullptr;
  WorldPipelinePhase previous_ = WorldPipelinePhase::None;
 };
 void prepareFrame(net::minecraft::World* world);
 void setFrameUniforms(const PackUniformValues& frame);
 bool renderBegin(int shadowDepthTextureId,
                  int shadowOpaqueDepthTextureId,
                  const int* shadowColorTextureIds,
                  int shadowColorTextureCount,
                  shadowmap::ShadowTargets* shadowTargets,
                  const int* shadowColorAltTextureIds);
 bool renderShadowComposite(int shadowDepthTextureId,
                            int shadowOpaqueDepthTextureId,
                            const int* shadowColorTextureIds,
                            int shadowColorTextureCount,
                            shadowmap::ShadowTargets* shadowTargets,
                            const int* shadowColorAltTextureIds);
 bool renderPreWorld(int shadowDepthTextureId,
                     int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds,
                     int shadowColorTextureCount,
                     shadowmap::ShadowTargets* shadowTargets,
                     const int* shadowColorAltTextureIds);
 bool renderDeferred(int shadowDepthTextureId,
                     int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds,
                     int shadowColorTextureCount,
                     const int* shadowColorAltTextureIds);
 bool renderPostProcess(int shadowDepthTextureId,
                        int shadowOpaqueDepthTextureId,
                        const int* shadowColorTextureIds,
                        int shadowColorTextureCount,
                        const int* shadowColorAltTextureIds);
 void sampleCenterDepth();
 void captureOpaqueDepth();
 void captureHandDepth();

  private:
  [[nodiscard]] WorldProgramBindContext makeWorldBindContext();
  void logOnce(PackInstance& pack, const std::string& message) const;
 [[nodiscard]] PackCompiler::LogFnLevel logFn();
 [[nodiscard]] PackInstance* activePack() noexcept;
 [[nodiscard]] const PackInstance* activePack() const noexcept;
 [[nodiscard]] PackInstance* renderPack() noexcept;
 [[nodiscard]] PackInstance* selectedPack() noexcept;
 [[nodiscard]] const PackInstance* selectedPack() const noexcept;
 void preparePendingPack(net::minecraft::World* world);
 [[nodiscard]] bool packReady(PackInstance& pack);
  void prepareStagedPack(net::minecraft::World* world);
  void commitStagedPack();
  void discardStagedPack();
  [[nodiscard]] std::unique_ptr<PackInstance> clonePack(const PackInstance& source,
                                                        const std::unordered_map<std::string, std::string>* settings = nullptr);
  void activatePack(std::size_t index);
  void cancelPendingPack();
  [[nodiscard]] std::unique_ptr<PackInstance> loadPack(const std::filesystem::path& path,
                                                       bool directory);
  [[nodiscard]] std::unique_ptr<PackInstance> loadEmbeddedVanillaPack();
  void initializePackRuntime(PackInstance& pack);
  void refreshSummaries();
  void warmBasePrograms();
  void prewarmPacks();
  void startDirectoryWatcher();
  void stopDirectoryWatcher();
  void directoryWatchLoop(const std::stop_token& stop);
  std::filesystem::path gameDirectory_;
 option::GameOptions* options_ = nullptr;
 gl::ShaderCompileService& compiler_;
 render::Pipeline pipeline_;
 std::vector<std::unique_ptr<PackInstance>> packs_;
 std::unique_ptr<PackInstance> basePack_;
 std::vector<PackSummary> summaries_;
 static constexpr std::size_t kNoActivePack = static_cast<std::size_t>(-1);
 std::size_t activeIndex_ = kNoActivePack;
 std::optional<std::size_t> pendingIndex_;
 std::unique_ptr<PackInstance> stagedPack_;
 std::size_t stagedIndex_ = kNoActivePack;
 std::uint64_t packDirectoryStamp_ = 0;
 std::atomic<std::uint64_t> watchedStamp_{0};
 std::atomic<bool> directoryChanged_{false};
 std::jthread directoryWatcher_;
};
} // namespace net::minecraft::client::render
