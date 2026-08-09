#pragma once
#include <atomic>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GlFramebuffer.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/render/ColorSpace.hpp"
#include "net/minecraft/client/render/pipeline/AsyncDepthSampler.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::render {
enum class ColorFormat;
class ColorTargets;
enum class WorldPipelinePhase { None,
                                Shadow,
                                World };
namespace shadowmap {
struct ShadowTargets;
}
class Pipeline {
 public:
 explicit Pipeline(option::GameOptions* options);
 Pipeline(std::filesystem::path gameDirectory, option::GameOptions* options,
          std::filesystem::path shaderCacheDirectory);
 ~Pipeline();
 void reload();
 void poll();
 bool select(const std::string& key);
 void reloadWorldMeshes();
 [[nodiscard]] bool usingUserPack() const noexcept {
  return activeIndex_ < packs_.size();
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
 gl::ShaderProgram* worldProgram(WorldProgramId id);
 void applyWorldPassDirectives(WorldProgramId id, gl::ShaderProgram& program);
 void uploadWorldProgramUniforms(gl::ShaderProgram& program, WorldProgramId id);
 void bindWorldProgramMaterial(gl::ShaderProgram& program, WorldProgramId id);
 [[nodiscard]] int resolveObjectId(const std::string& kind, const std::string& name,
                                   int fallback) const;
 [[nodiscard]] std::uint64_t objectIdRevision() const noexcept {
  return objectIdRevision_;
 }
 class PhaseScope {
  public:
   PhaseScope(Pipeline* pipeline, WorldPipelinePhase phase)
       : pipeline_(pipeline), previous_(pipeline != nullptr ? pipeline->pipelinePhase()
                                                           : WorldPipelinePhase::None) {
    if(pipeline_ != nullptr) pipeline_->setPipelinePhase(phase);
   }
   ~PhaseScope() {
    if(pipeline_ != nullptr) pipeline_->setPipelinePhase(previous_);
   }
   PhaseScope(const PhaseScope&) = delete;
   PhaseScope& operator=(const PhaseScope&) = delete;

  private:
   Pipeline* pipeline_ = nullptr;
   WorldPipelinePhase previous_ = WorldPipelinePhase::None;
 };
 void prepareFrame(net::minecraft::World* world);
 void setFrameUniforms(const PackUniformValues& frame);
 bool renderBegin(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                  const int* shadowColorTextureIds, int shadowColorTextureCount,
                  shadowmap::ShadowTargets* shadowTargets,
                  const int* shadowColorAltTextureIds);
 bool renderShadowComposite(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                            const int* shadowColorTextureIds, int shadowColorTextureCount,
                            shadowmap::ShadowTargets* shadowTargets,
                            const int* shadowColorAltTextureIds);
 bool renderPreWorld(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds, int shadowColorTextureCount,
                     shadowmap::ShadowTargets* shadowTargets,
                     const int* shadowColorAltTextureIds);
 bool renderDeferred(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds, int shadowColorTextureCount,
                     const int* shadowColorAltTextureIds);
 bool renderPostProcess(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                        const int* shadowColorTextureIds, int shadowColorTextureCount,
                        const int* shadowColorAltTextureIds);
 void sampleCenterDepth();
 void captureOpaqueDepth();
 void captureHandDepth();
 void reset();
 void prepareFrame(net::minecraft::World* world, PackInstance* activePack, PackInstance* basePack);
 [[nodiscard]] std::string dimensionKey(const PackInstance& pack,
                                        const net::minecraft::World* world) const;
 bool selectDimension(PackInstance& pack, const net::minecraft::World* world, bool applyIds);
 bool preparePackResources(PackInstance& pack, int width, int height);
 void refreshLightmap(net::minecraft::World* world);
 void setFrameUniforms(const PackUniformValues& frame, const PackDefinition& activeDef, PackInstance* activePack);
 void updateLightmap(const net::minecraft::World* world);
 void ensurePbrFallbackTextures();
 void bindPbrTextures();
 void refreshResourcePackState(PackInstance* basePack, const std::vector<std::unique_ptr<PackInstance>>& packs);
 void applyBlockIds(const PackDefinition& definition);
 [[nodiscard]] bool hasDeferredPasses(const PackInstance* activePack) const;
 [[nodiscard]] std::vector<ColorFormat> sceneColorFormats(const PackInstance* activePack) const;
 [[nodiscard]] bool ensureSceneTargets(PackInstance* activePack, int width, int height);
 void bindScene(PackInstance* activePack);
 void endScene(PackInstance* activePack);
 void clearScene(PackInstance* activePack, float fogR = 0.0f, float fogG = 0.0f, float fogB = 0.0f);
 [[nodiscard]] int sceneColorCount(const PackInstance* activePack) const;
 [[nodiscard]] unsigned int sceneDepthTexture(const PackInstance* activePack) const;
 void sampleCenterDepth(PackInstance* activePack, const PackDefinition& activeDef);
 void captureOpaqueDepth(PackInstance* activePack);
 void captureHandDepth(PackInstance* activePack);
 bool renderBegin(PackInstance* activePack, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                  const int* shadowColorTextureIds, int shadowColorTextureCount,
                  shadowmap::ShadowTargets* shadowTargets, const int* shadowColorAltTextureIds);
 bool renderShadowComposite(PackInstance* activePack, int shadowDepthTextureId,
                            int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds,
                            int shadowColorTextureCount, shadowmap::ShadowTargets* shadowTargets,
                            const int* shadowColorAltTextureIds);
 bool renderPreWorld(PackInstance* activePack, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds, int shadowColorTextureCount,
                     shadowmap::ShadowTargets* shadowTargets, const int* shadowColorAltTextureIds);
 bool renderDeferred(PackInstance* activePack, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                     const int* shadowColorTextureIds, int shadowColorTextureCount,
                     const int* shadowColorAltTextureIds);
 bool renderPostProcess(PackInstance* activePack, PackInstance* basePack, int shadowDepthTextureId,
                        int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds, int shadowColorTextureCount,
                        const int* shadowColorAltTextureIds);
 gl::ShaderProgram* worldProgram(WorldProgramId id, PackInstance* pack);
 void presentFinalToScreen(PackInstance* scenePack, int screenWidth, int screenHeight);
 void setPipelinePhase(WorldPipelinePhase phase) noexcept {
  pipelinePhase_ = phase;
 }
 [[nodiscard]] WorldPipelinePhase pipelinePhase() const noexcept {
  return pipelinePhase_;
 }
 [[nodiscard]] bool interfaceProgramsActive() const noexcept {
  return pipelinePhase_ == WorldPipelinePhase::None;
 }
 [[nodiscard]] PackUniformValues& worldUniforms() noexcept {
  return worldUniforms_;
 }
 [[nodiscard]] const PackUniformValues& worldUniforms() const noexcept {
  return worldUniforms_;
 }
 [[nodiscard]] unsigned int lightmapTexture() const noexcept {
  return lightmapTexture_.handle();
 }
 [[nodiscard]] unsigned int normalFallbackTexture() const noexcept {
  return normalFallbackTexture_.handle();
 }
 [[nodiscard]] unsigned int specularFallbackTexture() const noexcept {
  return specularFallbackTexture_.handle();
 }
 [[nodiscard]] int shadowDepthTexture() const noexcept {
  return shadowDepthTexture_;
 }
 [[nodiscard]] int shadowOpaqueDepthTexture() const noexcept {
  return shadowOpaqueDepthTexture_;
 }
 [[nodiscard]] const int* shadowColorTextures() const noexcept {
  return shadowColorTextures_;
 }
 [[nodiscard]] int shadowColorTextureCount() const noexcept {
  return shadowColorTextureCount_;
 }
 private:
 [[nodiscard]] WorldProgramBindContext makeWorldBindContext(WorldProgramId id);
 [[nodiscard]] PackCompiler::LogFnLevel logFn();
 [[nodiscard]] PackInstance* activePack() noexcept;
 [[nodiscard]] const PackInstance* activePack() const noexcept;
 [[nodiscard]] PackInstance* renderPack() noexcept;
 [[nodiscard]] PackInstance* selectedPack() noexcept;
 [[nodiscard]] const PackInstance* selectedPack() const noexcept;
 void activatePack(std::size_t index);
 void compileExecutionPlan(PackInstance& pack, std::size_t programBudget);
 [[nodiscard]] std::unique_ptr<PackInstance> loadPack(const std::filesystem::path& path,
                                                      bool directory);
 [[nodiscard]] std::unique_ptr<PackInstance> loadEmbeddedVanillaPack();
 void initializePackRuntime(PackInstance& pack);
 void refreshSummaries();
 void startDirectoryWatcher();
 void stopDirectoryWatcher();
 void directoryWatchLoop(const std::stop_token& stop);
 void captureDepth(PackInstance* activePack, std::size_t index);
 void logOnce(PackInstance& pack, const std::string& message,
              ::net::minecraft::util::logging::LogLevel level =
                  ::net::minecraft::util::logging::LogLevel::Info) const;
 [[nodiscard]] bool blitColortex0ToScreen(PackInstance& pack, int screenWidth, int screenHeight);
 [[nodiscard]] bool engineOwnsColorCorrection(const PackDefinition& def) const;
 [[nodiscard]] unsigned int screenDrawFramebuffer(int width, int height);
 void finalizeEngineColorCorrection(int screenWidth, int screenHeight);
 void initShadowColorFlips(const PackInstance& pack);
 bool renderCompositePasses(PackInstance& pack, CompositeStage stage, bool present,
                            int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                            const int* shadowColorTextureIds, int shadowColorTextureCount,
                            shadowmap::ShadowTargets* shadowTargets, const int* shadowColorAltTextureIds);
 void finishFinalPass(ColorTargets& targets, bool wroteToScreen);
 option::GameOptions* options_ = nullptr;
 std::filesystem::path gameDirectory_;
 std::filesystem::path shaderCacheDirectory_;
 std::shared_ptr<gl::ShaderBinaryCache> shaderBinaryCache_;
 std::vector<std::unique_ptr<PackInstance>> packs_;
 std::unique_ptr<PackInstance> basePack_;
 std::vector<PackSummary> summaries_;
 static constexpr std::size_t kNoActivePack = static_cast<std::size_t>(-1);
 std::size_t activeIndex_ = kNoActivePack;
 std::atomic<std::uint64_t> watchedStamp_{0};
 std::atomic<bool> directoryChanged_{false};
 std::jthread directoryWatcher_;
 PackUniformValues worldUniforms_{};
 ColorSpaceConverter colorSpace_;
 AsyncDepthSampler centerDepthSampler_;
 bool engineColorCorrect_ = false;
 gl::GlTexture lightmapTexture_;
 gl::GlTexture normalFallbackTexture_;
 gl::GlTexture specularFallbackTexture_;
 std::string resourcePackKey_;
 bool labPbr_ = false;
 bool labPbr13_ = false;
 bool lightmapLit_ = false;
 int lightmapAmbient_ = -1;
 float lightmapBrightness_ = -1.0f;
 WorldPipelinePhase pipelinePhase_ = WorldPipelinePhase::None;
 bool packWroteToScreen_ = false;
 gl::GlFramebuffer presentReadFbo_;
 int shadowDepthTexture_ = -1;
 int shadowOpaqueDepthTexture_ = -1;
 int shadowColorTextures_[8]{};
 int shadowColorAltTextures_[8]{};
 int shadowColorTextureCount_ = 0;
 shadowmap::ShadowTargets* shadowTargets_ = nullptr;
 const PackInstance* shadowColorFlipPack_ = nullptr;
 std::array<bool, 8> shadowColorFlipped_{};
 std::vector<std::array<bool, 8>> shadowCompPassReadFlips_;
 std::uint64_t objectIdRevision_ = 1;
};
} // namespace net::minecraft::client::render
