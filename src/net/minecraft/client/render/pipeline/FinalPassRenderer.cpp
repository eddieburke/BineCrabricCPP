#include "net/minecraft/client/render/pipeline/FinalPassRenderer.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/shaders/ShaderFail.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::render {
using LogLevel = ::net::minecraft::util::logging::LogLevel;

void FinalPassRenderer::finish(ColorTargets& targets, bool wroteToScreen) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java
 targets.resetMipmaps();
 pipeline_->packWroteToScreen_ = wroteToScreen;
}

void FinalPassRenderer::presentToScreen(PackInstance* scenePack, int screenWidth, int screenHeight) {
 if(pipeline_->packWroteToScreen_ || scenePack == nullptr || !scenePack->colorTargets.valid()) return;
 if(scenePack->colorTargets.readTexture(0) == 0 || !hasGlContext()) return;

  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java
  if(!scenePack->definition.programs.contains("final") ||
     !isProgramEnabledCached(scenePack->definition, scenePack->settings, "final",
                             scenePack->programEnabledCache)) {
   pipeline_->logOnce(*scenePack, "no final program (missing or disabled); presenting via colortex0 blit", LogLevel::Info);
    (void)pipeline_->blitColortex0ToScreen(*scenePack, screenWidth, screenHeight);
    pipeline_->packWroteToScreen_ = true;
    return;
   }

   gl::ShaderProgram* program = pipeline_->programFromPack(*scenePack, "final");
   if(program == nullptr || !program->valid()) {
    (void)pipeline_->blitColortex0ToScreen(*scenePack, screenWidth, screenHeight);
    pipeline_->packWroteToScreen_ = true;
    return;
   }

 render::ColorTargets& targets = scenePack->colorTargets;
 const int width = targets.width();
 const int height = targets.height();
 if(width <= 0 || height <= 0) return;
 if(!PackResources::ensure(*scenePack, width, height, pipeline_->lightmapTexture_,
                                 [](const PackInstance& p, const std::string& path) {
                                  return PackCompiler::readText(p, path);
                                 })) {
  return;
 }
 const core::DepthScope depthScope(false, false);
 const core::CullScope cullScope(false);
 const core::BlendScope blendScope(false);
 const core::TextureBindScope textureScope;
  std::unordered_map<std::string, int> textures;
  targets.fillReadSamplers(textures, true);
  textures["depthtex0"] = static_cast<int>(targets.depthTexture());
  const unsigned int drawFbo = pipeline_->screenDrawFramebuffer(width, height);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), drawFbo);
 core::viewport(0, 0, pipeline_->engineColorCorrect_ ? width : screenWidth,
                pipeline_->engineColorCorrect_ ? height : screenHeight);
 applyBufferBlends(scenePack->definition, "final", {});
 applyAlphaTest(scenePack->definition, "final");
 program->bind();

 std::unordered_map<std::string, int> volumeTextures;
 putShadowTextures(textures, pipeline_->shadowDepthTexture_, pipeline_->shadowOpaqueDepthTexture_,
                           pipeline_->shadowColorTextures_,
                           pipeline_->shadowColorTextureCount_, scenePack->definition);
 PackResources::addTextures(*scenePack, "composite", textures, volumeTextures);
 bindSamplers(*program, textures, volumeTextures, maxTextureUnits(),
                      scenePack->definition);
 PackResources::bind(*scenePack, *program, 0);

 const PackUniformValues& frameUniforms = pipeline_->worldUniforms_;
 const PackViewportValues viewport{static_cast<float>(width),
                                   static_cast<float>(height),
                                   static_cast<float>(width) / std::max(static_cast<float>(height), 1.0f),
                                   frameUniforms.farPlane,
                                   frameUniforms.shadowMapResolution,
                                   frameUniforms.shadowAvailable,
                                   targets.colorCount() > 1 ? 1 : 0};
 uploadShaderUniforms(*program, frameUniforms, true, &viewport);
  uploadIdentityDrawMatrices(*program);
 scenePack->customUniforms.upload(*program);
 program->bind();
 core::drawFullscreen();
 core::unlockBlend();
 pipeline_->packWroteToScreen_ = true;
 releaseSamplers(maxTextureUnits());
 core::activeTexture(gl::tex::Texture0);
}

} // namespace net::minecraft::client::render
