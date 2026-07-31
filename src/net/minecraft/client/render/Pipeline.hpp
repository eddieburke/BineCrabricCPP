#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/render/ColorSpace.hpp"
#include "net/minecraft/client/render/Instance.hpp"
#include "net/minecraft/client/render/Uniforms.hpp"
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

enum class WorldPipelinePhase { None, Shadow, World };

class Pipeline {
 public:
  explicit Pipeline(option::GameOptions* options);
  ~Pipeline();

  void reset();

  void prepareFrame(net::minecraft::World* world, PackInstance* activePack, PackInstance* basePack,
                   const std::vector<std::unique_ptr<PackInstance>>& packs);
  void refreshLightmap(net::minecraft::World* world);
  void setFrameUniforms(const FrameUniformSet& frame, const PackDefinition* activeDef, PackInstance* activePack);
  void updateLightmap(const net::minecraft::World* world);

  void ensurePbrFallbackTextures();
  void refreshResourcePackState(PackInstance* basePack, const std::vector<std::unique_ptr<PackInstance>>& packs);
  void applyBlockIds(const PackDefinition* definition);

  [[nodiscard]] bool activeHasPostProcess(const PackDefinition* activeDef, const PackInstance* activePack) const;
  [[nodiscard]] bool hasDeferredPasses(const PackInstance* activePack) const;

  [[nodiscard]] std::vector<ColorFormat> sceneColorFormats(const PackInstance* activePack) const;
  [[nodiscard]] bool ensureSceneTargets(PackInstance* activePack, int width, int height);
  void bindScene(PackInstance* activePack);
  void endScene(PackInstance* activePack);
  void destroyScene(PackInstance* activePack);
  void clearScene(PackInstance* activePack, float fogR = 0.0f, float fogG = 0.0f, float fogB = 0.0f);

  [[nodiscard]] int sceneColorCount(const PackInstance* activePack) const;
  [[nodiscard]] unsigned int sceneDepthTexture(const PackInstance* activePack) const;

  void sampleCenterDepth(PackInstance* activePack, const PackDefinition* activeDef);
  void captureOpaqueDepth(PackInstance* activePack);
  void captureHandDepth(PackInstance* activePack);

  bool renderBegin(PackInstance* activePack);
  bool renderPreWorld(PackInstance* activePack, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                      const int* shadowColorTextureIds, int shadowColorTextureCount);
  bool renderDeferred(PackInstance* activePack, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                      const int* shadowColorTextureIds, int shadowColorTextureCount);
  bool renderPostProcess(PackInstance* activePack, PackInstance* basePack, int shadowDepthTextureId,
                         int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds, int shadowColorTextureCount);

  gl::ShaderProgram* worldProgram(const std::string& key, PackInstance* activePack, PackInstance* basePack);
  gl::ShaderProgram* programFromPack(PackInstance& pack, const std::string& key);
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

  [[nodiscard]] FrameUniformSet& worldUniforms() noexcept {
   return worldUniforms_;
  }
  [[nodiscard]] const FrameUniformSet& worldUniforms() const noexcept {
   return worldUniforms_;
  }

  [[nodiscard]] unsigned int lightmapTexture() const noexcept {
   return lightmapTexture_;
  }
  [[nodiscard]] unsigned int* lightmapTexturePtr() noexcept {
   return &lightmapTexture_;
  }
  [[nodiscard]] unsigned int normalFallbackTexture() const noexcept {
   return normalFallbackTexture_;
  }
  [[nodiscard]] unsigned int specularFallbackTexture() const noexcept {
   return specularFallbackTexture_;
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
  [[nodiscard]] const std::string& lastWorldProgramKey() const noexcept {
   return lastWorldProgramKey_;
  }

 private:
  void captureDepth(PackInstance* activePack, std::size_t index);
  void logOnce(PackInstance& pack, const std::string& message,
               ::net::minecraft::util::logging::LogLevel level =
                   ::net::minecraft::util::logging::LogLevel::Info) const;
  [[nodiscard]] bool blitColortex0ToScreen(PackInstance& pack, int screenWidth, int screenHeight);
  [[nodiscard]] bool engineOwnsColorCorrection(const PackDefinition* def) const;
  [[nodiscard]] unsigned int screenDrawFramebuffer(int width, int height);
  void finalizeEngineColorCorrection(int screenWidth, int screenHeight);
  bool runPasses(PackInstance& pack, const std::vector<std::size_t>& passes, bool present,
                 const std::string& stage, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                 const int* shadowColorTextureIds, int shadowColorTextureCount);

  option::GameOptions* options_ = nullptr;
  FrameUniformSet worldUniforms_{};
  ColorSpaceConverter colorSpace_;
  bool engineColorCorrect_ = false;

  unsigned int lightmapTexture_ = 0;
  unsigned int normalFallbackTexture_ = 0;
  unsigned int specularFallbackTexture_ = 0;

  std::string resourcePackKey_;
  bool labPbr_ = false;
  bool labPbr13_ = false;
  bool lightmapLit_ = false;
  int lightmapAmbient_ = -1;
  float lightmapBrightness_ = -1.0f;

  WorldPipelinePhase pipelinePhase_ = WorldPipelinePhase::None;
  bool packWroteToScreen_ = false;
  unsigned int presentReadFbo_ = 0;
  std::string lastWorldProgramKey_;

  int shadowDepthTexture_ = -1;
  int shadowOpaqueDepthTexture_ = -1;
  int shadowColorTextures_[8]{};
  int shadowColorTextureCount_ = 0;
};

} // namespace net::minecraft::client::render
