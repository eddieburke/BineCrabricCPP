#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GlFramebuffer.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/render/ColorSpace.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
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

enum class WorldPipelinePhase { None, Shadow, World };

namespace shadowmap {
struct ShadowTargets;
}

class Pipeline {
 public:
  explicit Pipeline(option::GameOptions* options);
  ~Pipeline();

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

  gl::ShaderProgram* worldProgram(const std::string& key, PackInstance* pack);
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

  [[nodiscard]] PackUniformValues& worldUniforms() noexcept {
   return worldUniforms_;
  }
  [[nodiscard]] const PackUniformValues& worldUniforms() const noexcept {
   return worldUniforms_;
  }

  [[nodiscard]] unsigned int lightmapTexture() const noexcept {
   return lightmapTexture_.handle();
  }
  [[nodiscard]] unsigned int* lightmapTexturePtr() noexcept {
   return lightmapTexture_.handlePtr();
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
  [[nodiscard]] const std::string& lastWorldProgramKey() const noexcept {
   return lastWorldProgramKey_;
  }
  [[nodiscard]] const std::string& resolvedWorldProgramKey() const noexcept {
   return resolvedWorldProgramKey_;
  }

 private:
  friend class CompositeRenderer;
  friend class FinalPassRenderer;

  void captureDepth(PackInstance* activePack, std::size_t index);
  void logOnce(PackInstance& pack, const std::string& message,
               ::net::minecraft::util::logging::LogLevel level =
                   ::net::minecraft::util::logging::LogLevel::Info) const;
  [[nodiscard]] bool blitColortex0ToScreen(PackInstance& pack, int screenWidth, int screenHeight);
  [[nodiscard]] bool engineOwnsColorCorrection(const PackDefinition& def) const;
  [[nodiscard]] unsigned int screenDrawFramebuffer(int width, int height);
  void finalizeEngineColorCorrection(int screenWidth, int screenHeight);

  void initShadowColorFlips(const PackInstance& pack);

  option::GameOptions* options_ = nullptr;
  PackUniformValues worldUniforms_{};
  ColorSpaceConverter colorSpace_;
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
  std::string lastWorldProgramKey_;
  std::string resolvedWorldProgramKey_;

  int shadowDepthTexture_ = -1;
  int shadowOpaqueDepthTexture_ = -1;
  int shadowColorTextures_[8]{};
  int shadowColorAltTextures_[8]{};
  int shadowColorTextureCount_ = 0;
  shadowmap::ShadowTargets* shadowTargets_ = nullptr;
  const PackInstance* shadowColorFlipPack_ = nullptr;
  std::array<bool, 8> shadowColorFlipped_{};
  std::vector<std::array<bool, 8>> shadowCompPassReadFlips_;
};

} // namespace net::minecraft::client::render
