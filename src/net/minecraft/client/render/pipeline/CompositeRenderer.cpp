#include "net/minecraft/client/render/pipeline/CompositeRenderer.hpp"
#include "net/minecraft/client/render/pipeline/FinalPassRenderer.hpp"
#include "net/minecraft/client/render/pipeline/FullscreenPass.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
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
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace net::minecraft::client::render {
using LogLevel = ::net::minecraft::util::logging::LogLevel;
namespace {
void eraseColortexKeys(std::unordered_map<std::string, int>& map) {
 for(auto it = map.begin(); it != map.end();) {
  const std::string& key = it->first;
  if(key.rfind("colortex", 0) == 0 || key.rfind("colorimg", 0) == 0 || key == "gcolor" || key == "gdepth" ||
     key == "gnormal" || key == "composite" || key == "gaux1" || key == "gaux2" || key == "gaux3" ||
     key == "gaux4") {
   it = map.erase(it);
  } else {
   ++it;
  }
 }
}

void refreshColorMaps(render::ColorTargets& targets, std::unordered_map<std::string, int>& textures,
                      std::unordered_map<std::string, int>& colorImages, bool fullscreenPass) {
  eraseColortexKeys(textures);
  eraseColortexKeys(colorImages);
  targets.fillReadSamplers(textures, fullscreenPass);
  targets.fillImageBindings(colorImages);
}

// (Iris' stageReadsFromAlt samplers/images, ShadowCompositeRenderer.java:125,301-302).
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

// framebuffer (ShadowRenderTargets.java createColorFramebuffer, ShadowCompositeRenderer.java:123).
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
} // namespace

bool CompositeRenderer::render(PackInstance& pack, const std::vector<std::size_t>& passes, bool present,
                               const std::string& stage, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                               const int* shadowColorTextureIds, int shadowColorTextureCount,
                               shadowmap::ShadowTargets* shadowTargets, const int* shadowColorAltTextureIds) {
  FinalPassRenderer finalPass(pipeline_);
  render::ColorTargets& targets = pack.colorTargets;
   const int shadowMapResolution = static_cast<int>(pipeline_->worldUniforms_.shadowMapResolution);
   const int width = stage == "shadowcomp" && shadowMapResolution > 0 ? shadowMapResolution : targets.width();
  const int height = stage == "shadowcomp" && shadowMapResolution > 0 ? shadowMapResolution : targets.height();
 if(!pack.summary.valid || pack.programs == nullptr ||
    (passes.empty() && pack.computePasses.empty() && pack.setupPasses.empty())) {
  return false;
 }
   if(!targets.valid() || targets.depthTexture() == 0 || !hasGlContext() || width <= 0 || height <= 0) {
    return false;
   }
 if(!PackResources::ensure(pack, width, height, pipeline_->lightmapTexture_,
                                 [](const PackInstance& p, const std::string& path) {
                                  return PackCompiler::readText(p, path);
                                 })) {
  pipeline_->logOnce(pack, "pack GPU resources could not be allocated", LogLevel::Severe);
  return false;
 }

 int destinationFramebuffer = 0;
 ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &destinationFramebuffer);
 std::vector<std::pair<std::size_t, gl::ShaderProgram*>> programChain;
 programChain.reserve(passes.size());
 for(std::size_t passIndex : passes) {
  if(passIndex >= pack.definition.passes.size()) continue;
  const PackPass& pass = pack.definition.passes[passIndex];
  if(pack.definition.programs.count(pass.program) == 0) {
   pipeline_->logOnce(pack, "pass '" + pass.name + "' references unknown program '" + pass.program + "'",
           LogLevel::Severe);
   return false;
  }
  gl::ShaderProgram* program = PackCompiler::compile(
      pack, pass.program, [this](PackInstance& p, const std::string& m, LogLevel level) {
       pipeline_->logOnce(p, m, level);
      });
  if(program == nullptr) {
   pipeline_->logOnce(pack, "pass '" + pass.name + "' program '" + pass.program + "' is unusable", LogLevel::Severe);
   return false;
  }
  programChain.push_back({passIndex, program});
 }

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
  refreshColorMaps(targets, textures, colorImages, true);
  textures["depthtex0"] = static_cast<int>(targets.depthTexture());
  // https://shaders.properties/current/reference/buffers/depthtex/
  if(pack.depthTextures[0]) textures["depthtex1"] = static_cast<int>(pack.depthTextures[0].handle());
  if(pack.depthTextures[1]) {
   textures["depthtex2"] = static_cast<int>(pack.depthTextures[1].handle());
  } else if(pack.depthTextures[0]) {
   textures["depthtex2"] = static_cast<int>(pack.depthTextures[0].handle());
  }
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
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowCompositeRenderer.java
  const bool shadowFlipsKnown = pipeline_->shadowColorFlipPack_ == &pack;
  if(stage != "shadowcomp" && shadowFlipsKnown) {
   publishReadSide(textures, colorImages, pipeline_->shadowColorFlipped_,
                   shadowColorTextureIds, shadowColorAltTextureIds,
                   shadowColorTextureCount, pack);
  }
 std::unordered_map<std::string, int> volumeTextures;
 PackResources::addTextures(pack, stage, textures, volumeTextures);

 const PackUniformValues& frameUniforms = pipeline_->worldUniforms_;
 const PackViewportValues viewport{static_cast<float>(width),
                                   static_cast<float>(height),
                                   static_cast<float>(width) / std::max(static_cast<float>(height), 1.0f)};

 const bool computeReady = gl::GLCore::computeSupported;
 auto compileFn = [this](PackInstance& p, const std::string& name) {
  return PackCompiler::compile(p, name, [this](PackInstance& p2, const std::string& m, LogLevel level) {
   pipeline_->logOnce(p2, m, level);
  });
 };
 if(computeReady &&
    !dispatchSetupIfNeeded(pack, frameUniforms, &viewport, width, height, textures, colorImages, volumeTextures, &targets,
                           compileFn)) {
  releaseSamplers(maxTextureUnits());
  return false;
 }

 // https://github.com/IrisShaders/ShaderDoc/blob/master/passes/compute.md
 const bool concurrent = pack.definition.allowConcurrentCompute;
 std::vector<std::size_t> stageComputes;
 if(computeReady) {
  for(std::size_t passIndex : pack.computePasses) {
   const PackPass& compute = pack.definition.passes[passIndex];
   if(!ComputeDispatcher::matchesStage(compute.name, stage) &&
      !(stage == "composite" && ComputeDispatcher::matchesStage(compute.name, "final"))) {
    continue;
   }
   stageComputes.push_back(passIndex);
  }
  std::stable_sort(stageComputes.begin(), stageComputes.end(), [&pack](std::size_t a, std::size_t b) {
   const std::string& na = pack.definition.passes[a].name;
   const std::string& nb = pack.definition.passes[b].name;
   const std::string pa = ComputeDispatcher::computeParentName(na);
   const std::string pb = ComputeDispatcher::computeParentName(nb);
   if(pa != pb) return ComputeDispatcher::lessComputeParent(pa, pb);
   return ComputeDispatcher::lessComputeOrder(pack.definition.passes[a], pack.definition.passes[b]);
  });
 }

 std::vector<bool> computeDispatched(pack.definition.passes.size(), false);
 bool ranCompute = false;

  const auto prepareComputeBinds = [&]() {
   if(gl::GLCore::bindFramebuffer != nullptr) gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
   if(gl::GLCore::memoryBarrier != nullptr) {
    gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
   }
   refreshColorMaps(targets, textures, colorImages, true);
   refreshTextureAliases(textures, pack.definition.usesWaterShadow);
  };

 const auto dispatchParentComputes = [&](const std::string& parent) -> bool {
  std::vector<std::string> writeBuffers;
  writeBuffers.reserve(8);
  for(std::size_t passIndex : stageComputes) {
   if(ComputeDispatcher::computeParentName(pack.definition.passes[passIndex].name) != parent) continue;
   gl::ShaderProgram* program = compileFn(pack, pack.definition.passes[passIndex].program);
   if(program == nullptr) continue;
   for(int i = 0; i < targets.colorCount(); ++i) {
    if(program->location("colorimg" + std::to_string(i)) < 0) continue;
    const std::string name = "colortex" + std::to_string(i);
    if(std::find(writeBuffers.begin(), writeBuffers.end(), name) == writeBuffers.end()) {
     writeBuffers.push_back(name);
    }
   }
  }
  {
   std::ostringstream wb;
   for(std::size_t i = 0; i < writeBuffers.size(); ++i) {
    if(i) wb << ',';
    wb << writeBuffers[i] << '=' << colorFormatName(targets.formatOf(writeBuffers[i]));
   }
  pipeline_->logOnce(pack, "compute parent '" + parent + "' writeBuffers=[" + wb.str() + "] concurrent=" +
                    (concurrent ? "true" : "false"));
 }
 for(const std::string& name : writeBuffers) {
  targets.prepareWrite(name);
 }
  prepareComputeBinds();
  bool any = false;
 for(std::size_t passIndex : stageComputes) {
  if(computeDispatched[passIndex]) continue;
  if(ComputeDispatcher::computeParentName(pack.definition.passes[passIndex].name) != parent) continue;
   if(!ComputeDispatcher::dispatch(pack, pack.definition.passes[passIndex], frameUniforms, &viewport, textures,
                                   colorImages, volumeTextures, &targets, width, height, !concurrent, compileFn)) {
   return false;
  }
   computeDispatched[passIndex] = true;
   any = true;
   ranCompute = true;
  }
  if(any && concurrent && gl::GLCore::memoryBarrier != nullptr) {
   gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
  }
  if(any) {
   targets.applyPassFlips(pack.definition, parent, writeBuffers);
   refreshColorMaps(targets, textures, colorImages, true);
   pipeline_->logOnce(pack, "compute parent '" + parent + "' flipped writeBuffers and refreshed samplers");
   // TEMP DIAGNOSTIC — [stage-probe]. Reads the centre texel of every colortex after
   // each compute parent, so the stage where the values blow out is visible directly
   // instead of being inferred from the final image. DELETE once lighting is correct.
   {
    static int stageProbeFrame = 0;
    static std::string firstParent;
    if(firstParent.empty()) firstParent = parent;
    if(parent == firstParent) ++stageProbeFrame;
    if((stageProbeFrame % 120) == 0) {
     gl::GlFramebuffer probeFbo;
     std::ostringstream line;
     line << "[stage-probe] after '" << parent << "'";
     for(int i = 0; i < targets.colorCount(); ++i) {
      const unsigned tex = static_cast<unsigned>(targets.readTexture(i));
      if(tex == 0) continue;
      probeFbo.addColorAttachment(0, tex);
      probeFbo.bindAsReadBuffer();
      // An integer target must be read as GL_RGBA_INTEGER/GL_UNSIGNED_INT — asking for
      // GL_RGBA/GL_FLOAT is a GL_INVALID_OPERATION that quietly leaves the destination
      // untouched, which is why colortex2 read as all-zero here regardless of content.
      // Packs stash bit-cast floats in integer targets (RenderPearl puts s_distortion
      // in colortex2.r), so print both the raw bits and their float reinterpretation.
      if(render::isIntegerColorFormat(targets.formatOf("colortex" + std::to_string(i)))) {
       unsigned int texel[4]{};
       ::glReadPixels(width / 2, height / 2, 1, 1, 0x8D99, 0x1405, texel); // GL_RGBA_INTEGER, GL_UNSIGNED_INT
       float asFloat = 0.0f;
       std::memcpy(&asFloat, &texel[0], sizeof(float));
       line << " colortex" << i << "=u(" << texel[0] << ',' << texel[1] << ',' << texel[2]
            << ") r_as_float=" << std::fixed << std::setprecision(4) << asFloat;
      } else {
       float texel[4]{};
       ::glReadPixels(width / 2, height / 2, 1, 1, 0x1908, 0x1406, texel);
       line << " colortex" << i << "=(" << std::fixed << std::setprecision(4) << texel[0] << ','
            << texel[1] << ',' << texel[2] << ')';
      }
     }
     gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::ReadFramebuffer), 0);
     gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
     ::net::minecraft::client::ClientLog::LOGGER.log(LogLevel::Info, line.str());
    }
   }
  }
  return true;
 };

 const auto dispatchOrphansBefore = [&](const std::string* nextRasterPass) -> bool {
  for(std::size_t passIndex : stageComputes) {
   if(computeDispatched[passIndex]) continue;
   const std::string parent = ComputeDispatcher::computeParentName(pack.definition.passes[passIndex].name);
   const bool hasRaster = std::any_of(programChain.begin(), programChain.end(), [&](const auto& entry) {
    return pack.definition.passes[entry.first].name == parent;
   });
   if(hasRaster) continue;
   if(nextRasterPass != nullptr && !ComputeDispatcher::lessComputeParent(parent, *nextRasterPass)) break;
   if(!dispatchParentComputes(parent)) return false;
  }
  return true;
 };

 if(computeReady && programChain.empty()) {
  if(!dispatchOrphansBefore(nullptr)) {
   releaseSamplers(maxTextureUnits());
   return false;
  }
 }
  const auto publishShadowColorReadSide = [&]() {
   pipeline_->shadowDepthTexture_ = shadowDepthTextureId;
   pipeline_->shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
   for(int index = 0; index < pipeline_->shadowColorTextureCount_; ++index) {
    pipeline_->shadowColorTextures_[index] =
        flipSideTextureId(index, shadowColorTextureIds, shadowColorAltTextureIds, &pack,
                          pipeline_->shadowColorFlipped_, pipeline_->shadowColorFlipPack_);
   }
  };

 if(programChain.empty()) {
  if(present) {
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java
   finalPass.finish(targets, false);
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
  for(const auto& [passIndex, program] : programChain) {
   const PackPass& pass = pack.definition.passes[passIndex];
   const bool shadowCompPass = stage == "shadowcomp" && shadowFlipsKnown &&
                               passPosition < pipeline_->shadowCompPassReadFlips_.size();
    if(shadowCompPass) {
     publishReadSide(textures, colorImages,
                     pipeline_->shadowCompPassReadFlips_[passPosition],
                     shadowColorTextureIds, shadowColorAltTextureIds,
                     shadowColorTextureCount, pack);
    }
   if(computeReady && !dispatchOrphansBefore(&pass.name)) {
   releaseSamplers(maxTextureUnits());
   return false;
  }
  if(computeReady) {
   if(!dispatchParentComputes(pass.name)) {
    releaseSamplers(maxTextureUnits());
    return false;
   }
  }
  const std::string output =
      pass.outputs.empty() ? (stage == "shadowcomp" ? std::string("shadowcolor0") : std::string("screen"))
                           : pass.outputs.front();
  const bool toScreen =
      PackCatalog::lower(output) == "screen" || pass.name == "final" ||
      pass.program.rfind("final", 0) == 0;
  for(const std::string& buffer : pass.mipmapBuffers) {
   const auto tex = textures.find(buffer);
   if(tex == textures.end() || tex->second <= 0 || gl::GLCore::generateMipmap == nullptr) continue;
   core::activeTexture(gl::tex::Texture0);
   core::bindTexture(tex->second);
   ::glTexParameteri(kTexture2D, 0x2801, 0x2703);
   ::glTexParameteri(kTexture2D, 0x2800, 0x2601);
   gl::GLCore::generateMipmap(kTexture2D);
  }
  if(!toScreen) {
    if(shadowCompPass && shadowTargets != nullptr) {
     // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderTargets.java
     bindPassTargets(shadowTargets, pass,
                     pipeline_->shadowCompPassReadFlips_[passPosition], width, height);
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
     if(tw != targets.width() || th != targets.height() || targets.readTexture(name) == 0) {
      if(!targets.ensureNamed(name, tw, th, format)) {
       pipeline_->logOnce(pack, "pass '" + pass.name + "' could not allocate target '" + name + "'", LogLevel::Severe);
       return false;
      }
     }
     targets.prepareWrite(name);
    }
     refreshColorMaps(targets, textures, colorImages, true);
     if(!targets.bindWrite(outputs)) {
     pipeline_->logOnce(pack, "pass '" + pass.name + "' could not bind write targets", LogLevel::Severe);
     return false;
    }
   }
  } else {
   const unsigned int drawFbo =
       present ? pipeline_->screenDrawFramebuffer(width, height) : static_cast<unsigned int>(destinationFramebuffer);
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
  PackResources::addTextures(pack, stage, textures, volumeTextures);
  bindSamplers(*program, textures, volumeTextures, maxTextureUnits(),
                       pack.definition);
  const unsigned int nextImageUnit =
      bindColorImages(*program, colorImages, pack.definition, &targets);
  PackResources::bind(pack, *program, nextImageUnit);
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
 if(computeReady && !dispatchOrphansBefore(nullptr)) {
  releaseSamplers(maxTextureUnits());
  return false;
 }
 if(present) {
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java (renderFinalPass)
  finalPass.finish(targets, wroteToScreen);
 }
 if(!present) {
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), static_cast<unsigned int>(destinationFramebuffer));
  core::viewport(0, 0, width, height);
 }
 releaseSamplers(maxTextureUnits());
 core::activeTexture(gl::tex::Texture0);
 publishShadowColorReadSide();
 return present ? wroteToScreen : executed;
}

} // namespace net::minecraft::client::render
