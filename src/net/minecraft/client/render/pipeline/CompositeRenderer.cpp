#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/pipeline/FullscreenPass.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/targets/ShadowMapPass.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/debug/VTuneTrace.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GlFramebuffer.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
namespace net::minecraft::client::render {
using LogLevel = ::net::minecraft::util::logging::LogLevel;
namespace {
void refreshColorMaps(render::ColorTargets& targets, std::unordered_map<std::string, int>& textures,
                      std::unordered_map<std::string, int>& colorImages, bool fullscreenPass) {
 targets.fillReadSamplers(textures, fullscreenPass);
 targets.fillImageBindings(colorImages);
}
void publishReadSide(std::unordered_map<std::string, int>& textures,
                     std::unordered_map<std::string, int>& colorImages,
                     const std::array<bool, 8>& flips,
                     const int* shadowColorTextureIds, const int* shadowColorAltTextureIds,
                     int shadowColorTextureCount, const PackInstance& pack) {
 for(int i = 0; i < std::min(shadowColorTextureCount, 8); ++i) {
  if(shadowColorTextureIds == nullptr || shadowColorAltTextureIds == nullptr) continue;
  const int read = flips[static_cast<std::size_t>(i)] ? shadowColorAltTextureIds[i] : shadowColorTextureIds[i];
  if(read < 0) continue;
  const std::string name = "shadowcolor" + std::to_string(i);
  textures[name] = read;
  colorImages[name] = read;
 }
 refreshTextureAliases(textures, pack.definition.usesWaterShadow);
}
void bindPassTargets(shadowmap::ShadowTargets* shadowTargets, const PackPass& pass,
                     const std::array<bool, 8>& readFlips, int width, int height) {
 std::vector<std::string> outputs = pass.outputs.empty() ? std::vector<std::string>{"shadowcolor0"} : pass.outputs;
 int colorCount = 0;
 for(const std::string& name : outputs) {
  const int index = shadowColorIndex(name);
  if(index >= 0) colorCount = std::max(colorCount, index + 1);
 }
 if(colorCount <= 0) {
  colorCount = 1;
 }
 shadowTargets->attachCompositeColors(readFlips, colorCount);
 shadowTargets->fbo.bind();
 core::viewport(0, 0, width, height);
}
const std::string& stageName(CompositeStage stage) {
 static const std::array<std::string, static_cast<std::size_t>(CompositeStage::Count)> names = {
     "begin", "shadowcomp", "prepare", "deferred", "composite"};
 return names[static_cast<std::size_t>(stage)];
}
} // namespace
bool Pipeline::renderCompositePasses(PackInstance& pack, CompositeStage stageId, bool present,
                                     int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                     const int* shadowColorTextureIds, int shadowColorTextureCount,
                                     shadowmap::ShadowTargets* shadowTargets, const int* shadowColorAltTextureIds) {
 const std::string& stage = stageName(stageId);
 const std::string stageSpanName = "shader/" + stage;
 VT_TRACE_EVENT(stageSpanName.c_str());
 const std::vector<PackInstance::RuntimeOperation>& operations = pack.stagePlan(stageId);
 render::ColorTargets& targets = pack.colorTargets;
 const int shadowMapResolution = static_cast<int>(worldUniforms_.shadowMapResolution);
 const int width = stage == "shadowcomp" && shadowMapResolution > 0 ? shadowMapResolution : targets.width();
 const int height = stage == "shadowcomp" && shadowMapResolution > 0 ? shadowMapResolution : targets.height();
 if(!pack.summary.valid || pack.programs == nullptr ||
    !pack.stagePlanValid[static_cast<std::size_t>(stageId)] ||
    (operations.empty() && pack.setupPlan.empty())) {
  return false;
 }
 if(!targets.valid() || targets.depthTexture() == 0 || !hasGlContext() || width <= 0 || height <= 0) {
  return false;
 }
 if(!ensurePackResources(pack, targets.width(), targets.height(), lightmapTexture_,
                         [](const PackInstance& p, const std::string& path) {
                          return PackCompiler::readText(p, path);
                         })) {
  logOnce(pack, "pack GPU resources could not be allocated", LogLevel::Severe);
  return false;
 }
 int destinationFramebuffer = 0;
 ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &destinationFramebuffer);
 struct ViewportGuard {
  int saved[4]{};
  bool valid = false;
  ViewportGuard() {
   valid = core::getCachedViewport(saved);
  }
  ~ViewportGuard() {
   if(valid) core::viewport(saved[0], saved[1], saved[2], saved[3]);
  }
  ViewportGuard(const ViewportGuard&) = delete;
  ViewportGuard& operator=(const ViewportGuard&) = delete;
 };
 const ViewportGuard viewportGuard;
 const core::DepthScope depthScope(false, false);
 const core::CullScope cullScope(false);
 const core::BlendScope blendScope(false);
 const core::TextureBindScope textureScope;
 std::unordered_map<std::string, int> textures;
 std::unordered_map<std::string, int> colorImages;
 textures.reserve(96);
 colorImages.reserve(48);
 refreshColorMaps(targets, textures, colorImages, true);
 textures["depthtex0"] = static_cast<int>(targets.depthTexture());
 textures["depthtex1"] = pack.opaqueDepthTexture(textures["depthtex0"]);
 textures["depthtex2"] = pack.handDepthTexture(textures["depthtex0"]);
 VT_TRACE_COUNTER("CompositeOpaqueDepthReady", pack.depthTextures[1] ? 1 : 0);
 VT_TRACE_COUNTER("CompositeHandDepthReady", pack.depthTextures[0] ? 1 : 0);
 targets.applyPreFlips(pack.definition, stage);
 refreshColorMaps(targets, textures, colorImages, true);
 for(const auto& [name, texture] : pack.publishedTextures) {
  textures[name] = texture;
 }
 const int opaqueShadow =
     shadowOpaqueDepthTextureId >= 0 ? shadowOpaqueDepthTextureId : shadowDepthTextureId;
 putShadowTextures(textures, shadowDepthTextureId, opaqueShadow, shadowColorTextureIds,
                   shadowColorTextureCount, pack.definition);
 for(int i = 0; i < std::min(shadowColorTextureCount, 8); ++i) {
  if(shadowColorTextureIds != nullptr && shadowColorTextureIds[i] >= 0) {
   colorImages["shadowcolor" + std::to_string(i)] = shadowColorTextureIds[i];
  }
 }
 const bool shadowFlipsKnown = shadowColorFlipPack_ == &pack;
 if(stage != "shadowcomp" && shadowFlipsKnown) {
  publishReadSide(textures, colorImages, shadowColorFlipped_,
                  shadowColorTextureIds, shadowColorAltTextureIds,
                  shadowColorTextureCount, pack);
 }
 std::unordered_map<std::string, int> volumeTextures;
 volumeTextures.reserve(16);
 addPackTextures(pack, stage, textures, volumeTextures);
 const PackUniformValues& frameUniforms = worldUniforms_;
 const PackViewportValues viewport{static_cast<float>(width),
                                   static_cast<float>(height),
                                   static_cast<float>(width) / std::max(static_cast<float>(height), 1.0f)};
 const bool computeReady = gl::GLCore::computeSupported;
 const bool concurrent = pack.definition.allowConcurrentCompute;
 bool ranCompute = false;
 const auto prepareComputeBinds = [&]() {
  if(gl::GLCore::bindFramebuffer != nullptr) gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
  refreshColorMaps(targets, textures, colorImages, true);
  refreshTextureAliases(textures, pack.definition.usesWaterShadow);
 };
 const auto publishShadowColorReadSide = [&]() {
  shadowDepthTexture_ = shadowDepthTextureId;
  shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
  for(int index = 0; index < shadowColorTextureCount_; ++index) {
   shadowColorTextures_[index] =
       flipSideTextureId(index, shadowColorTextureIds, shadowColorAltTextureIds, &pack,
                         shadowColorFlipped_, shadowColorFlipPack_);
  }
 };
 if(operations.empty()) {
  if(present) {
   finishFinalPass(targets, false);
  } else if(gl::GLCore::bindFramebuffer != nullptr) {
   gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), static_cast<unsigned int>(destinationFramebuffer));
   core::viewport(0, 0, width, height);
  }
  releaseSamplers(maxTextureUnits());
  core::activeTexture(gl::tex::Texture0);
  publishShadowColorReadSide();
  return present ? false : ranCompute;
 }
 bool wroteToScreen = false;
 bool executed = false;
 std::size_t passPosition = 0;
 for(const PackInstance::RuntimeOperation& operation : operations) {
  const PackInstance::RuntimePass& runtime = operation.pass;
  const std::string passSpanName = stage + "/" + pack.definition.passes[runtime.passIndex].name;
  VT_TRACE_EVENT(passSpanName.c_str());
  VT_TRACE_COUNTER("ShaderPasses", 1);
  if(operation.compute) {
   if(!computeReady) continue;
   if(operation.groupBegin) prepareComputeBinds();
   if(runtime.program == nullptr ||
      !ComputeDispatcher::dispatch(pack, pack.definition.passes[runtime.passIndex], *runtime.program,
                                   frameUniforms, &viewport, textures, colorImages, volumeTextures,
                                   &targets, width, height, !concurrent)) {
    releaseSamplers(maxTextureUnits());
    return false;
   }
   ranCompute = true;
   if(operation.groupEnd && gl::GLCore::memoryBarrier != nullptr) {
    gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
    VT_TRACE_COUNTER("MemoryBarriers", 1);
   }
   continue;
  }
  const std::size_t passIndex = runtime.passIndex;
  gl::ShaderProgram* program = runtime.program;
  if(program == nullptr) return false;
  const PackPass& pass = pack.definition.passes[passIndex];
  const bool shadowCompPass = stage == "shadowcomp" && shadowFlipsKnown &&
                              passPosition < shadowCompPassReadFlips_.size();
  if(shadowCompPass) {
   publishReadSide(textures, colorImages,
                   shadowCompPassReadFlips_[passPosition],
                   shadowColorTextureIds, shadowColorAltTextureIds,
                   shadowColorTextureCount, pack);
  }
  const std::string output =
      pass.outputs.empty() ? (stage == "shadowcomp" ? std::string("shadowcolor0") : std::string("screen"))
                           : pass.outputs.front();
  const bool toScreen =
      PackCatalog::lower(output) == "screen" || pass.name == "final" ||
      pass.program.rfind("final", 0) == 0;
  // CompositeRenderer.java:307-312 setupMipmapping.
  for(const std::string& buffer : pass.mipmapBuffers) {
   core::activeTexture(gl::tex::Texture0);
   targets.enableMipmaps(buffer);
  }
  if(!toScreen) {
   if(shadowCompPass && shadowTargets != nullptr) {
    bindPassTargets(shadowTargets, pass,
                    shadowCompPassReadFlips_[passPosition], width, height);
   } else {
    std::vector<std::string> outputs = pass.outputs.empty() ? std::vector<std::string>{output} : pass.outputs;
    for(const std::string& name : outputs) {
     const auto declared = pack.definition.targets.find(name);
     ColorFormat format = ColorFormat::Rgba8;
     int tw = width;
     int th = height;
     if(declared != pack.definition.targets.end()) {
      format = parseFormat(declared->second.format);
      const PackTarget& tgt = declared->second;
      if(tgt.absoluteWidth > 0 && tgt.absoluteHeight > 0) {
       tw = tgt.absoluteWidth;
       th = tgt.absoluteHeight;
      } else {
       const float sx = tgt.scaleX > 0.0f ? tgt.scaleX : tgt.scale;
       const float sy = tgt.scaleY > 0.0f ? tgt.scaleY : tgt.scale;
       tw = std::max(1, static_cast<int>(std::lround(static_cast<float>(width) * sx)));
       th = std::max(1, static_cast<int>(std::lround(static_cast<float>(height) * sy)));
      }
     }
     if(!targets.targetMatches(name, tw, th, format) && !targets.ensureNamed(name, tw, th, format)) {
      logOnce(pack, "pass '" + pass.name + "' could not allocate target '" + name + "'", LogLevel::Severe);
      return false;
     }
    }
    refreshColorMaps(targets, textures, colorImages, true);
    if(!targets.bindWrite(outputs)) {
     logOnce(pack, "pass '" + pass.name + "' could not bind write targets", LogLevel::Severe);
     return false;
    }
   }
  } else {
   const unsigned int drawFbo =
       present ? screenDrawFramebuffer(width, height) : static_cast<unsigned int>(destinationFramebuffer);
   gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), drawFbo);
   core::viewport(0, 0, width, height);
  }
  program->bind();
  applyBufferBlends(pack.definition, pass.program, program->drawBufferColortexIndices());
  applyAlphaTest(pack.definition, pass.program);
  bool fullViewport = true;
  if(const auto scaleIt = pack.definition.programScales.find(pass.program);
     scaleIt != pack.definition.programScales.end()) {
   const ProgramScale& sc = scaleIt->second;
   const int passViewW = std::max(1, static_cast<int>(std::lround(static_cast<float>(width) * sc.scale)));
   const int passViewH = std::max(1, static_cast<int>(std::lround(static_cast<float>(height) * sc.scale)));
   const int passViewX = static_cast<int>(std::lround(static_cast<float>(width) * sc.offsetX));
   const int passViewY = static_cast<int>(std::lround(static_cast<float>(height) * sc.offsetY));
   core::viewport(passViewX, passViewY, passViewW, passViewH);
   fullViewport = sc.scale >= 0.999f && sc.offsetX <= 0.001f && sc.offsetY <= 0.001f;
  }
  refreshTextureAliases(textures, pack.definition.usesWaterShadow);
  addPackTextures(pack, stage, textures, volumeTextures);
  bindSamplers(*program, textures, volumeTextures, maxTextureUnits(),
               pack.definition);
  const unsigned int nextImageUnit =
      bindColorImages(*program, colorImages, pack.definition, &targets);
  bindPackResources(pack, *program, nextImageUnit);
  uploadShaderUniforms(*program, frameUniforms, true, &viewport);
  uploadIdentityDrawMatrices(*program);
  pack.customUniforms.upload(*program);
  program->bind();
  core::drawFullscreen();
  core::unlockBlend();
  executed = true;
  if(toScreen) {
   wroteToScreen = fullViewport;
  } else if(shadowCompPass) {
  } else {
   std::vector<std::string> outputs = pass.outputs.empty() ? std::vector<std::string>{output} : pass.outputs;
   targets.applyPassFlips(pack.definition, pass.name, outputs);
   if(stage == "shadowcomp" || stage == "prepare") {
    for(const std::string& name : outputs) {
     pack.publishedTextures[name] = static_cast<int>(targets.readTexture(name));
    }
   }
   refreshColorMaps(targets, textures, colorImages, true);
  }
  core::activeTexture(gl::tex::Texture0);
  ++passPosition;
 }
 if(present) {
  finishFinalPass(targets, wroteToScreen);
 }
 if(!present) {
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), static_cast<unsigned int>(destinationFramebuffer));
  core::viewport(0, 0, width, height);
 }
 releaseSamplers(maxTextureUnits());
 core::activeTexture(gl::tex::Texture0);
 publishShadowColorReadSide();
 return present ? wroteToScreen : executed || ranCompute;
}
void Pipeline::finishFinalPass(ColorTargets& targets, bool wroteToScreen) {
 targets.resetMipmaps();
 packWroteToScreen_ = wroteToScreen;
}
} // namespace net::minecraft::client::render
