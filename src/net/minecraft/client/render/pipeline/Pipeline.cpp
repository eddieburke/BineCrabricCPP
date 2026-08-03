#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/pipeline/CompositeRenderer.hpp"
#include "net/minecraft/client/render/pipeline/FinalPassRenderer.hpp"
#include "net/minecraft/client/render/pipeline/FullscreenPass.hpp"
#include "net/minecraft/client/render/ColorSpace.hpp"
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/PbrTextures.hpp"
#include "net/minecraft/client/render/targets/ShadowMapPass.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/shaders/ShaderFail.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/targets/RenderTargets.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/resource/pack/TexturePack.hpp"
#include "net/minecraft/client/resource/pack/TexturePacks.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

namespace net::minecraft::client::render {
using LogLevel = ::net::minecraft::util::logging::LogLevel;
namespace {
using PackCatalog::lower;
void uploadRgbaStub(gl::GlTexture& texture, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
 if(!texture) {
  texture = gl::GlTexture(static_cast<unsigned int>(core::genTexture()));
  if(!texture) return;
 }
 const unsigned char pixel[4] = {r, g, b, a};
 core::bindTexture(static_cast<int>(texture.handle()));
 ::glTexImage2D(0x0DE1, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, pixel);
 ::glTexParameteri(0x0DE1, 0x2801, 0x2600);
 ::glTexParameteri(0x0DE1, 0x2800, 0x2600);
 ::glTexParameteri(0x0DE1, 0x2802, 0x812F);
 ::glTexParameteri(0x0DE1, 0x2803, 0x812F);
}

std::string trim(std::string_view value) {
 const std::size_t first = value.find_first_not_of(" \t\r\n");
 if(first == std::string_view::npos) return {};
 const std::size_t last = value.find_last_not_of(" \t\r\n");
 return std::string(value.substr(first, last - first + 1));
}

std::pair<bool, bool> pbrFormat(const resource::pack::TexturePack* pack) {
 if(pack == nullptr) return {};
 const std::vector<std::uint8_t> bytes = pack->getResource("texture.properties");
 std::istringstream stream{std::string(bytes.begin(), bytes.end())};
 for(std::string line; std::getline(stream, line);) {
  line = trim(line);
  if(line.empty() || line.front() == '#' || line.front() == '!') continue;
  const std::size_t separator = line.find_first_of("=:");
  if(separator == std::string::npos || lower(trim(line.substr(0, separator))) != "format") continue;
  const std::string format = lower(trim(line.substr(separator + 1)));
  return {format == "lab-pbr" || format.starts_with("lab-pbr/"), format == "lab-pbr/1.3"};
 }
 return {};
}

}

Pipeline::Pipeline(option::GameOptions* options) : options_(options) {}

Pipeline::~Pipeline() {
 presentReadFbo_.destroy();
 colorSpace_.destroy();
}

void Pipeline::logOnce(PackInstance& pack, const std::string& message,
                       ::net::minecraft::util::logging::LogLevel level) const {
 if(!pack.logged.insert(message).second) return;
 const std::string& label =
     !pack.summary.name.empty() ? pack.summary.name
     : !pack.definition.name.empty() ? pack.definition.name
                                     : std::string("Shader pack");
 ClientLog::LOGGER.log(level, "[shaderpack:" + label + "] " + message);
}

void Pipeline::reset() {
 pipelinePhase_ = WorldPipelinePhase::None;
 packWroteToScreen_ = false;
 engineColorCorrect_ = false;
 shadowDepthTexture_ = -1;
 shadowOpaqueDepthTexture_ = -1;
 shadowColorTextureCount_ = 0;
  std::fill(std::begin(shadowColorTextures_), std::end(shadowColorTextures_), 0);
 if(!hasGlContext()) return;
 core::setActiveProgram(nullptr);
 gl::ShaderProgram::unbind();
 releaseSamplers(maxTextureUnits());
 if(gl::GLCore::bindFramebuffer != nullptr) {
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
 }
 core::activeTexture(gl::tex::Texture0);
 core::invalidateTextureBindCache();
 core::disableBlend();
 core::blendAlpha();
 core::invalidateAttribCache();
 core::advanceProgramUniforms();
}

bool Pipeline::engineOwnsColorCorrection(const PackDefinition* def) const {
 // https://shaders.properties/current/reference/shadersproperties/features/
 if(options_ == nullptr || options_->colorSpace == 0) return false;
 return def == nullptr || !def->supportsColorCorrection;
}

unsigned int Pipeline::screenDrawFramebuffer(int width, int height) {
 if(!engineColorCorrect_ || options_ == nullptr) return 0;
 colorSpace_.rebuild(width, height, colorSpaceFromOption(options_->colorSpace));
 return colorSpace_.writeFramebuffer();
}

void Pipeline::finalizeEngineColorCorrection(int screenWidth, int screenHeight) {
 if(!engineColorCorrect_ || !packWroteToScreen_ || !colorSpace_.ready()) return;
 colorSpace_.process(colorSpace_.presentTexture());
 if(!colorSpace_.blitPresentToScreen(screenWidth, screenHeight)) packWroteToScreen_ = false;
}

bool Pipeline::hasDeferredPasses(const PackInstance* activePack) const {
 if(activePack == nullptr) return false;
 if(!activePack->deferredPasses.empty()) return true;
 return std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(), [activePack](std::size_t index) {
  return ComputeDispatcher::matchesStage(activePack->definition.passes[index].name, "deferred");
 });
}

void Pipeline::ensurePbrFallbackTextures() {
 if(!normalFallbackTexture_) uploadRgbaStub(normalFallbackTexture_, 127, 127, 255, 255);
 if(!specularFallbackTexture_) uploadRgbaStub(specularFallbackTexture_, 0, 0, 0, 0);
}

void Pipeline::bindPbrTextures() {
 // Frame-boundary PBR hook (Java: beginRender -> PBRTextureManager.onNewFrame,
 // IrisRenderingPipeline.java:935). Companion textures are resolved and bound
 // per draw in the world-program uniform uploader (Manager.cpp), which keeps
 // them in sync with the current diffuse texture; this hook only guarantees
 // the 1x1 fallback state (PBRType default values) before gbuffer passes run.
 ensurePbrFallbackTextures();
 PbrTextures::onNewFrame();
}

void Pipeline::refreshResourcePackState(PackInstance* basePack,
                                               const std::vector<std::unique_ptr<PackInstance>>& packs) {
 const auto* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 const auto* selected =
     minecraft != nullptr && minecraft->texturePacks != nullptr ? minecraft->texturePacks->selected : nullptr;
 const std::string key = selected == nullptr ? std::string{} :
                         selected->key.empty() ? selected->name : selected->key;
 if(key != resourcePackKey_) {
  resourcePackKey_ = key;
  const auto [labPbr, labPbr13] = pbrFormat(selected);
  labPbr_ = labPbr;
  labPbr13_ = labPbr13;
 }
 const auto apply = [this](PackInstance* pack) {
  if(pack == nullptr) return;
  const auto setFormat = [this](auto&& self, PackDefinition& definition) -> void {
   definition.labPbr = labPbr_;
   definition.labPbr13 = labPbr13_;
   for(auto& [name, dimension] : definition.dimensionDefinitions) {
    (void)name;
    if(dimension != nullptr) self(self, *dimension);
   }
  };
  const bool changed = pack->definition.labPbr != labPbr_ || pack->definition.labPbr13 != labPbr13_;
  setFormat(setFormat, pack->rootDefinition);
  setFormat(setFormat, pack->definition);
  if(changed) {
   core::setActiveProgram(nullptr);
   if(gl::GLCore::useProgram != nullptr) gl::GLCore::useProgram(0);
   pack->resetPrograms();
   logOnce(*pack, "clearing programs (labPBR format changed)", LogLevel::Info);
  }
 };
 apply(basePack);
 for(auto& pack : packs) apply(pack.get());
}

void Pipeline::applyBlockIds(const PackDefinition* definition) {
 std::array<int, 256> ids{};
 if(definition != nullptr && definition->hasBlockProperties) ids.fill(-1);
 else
  for(int i = 0; i < 256; ++i) ids[static_cast<std::size_t>(i)] = i;
 if(definition != nullptr) {
  for(int id = 0; id < net::minecraft::block::Block::BLOCK_COUNT; ++id) {
   net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
   if(block == nullptr) {
    continue;
   }
   std::string name = PackCatalog::lower(block->getTranslationKey());
   if(name.rfind("tile.", 0) == 0) {
    name.erase(0, 5);
   }
   if(const auto found = definition->blockIds.find(name); found != definition->blockIds.end()) {
    ids[static_cast<std::size_t>(id)] = found->second;
    continue;
   }
   if(const auto found = definition->blockIds.find("minecraft:" + name);
      found != definition->blockIds.end()) {
    ids[static_cast<std::size_t>(id)] = found->second;
   }
  }
 }
 setShaderBlockIds(ids);
}

std::string Pipeline::dimensionKey(const PackInstance& pack, const net::minecraft::World* world) const {
  std::string key = "*";
  if(world != nullptr && world->dimension != nullptr) {
   if(world->dimension->id == 0) {
    key = "minecraft:overworld";
   } else if(world->dimension->id == -1) {
    key = "minecraft:the_nether";
   } else if(world->dimension->id == 1) {
    key = "minecraft:the_end";
   } else {
    key = "minecraft:dimension_" + std::to_string(world->dimension->id);
   }
  }
  if(pack.rootDefinition.dimensionDefinitions.contains(key)) return key;
  if(pack.rootDefinition.dimensionDefinitions.contains("*")) return "*";
  return {};
}

bool Pipeline::selectDimension(PackInstance& pack, const net::minecraft::World* world, bool applyIds) {
  const std::string selectedKey = dimensionKey(pack, world);
  if(pack.dimensionKey == selectedKey) return false;
  const PackDefinition* selected = selectedKey.empty()
                                       ? &pack.rootDefinition
                                       : pack.rootDefinition.dimensionDefinitions.at(selectedKey).get();
  // Drop any bound ShaderProgram* before ProgramCache wipe — otherwise uniform
  // upload / draw can call through a freed program (wild RIP on respawn/reload).
  core::setActiveProgram(nullptr);
  if(gl::GLCore::useProgram != nullptr) gl::GLCore::useProgram(0);
  const std::string transition =
      "clearing programs (dimension '" + pack.dimensionKey + "' -> '" + selectedKey + "')";
  pack.clearGpuResources();
  pack.dimensionKey = selectedKey;
  pack.definition = pack.rootDefinition;
  if(selected != &pack.rootDefinition) {
   for(const auto& [name, program] : selected->programs) pack.definition.programs[name] = program;
   for(const auto& [name, target] : selected->targets) pack.definition.targets[name] = target;
   for(const auto& [name, scale] : selected->programScales) pack.definition.programScales[name] = scale;
   for(const auto& [name, flip] : selected->flips) pack.definition.flips[name] = flip;
   for(const auto& blend : selected->bufferBlends) pack.definition.bufferBlends.push_back(blend);
   for(const auto& img : selected->images) pack.definition.images.push_back(img);
   for(const auto& tex : selected->customTextures) pack.definition.customTextures.push_back(tex);
   for(const auto& buf : selected->bufferObjects) pack.definition.bufferObjects.push_back(buf);
   for(const auto& [name, id] : selected->blockIds) pack.definition.blockIds[name] = id;
   if(selected->hasBlockProperties) pack.definition.hasBlockProperties = true;
   for(const auto& [name, id] : selected->itemIds) pack.definition.itemIds[name] = id;
   for(const auto& [name, id] : selected->entityIds) pack.definition.entityIds[name] = id;
   pack.definition.gbufferColorBuffers =
       std::max(pack.definition.gbufferColorBuffers, selected->gbufferColorBuffers);
   pack.definition.shadowColorBuffers = std::max(pack.definition.shadowColorBuffers, selected->shadowColorBuffers);
   // A dimension folder overriding the root's shadow resolution is legal but almost
   // always a scanning bug rather than intent: the pack-wide const usually lives in a
   // shared include (shaders/prelude/), so a dimension that reports a *different* number
   // is reporting one it failed to see. Shout about it — this silently allocated a 1024
   // shadow map against GLSL compiled for 2048, which reads as shadow acne.
   if(selected->shadowMapResolution > 0) {
    if(pack.definition.shadowMapResolution > 0 &&
       pack.definition.shadowMapResolution != selected->shadowMapResolution) {
     logOnce(pack,
             "dimension shadowMapResolution " + std::to_string(selected->shadowMapResolution) +
                 " overrides pack-wide " + std::to_string(pack.definition.shadowMapResolution) +
                 "; the shadow texture and the pack's compiled shadowMapResolution const must agree",
             LogLevel::Warning);
    }
    pack.definition.shadowMapResolution = selected->shadowMapResolution;
   }
   if(!selected->customUniforms.empty()) {
    pack.definition.customUniforms = selected->customUniforms;
   }
   for(const PackPass& pass : selected->passes) {
    const auto match = std::find_if(pack.definition.passes.begin(), pack.definition.passes.end(),
                                    [&pass](const PackPass& root) {
                                     return root.type == pass.type && root.name == pass.name;
                                    });
    if(match == pack.definition.passes.end()) pack.definition.passes.push_back(pass);
    else *match = pass;
   }
  }
  std::string customError;
  if(!pack.rebuildRuntime(customError)) logOnce(pack, customError, LogLevel::Warning);
  logOnce(pack, transition, LogLevel::Info);
  if(applyIds) {
   applyBlockIds(&pack.definition);
  }
  return true;
}

bool Pipeline::preparePackResources(PackInstance& pack, int width, int height) {
 // The pipeline runs unconditionally: every pack renders through the scene targets,
 // even when it declares no post stages beyond the gbuffer passes (final-only packs
 // still present via the colortex0 blit).
 if(!ensureSceneTargets(&pack, width, height)) return false;
 return PackResources::ensure(pack, width, height, lightmapTexture_,
                              [](const PackInstance& candidate, const std::string& path) {
                               return PackCompiler::readText(candidate, path);
                              });
}

void Pipeline::prepareFrame(net::minecraft::World* world, PackInstance* activePack,
                            PackInstance* basePack) {
 bindPbrTextures();

 const auto preparePrograms = [this](PackInstance* pack) {
  if(pack == nullptr) return;
  if(pack->programState != PackProgramState::Cold) return;
  PackCompiler::prewarm(*pack, [this](PackInstance& p, const std::string& message, LogLevel level) {
   logOnce(p, message, level);
  });
 };

 preparePrograms(activePack);
 if(basePack != activePack) preparePrograms(basePack);

 const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 const int width = minecraft != nullptr ? std::max(1, minecraft->displayWidth) : 1;
 const int height = minecraft != nullptr ? std::max(1, minecraft->displayHeight) : 1;
 if(worldUniforms_.viewWidth < 1.0f) {
  worldUniforms_.viewWidth = static_cast<float>(width);
  worldUniforms_.viewHeight = static_cast<float>(height);
  worldUniforms_.aspectRatio = static_cast<float>(width) / static_cast<float>(std::max(height, 1));
 }
  if(minecraft != nullptr && minecraft->player != nullptr) {
   if(const ItemStack* held = minecraft->player->inventory.getSelectedItem()) {
    std::string name = lower(held->getTranslationKey());
    if(name.rfind("item.", 0) == 0 || name.rfind("tile.", 0) == 0) name.erase(0, 5);
    // Java IdMapUniforms.HeldItemSupplier always resolves through the
    // item.properties map (never block.properties), defaulting to -1.
    // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/IdMapUniforms.java
    worldUniforms_.heldItemId = render::resolveShaderObjectId("item", name, -1);
   }
  }
 if(minecraft != nullptr) {
  core::setTextureFilteringMode(minecraft->options.mipmapLinear ? 1 : 0);
  worldUniforms_.textureFilteringMode = minecraft->options.mipmapLinear ? 1 : 0;
 }
 core::advanceProgramUniforms();
 updateLightmap(world);

 const bool resourcesReady = activePack != nullptr && preparePackResources(*activePack, width, height);
 if(resourcesReady) {
  for(const CustomImage& declaration : activePack->definition.images) {
   const auto found = activePack->images.find(declaration.name);
   if(!declaration.clearEachFrame || found == activePack->images.end() || !found->second.texture) {
    continue;
   }
    // https://shaders.properties/current/reference/buffers/custom_images/
    if(gl::GLCore::clearTexImage != nullptr) {
     gl::GLCore::clearTexImage(found->second.texture.handle(), 0, pixelFormat(declaration.format),
                               pixelType(declaration.pixelType), nullptr);
    } else if(gl::GLCore::bindFramebuffer != nullptr && gl::GLCore::framebufferTexture2D != nullptr) {
     static gl::GlFramebuffer clearFbo;
     if(clearFbo.addColorAttachment(0, found->second.texture.handle())) {
      int prevDraw = 0;
      ::glGetIntegerv(static_cast<unsigned>(gl::query::FramebufferBinding), &prevDraw);
      clearFbo.bind();
      const float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      if(gl::GLCore::clearBufferfv != nullptr) {
       gl::GLCore::clearBufferfv(gl::framebuffer::Color, 0, zero);
      } else {
       ::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
       ::glClear(gl::attrib::ColorBufferBit);
      }
      gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer),
                                  static_cast<unsigned int>(prevDraw));
     }
    }
  }
  std::unordered_map<std::string, int> textures;
  std::unordered_map<std::string, int> colorImages;
  std::unordered_map<std::string, int> volumes;
  PackResources::addTextures(*activePack, "setup", textures, volumes);
  if(activePack->colorTargets.valid()) {
   activePack->colorTargets.fillImageBindings(colorImages);
  }
  dispatchSetupIfNeeded(*activePack, worldUniforms_, width, height, textures, colorImages, volumes,
                        &activePack->colorTargets,
                        [this](PackInstance& p, const std::string& name) {
                         return PackCompiler::compile(
                             p, name,
                             [this](PackInstance& p2, const std::string& m, LogLevel level) {
                              logOnce(p2, m, level);
                             });
                        });
 }
}

void Pipeline::refreshLightmap(net::minecraft::World* world) {
 updateLightmap(world);
}

void Pipeline::setFrameUniforms(const PackUniformValues& frame, const PackDefinition* activeDef,
                                      PackInstance* activePack) {
 worldUniforms_ = frame;
 if(activeDef != nullptr) {
  worldUniforms_.wetness = updateWetnessSmooth(worldUniforms_.rainStrength, worldUniforms_.frameTime,
                                               activeDef->wetnessHalflife, activeDef->drynessHalflife);
 } else {
  worldUniforms_.wetness = worldUniforms_.rainStrength;
 }
 if(activePack != nullptr) {
  activePack->customUniforms.evaluate(worldUniforms_);
 }
 core::advanceProgramUniforms();
}

void Pipeline::updateLightmap(const net::minecraft::World* world) {
 if(!hasGlContext()) return;
 const bool lit = world != nullptr && world->dimension != nullptr;
 const float brightness = options_ != nullptr ? std::clamp(options_->brightness, 0.0f, 1.0f) : 0.0f;
 const int ambient = world != nullptr ? world->ambientDarkness : 0;
 if(lightmapTexture_ && lightmapLit_ == lit && lightmapBrightness_ == brightness &&
    lightmapAmbient_ == ambient) {
  return;
 }
  const int previousUnit = std::max(0, core::getActiveTextureUnit());
  constexpr int kAuxUnit = 1;
  core::activeTexture(gl::tex::Texture0 + kAuxUnit);
  if(!lightmapTexture_) {
   lightmapTexture_ = gl::GlTexture(core::genTexture());
   if(!lightmapTexture_) {
    core::activeTexture(gl::tex::Texture0 + previousUnit);
    return;
   }
   core::bindTexture(static_cast<int>(lightmapTexture_.handle()));
   ::glTexParameteri(kTexture2D, 0x2801, 0x2601);
   ::glTexParameteri(kTexture2D, 0x2800, 0x2601);
   ::glTexParameteri(kTexture2D, 0x2802, 0x812F);
   ::glTexParameteri(kTexture2D, 0x2803, 0x812F);
  }
 std::array<std::uint8_t, 16 * 16 * 4> pixels{};
 for(int sky = 0; sky < 16; ++sky) {
  for(int block = 0; block < 16; ++block) {
   float value = 1.0f;
   if(lit) {
    const int effectiveSky = std::max(0, sky - ambient);
    const int level = std::clamp(std::max(block, effectiveSky), 0, 15);
    value = world->dimension->lightLevelToLuminance[static_cast<std::size_t>(level)];
    const float gamma = 1.0f - std::pow(1.0f - value, 4.0f);
    value = std::clamp(value + (gamma - value) * brightness, 0.0f, 1.0f);
   }
   const std::uint8_t channel = static_cast<std::uint8_t>(std::lround(value * 255.0f));
   const std::size_t offset = static_cast<std::size_t>((sky * 16 + block) * 4);
   pixels[offset] = channel;
   pixels[offset + 1] = channel;
   pixels[offset + 2] = channel;
   pixels[offset + 3] = 255;
  }
 }
 core::bindTexture(static_cast<int>(lightmapTexture_.handle()));
 ::glTexImage2D(kTexture2D, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, pixels.data());
 core::activeTexture(gl::tex::Texture0 + previousUnit);
 lightmapLit_ = lit;
 lightmapAmbient_ = ambient;
 lightmapBrightness_ = brightness;
}

std::vector<ColorFormat> Pipeline::sceneColorFormats(const PackInstance* activePack) const {
 int count = activePack != nullptr && activePack->summary.valid
                 ? std::clamp(activePack->definition.gbufferColorBuffers, 1, 32)
                 : 1;
 if(activePack != nullptr) {
  for(const auto& [name, target] : activePack->definition.targets) {
   (void)target;
   if(name.rfind("colortex", 0) != 0) continue;
   count = std::max(count, std::atoi(name.c_str() + 8) + 1);
  }
  count = std::clamp(count, 1, 32);
 }
 std::vector<ColorFormat> formats;
 formats.reserve(static_cast<std::size_t>(count));
 for(int i = 0; i < count; ++i) {
  ColorFormat format = ColorFormat::Rgba8;
  if(activePack != nullptr) {
   const auto found = activePack->definition.targets.find("colortex" + std::to_string(i));
   if(found != activePack->definition.targets.end()) format = parseFormat(found->second.format);
  }
  formats.push_back(format);
 }
 return formats;
}

bool Pipeline::ensureSceneTargets(PackInstance* activePack, int width, int height) {
 if(activePack == nullptr) return false;
 const std::vector<ColorFormat> formats = sceneColorFormats(activePack);
 if(!activePack->colorTargets.ensure(width, height, formats)) {
  logOnce(*activePack,
          "ensure color targets failed " + std::to_string(width) + "x" + std::to_string(height) +
              " count=" + std::to_string(formats.size()),
          LogLevel::Severe);
  return false;
 }
 for(const auto& [name, target] : activePack->definition.targets) {
  if(name.rfind("colortex", 0) != 0) continue;
  int tw = width;
  int th = height;
  if(target.absoluteWidth > 0 && target.absoluteHeight > 0) {
   tw = target.absoluteWidth;
   th = target.absoluteHeight;
  } else {
   const float sx = target.scaleX > 0.0f ? target.scaleX : target.scale;
   const float sy = target.scaleY > 0.0f ? target.scaleY : target.scale;
   if(sx >= 0.999f && sy >= 0.999f && sx <= 1.001f && sy <= 1.001f) continue;
   tw = std::max(1, static_cast<int>(std::lround(static_cast<float>(width) * sx)));
   th = std::max(1, static_cast<int>(std::lround(static_cast<float>(height) * sy)));
  }
  if(tw == width && th == height) continue;
  const ColorFormat format = parseFormat(target.format);
  if(!activePack->colorTargets.ensureNamed(name, tw, th, format)) {
   logOnce(*activePack,
           "size.buffer." + name + " allocate failed format=" + colorFormatName(format) +
               " size=" + std::to_string(tw) + "x" + std::to_string(th),
           LogLevel::Severe);
   return false;
  }
 }
 {
  std::ostringstream slots;
  for(int i = 0; i < activePack->colorTargets.colorCount(); ++i) {
   if(i) slots << ", ";
   const std::string name = "colortex" + std::to_string(i);
   slots << name << '=' << colorFormatName(activePack->colorTargets.formatOf(name));
  }
  // Omit main/alt texture ids — they flip each compute pass and would defeat logOnce.
  logOnce(*activePack, "scene targets " + std::to_string(width) + "x" + std::to_string(height) + " {" +
                           slots.str() + "} depthtex");
 }
 return true;
}

void Pipeline::bindScene(PackInstance* activePack) {
 pipelinePhase_ = WorldPipelinePhase::World;
 if(activePack != nullptr) activePack->colorTargets.bindGbuffers();
}

void Pipeline::endScene(PackInstance* activePack) {
 if(activePack != nullptr) activePack->colorTargets.endGbuffers();
}

int Pipeline::sceneColorCount(const PackInstance* activePack) const {
 return activePack != nullptr ? activePack->colorTargets.colorCount() : 0;
}

unsigned int Pipeline::sceneDepthTexture(const PackInstance* activePack) const {
 return activePack != nullptr ? activePack->colorTargets.depthTexture() : 0u;
}

void Pipeline::clearScene(PackInstance* activePack, float fogR, float fogG, float fogB) {
 if(activePack == nullptr || !activePack->colorTargets.valid()) return;
 auto& scene = activePack->colorTargets;
 std::vector<bool> clear(static_cast<std::size_t>(scene.colorCount()), true);
 std::vector<std::array<float, 4>> colors(static_cast<std::size_t>(scene.colorCount()));
 for(int i = 0; i < scene.colorCount(); ++i) {
  if(i == 0) colors[static_cast<std::size_t>(i)] = {fogR, fogG, fogB, 1.0f};
  else if(i == 1) colors[static_cast<std::size_t>(i)] = {1.0f, 1.0f, 1.0f, 1.0f};
  else colors[static_cast<std::size_t>(i)] = {0.0f, 0.0f, 0.0f, 0.0f};
  const auto found = activePack->definition.targets.find("colortex" + std::to_string(i));
  if(found == activePack->definition.targets.end()) continue;
  clear[static_cast<std::size_t>(i)] = found->second.clear;
  if(found->second.customClearColor) {
   std::copy(std::begin(found->second.clearColor), std::end(found->second.clearColor),
             colors[static_cast<std::size_t>(i)].begin());
  }
 }
  const bool fullClear = scene.fullClearRequired();
  scene.clearColors(clear, colors);
 // Named (non-colortex) buffers are a C++ extension; apply the same ClearPassCreator
 // policy (clear flag, custom clear color, black default, force on full clear).
 for(const auto& [name, target] : activePack->definition.targets) {
  if(name.rfind("colortex", 0) == 0 || name.rfind("shadowcolor", 0) == 0 || name == "screen") continue;
  if(!fullClear && !target.clear) continue;
  std::array<float, 4> color = {0.0f, 0.0f, 0.0f, 0.0f};
  if(target.customClearColor) {
   std::copy(std::begin(target.clearColor), std::end(target.clearColor), color.begin());
  }
  scene.clearNamedColors(name, color);
 }
}

void Pipeline::sampleCenterDepth(PackInstance* activePack, const PackDefinition* activeDef) {
 if(activePack == nullptr || !activePack->colorTargets.valid()) return;
 const auto& targets = activePack->colorTargets;
 if(targets.width() <= 0 || targets.height() <= 0 || targets.depthTexture() == 0) return;
 bindScene(activePack);
 float depth = 1.0f;
 ::glReadPixels(targets.width() / 2, targets.height() / 2, 1, 1, 0x1902, 0x1406, &depth);

 const float nearPlane = worldUniforms_.nearPlane > 0.0f ? worldUniforms_.nearPlane : 0.05f;
 const float farPlane = worldUniforms_.farPlane > nearPlane ? worldUniforms_.farPlane : 256.0f;
 const float halfLife = activeDef != nullptr ? activeDef->centerDepthHalflife : 1.0f;
 worldUniforms_.centerDepthSmooth =
     updateCenterDepthSmooth(depth, nearPlane, farPlane, worldUniforms_.frameTime, halfLife);
}

void Pipeline::captureOpaqueDepth(PackInstance* activePack) {
 // Pre-hand copy → depthTextures[1] → depthtex2.
 // https://shaders.properties/current/reference/buffers/depthtex/
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisRenderingPipeline.java
 captureDepth(activePack, 1);
}

void Pipeline::captureHandDepth(PackInstance* activePack) {
 // Pre-translucent copy → depthTextures[0] → depthtex1.
 // https://shaders.properties/current/reference/buffers/depthtex/
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisRenderingPipeline.java
 captureDepth(activePack, 0);
}

void Pipeline::captureDepth(PackInstance* activePack, std::size_t index) {
 if(activePack == nullptr || !activePack->colorTargets.valid() || index >= 2) return;
 const int width = activePack->colorTargets.width();
 const int height = activePack->colorTargets.height();
 if(width <= 0 || height <= 0 || activePack->colorTargets.depthTexture() == 0) return;
 bindScene(activePack);
 if(!activePack->depthTextures[index]) {
  activePack->depthTextures[index] = gl::GlTexture(core::genTexture());
  activePack->depthTextureW[index] = 0;
  activePack->depthTextureH[index] = 0;
 }
 if(!activePack->depthTextures[index]) return;
 core::bindTexture(static_cast<int>(activePack->depthTextures[index].handle()));
 if(activePack->depthTextureW[index] != width || activePack->depthTextureH[index] != height) {
  ::glTexImage2D(kTexture2D, 0, 0x81A6, width, height, 0, 0x1902, 0x1405, nullptr);
  ::glTexParameteri(kTexture2D, 0x2801, 0x2600);
  ::glTexParameteri(kTexture2D, 0x2800, 0x2600);
  ::glTexParameteri(kTexture2D, 0x2802, 0x812F);
  ::glTexParameteri(kTexture2D, 0x2803, 0x812F);
  activePack->depthTextureW[index] = width;
  activePack->depthTextureH[index] = height;
 }
 ::glCopyTexSubImage2D(kTexture2D, 0, 0, 0, 0, 0, width, height);
}

gl::ShaderProgram* Pipeline::programFromPack(PackInstance& pack, const std::string& key) {
 return pack.summary.valid && pack.definition.programs.contains(key)
            ? PackCompiler::compile(pack, key,
                                    [this](PackInstance& p, const std::string& m, LogLevel level) {
                                     logOnce(p, m, level);
                                    })
            : nullptr;
}

gl::ShaderProgram* Pipeline::worldProgram(const std::string& key, PackInstance* pack) {
 lastWorldProgramKey_ = key;
 // The applier (RenderType setupRenderState) keys blend/alphaTest directives by the
 // RESOLVED program source name (Java ProgramDirectives.java:80-83 reads the resolved
 // ProgramSource); publish the final key here and clear it on every failure path so the
 // applier never re-derives it or applies stale directives for a skipped draw.
 resolvedWorldProgramKey_.clear();
 if(pack == nullptr) return nullptr;

 core::RenderStage renderStage = core::RenderStage::None;
 if(key == "gbuffers_terrain_solid") {
  renderStage = core::RenderStage::TerrainSolid;
 } else if(key == "gbuffers_terrain_cutout" || key == "gbuffers_damagedblock") {
  renderStage = core::RenderStage::TerrainCutout;
 } else if(key.rfind("gbuffers_entities", 0) == 0 || key == "gbuffers_item" || key == "gbuffers_lightning") {
  renderStage = core::RenderStage::Entities;
 } else if(key.rfind("gbuffers_block", 0) == 0) {
  renderStage = core::RenderStage::BlockEntities;
 } else if(key == "gbuffers_hand") {
  renderStage = core::RenderStage::HandSolid;
 } else if(key == "gbuffers_hand_water") {
  renderStage = core::RenderStage::HandTranslucent;
 } else if(key == "gbuffers_water") {
  renderStage = core::RenderStage::TerrainTranslucent;
 } else if(key.rfind("gbuffers_particles", 0) == 0) {
  renderStage = core::RenderStage::Particles;
 } else if(key == "gbuffers_clouds") {
  renderStage = core::RenderStage::Clouds;
 } else if(key == "gbuffers_weather") {
  renderStage = core::RenderStage::RainSnow;
 } else if(key.rfind("gbuffers_sky", 0) == 0) {
  renderStage = core::RenderStage::Sky;
 } else if(key == "gbuffers_line") {
  renderStage = core::renderStage();
 }
 core::setRenderStage(renderStage);

 if(key.rfind("clrwl_", 0) == 0) return nullptr;

 const bool shadowPass = RenderCameraState::instance().frame().shadowPass;
 std::string programKey = shadowPass ? irisShadowProgramForGbuffers(key) : key;
 if(programKey.empty()) return nullptr;
 programKey = resolveProgramKey(pack->definition, pack->settings, programKey, pack->programEnabledCache);
 if(programKey.empty()) return nullptr;
 resolvedWorldProgramKey_ = programKey;
 gl::ShaderProgram* program = programFromPack(*pack, programKey);
 return program;
}

bool Pipeline::renderBegin(PackInstance* activePack, int shadowDepthTextureId,
                                  int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds,
                                  int shadowColorTextureCount, shadowmap::ShadowTargets* shadowTargets,
                                  const int* shadowColorAltTextureIds) {
 if(activePack == nullptr) return false;
 activePack->publishedTextures.clear();
 shadowTargets_ = shadowTargets;
 shadowDepthTexture_ = shadowDepthTextureId;
 shadowOpaqueDepthTexture_ = shadowOpaqueDepthTextureId;
 shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
 for(int index = 0; index < shadowColorTextureCount_; ++index) {
  shadowColorTextures_[index] = shadowColorTextureIds == nullptr ? -1 : shadowColorTextureIds[index];
  shadowColorAltTextures_[index] = shadowColorAltTextureIds == nullptr ? -1 : shadowColorAltTextureIds[index];
 }
 // The shadow color flip state is static per pack (Iris computes it once when the
 // shadow composite renderer is created); recompute when the pack changes so the
 // shadow map render's gbuffer shaders sample the correct static flip side.
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowCompositeRenderer.java
 if(shadowTargets_ != nullptr && shadowColorFlipPack_ != activePack) {
  shadowColorFlipPack_ = activePack;
  initShadowColorFlips(*activePack);
 }
  // Java's BEGIN stage runs before the shadow map render (onBeginClear →
  // beginRenderer.renderAll, IrisRenderingPipeline.java:1022) and samples the shadow
  // targets as they exist at that point — the previous frame's maps (Iris keeps the
  // targets across frames). The caller passes the last frame's ids; on the very first
  // frame they are -1 (the C++ targets are created lazily by the first map render,
  // where Java creates them at pipeline construction).
  return CompositeRenderer(this).render(*activePack, activePack->beginPasses, false, "begin",
                                        shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                        shadowColorTextureIds, shadowColorTextureCount,
                                        shadowTargets_, shadowColorAltTextures_);
}

bool Pipeline::renderShadowComposite(PackInstance* activePack, int shadowDepthTextureId,
                                            int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds,
                                            int shadowColorTextureCount, shadowmap::ShadowTargets* shadowTargets,
                                            const int* shadowColorAltTextureIds) {
 if(activePack == nullptr) return false;
 shadowTargets_ = shadowTargets;
 shadowDepthTexture_ = shadowDepthTextureId;
 shadowOpaqueDepthTexture_ = shadowOpaqueDepthTextureId;
 shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
 for(int index = 0; index < shadowColorTextureCount_; ++index) {
  shadowColorTextures_[index] = shadowColorTextureIds == nullptr ? -1 : shadowColorTextureIds[index];
  shadowColorAltTextures_[index] = shadowColorAltTextureIds == nullptr ? -1 : shadowColorAltTextureIds[index];
 }
 if(shadowTargets_ != nullptr && shadowColorFlipPack_ != activePack) {
  shadowColorFlipPack_ = activePack;
  initShadowColorFlips(*activePack);
 }
 const auto hasCompute = [activePack](const std::string& stage) {
  return std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(),
                     [activePack, &stage](std::size_t index) {
                      return ComputeDispatcher::matchesStage(activePack->definition.passes[index].name, stage);
                     });
 };
 {
  const auto& def = activePack->definition;
  // https://shaders.properties/current/reference/constants/shadow_mipmaps/
  auto generate = [&def](int textureId, bool nearest) {
   if(textureId <= 0 || gl::GLCore::generateMipmap == nullptr) return;
   core::bindTexture(kTexture2D, textureId);
   const int minFilter = nearest ? gl::filter::NearestMipmapNearest : gl::filter::LinearMipmapLinear;
   const int magFilter = nearest ? gl::filter::Nearest : gl::filter::Linear;
   ::glTexParameteri(kTexture2D, gl::tex::MinFilter, minFilter);
   ::glTexParameteri(kTexture2D, gl::tex::MagFilter, magFilter);
   gl::GLCore::generateMipmap(kTexture2D);
  };
  if(def.shadowtexMipmap[0]) generate(shadowDepthTextureId, def.shadowtexNearest[0]);
  if(def.shadowtexMipmap[1]) generate(shadowOpaqueDepthTextureId, def.shadowtexNearest[1]);
  for(int i = 0; i < 8 && i < shadowColorTextureCount; ++i) {
   if(def.shadowcolorMipmap[i] && shadowColorTextureIds != nullptr) {
    generate(shadowColorTextureIds[i], def.shadowcolorNearest[i]);
   }
  }
 }
 // Java runs the shadow composite stage after the shadow map render (ShadowRenderer
 // renderShadows → compositeRenderer.renderAll, ShadowRenderer.java:632) and only when
 // the shadow renderer is active (no shadow map this frame → no shadowcomp stage, like
 // Iris' renderShadows with null shadow renderer).
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisRenderingPipeline.java
 // Java derives shadowMapResolution from the pack directives at pipeline construction
 // (IrisRenderingPipeline.java:315), while the C++ frame uniform set is built before
 // the map render and carries the previous frame's resolution; fill in the pack
 // constant so the shadowcomp viewport and uniform are right on the first frame.
 if(worldUniforms_.shadowMapResolution <= 0.0f && activePack->definition.shadowMapResolution > 0) {
  worldUniforms_.shadowMapResolution = static_cast<float>(activePack->definition.shadowMapResolution);
 }
  bool rendered = false;
  if(shadowTargets_ != nullptr && shadowDepthTextureId >= 0 &&
     (!activePack->shadowCompositePasses.empty() || hasCompute("shadowcomp"))) {
   rendered = CompositeRenderer(this).render(*activePack, activePack->shadowCompositePasses, false, "shadowcomp",
                                             shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                             shadowColorTextureIds, shadowColorTextureCount,
                                             shadowTargets_, shadowColorAltTextures_) ||
              rendered;
  }
  return rendered;
}

bool Pipeline::renderPreWorld(PackInstance* activePack, int shadowDepthTextureId,
                                     int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds,
                                     int shadowColorTextureCount, shadowmap::ShadowTargets* shadowTargets,
                                     const int* shadowColorAltTextureIds) {
  if(activePack == nullptr) return false;
  shadowTargets_ = shadowTargets;
  shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
  for(int index = 0; index < shadowColorTextureCount_; ++index) {
   shadowColorTextures_[index] = shadowColorTextureIds == nullptr ? -1 : shadowColorTextureIds[index];
   shadowColorAltTextures_[index] = shadowColorAltTextureIds == nullptr ? -1 : shadowColorAltTextureIds[index];
  }
  // The shadow color flip state is static per pack (Iris computes it once when the
  // shadow composite renderer is created); the normal path initializes it in
  // renderBegin (before the shadow map render); this guard covers direct callers.
  if(shadowTargets_ != nullptr && shadowColorFlipPack_ != activePack) {
   shadowColorFlipPack_ = activePack;
   initShadowColorFlips(*activePack);
  }
  const auto hasCompute = [activePack](const std::string& stage) {
  return std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(),
                     [activePack, &stage](std::size_t index) {
                      return ComputeDispatcher::matchesStage(activePack->definition.passes[index].name, stage);
                     });
 };
  bool rendered = false;
  if(!activePack->preparePasses.empty() || hasCompute("prepare")) {
   rendered = CompositeRenderer(this).render(*activePack, activePack->preparePasses, false, "prepare",
                                             shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                             shadowColorTextureIds, shadowColorTextureCount,
                                             shadowTargets_, shadowColorAltTextures_) ||
              rendered;
  }
  shadowDepthTexture_ = shadowDepthTextureId;
  shadowOpaqueDepthTexture_ = shadowOpaqueDepthTextureId;
  // World-pass gbuffer shaders and the final pass read the static final flip side,
  // like Iris' dynamic samplers (flipped == null → getColorTextureId).
  if(shadowColorFlipPack_ == activePack) {
   for(int index = 0; index < shadowColorTextureCount_; ++index) {
    if(shadowColorFlipped_[static_cast<std::size_t>(index)] && shadowColorAltTextures_[index] >= 0) {
     shadowColorTextures_[index] = shadowColorAltTextures_[index];
    }
   }
  }
  worldUniforms_.shadowAvailable = shadowDepthTextureId >= 0 ? 1 : 0;
  worldUniforms_.normalAvailable = activePack->colorTargets.colorCount() > 1 ? 1 : 0;
  core::advanceProgramUniforms();
  return rendered;
}

void Pipeline::initShadowColorFlips(const PackInstance& pack) {
  // Mirrors ShadowCompositeRenderer's constructor: explicit shadowcomp_pre flips are
  // applied first, then each shadow composite pass flips the buffers it writes (unless
  // the pass's own flip directive says false) and every explicit true directive flips
  // its buffer even when the pass does not write it (Java's drawBuffers loop followed by
  // the explicitFlips sweep). The per-pass states are snapshotted so every pass reads
  // and writes the same static flip-side textures; the state is never mutated during
  // rendering (the old per-pass applyWroteFlips toggles were vestigial).
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowCompositeRenderer.java
  shadowColorFlipped_.fill(false);
  shadowCompPassReadFlips_.clear();
  constexpr std::string_view prePrefix = "shadowcomp_pre.";
  for(const auto& [binding, shouldFlip] : pack.definition.flips) {
   if(!shouldFlip || binding.rfind(prePrefix, 0) != 0) continue;
   const int index = shadowColorIndex(binding.substr(prePrefix.size()));
   if(index >= 0) shadowColorFlipped_[static_cast<std::size_t>(index)] =
                         !shadowColorFlipped_[static_cast<std::size_t>(index)];
  }
   for(std::size_t passIndex : pack.shadowCompositePasses) {
    if(passIndex >= pack.definition.passes.size()) continue;
    const PackPass& pass = pack.definition.passes[passIndex];
    shadowCompPassReadFlips_.push_back(shadowColorFlipped_);
    std::vector<std::string> outputs =
        pass.outputs.empty() ? std::vector<std::string>{"shadowcolor0"} : pass.outputs;
    for(const std::string& name : outputs) {
     const int index = shadowColorIndex(name);
     if(index < 0) continue;
     bool flip = true;
     if(const auto declared = pack.definition.flips.find(pass.name + "." + name);
        declared != pack.definition.flips.end()) {
      flip = declared->second;
     }
     if(flip) {
      shadowColorFlipped_[static_cast<std::size_t>(index)] =
          !shadowColorFlipped_[static_cast<std::size_t>(index)];
     }
    }
    for(const std::string& name : ColorTargets::explicitTrueFlips(pack.definition, pass.name)) {
     const int index = shadowColorIndex(name);
     if(index < 0) continue;
     shadowColorFlipped_[static_cast<std::size_t>(index)] =
         !shadowColorFlipped_[static_cast<std::size_t>(index)];
    }
   }
  }

bool Pipeline::renderDeferred(PackInstance* activePack, int shadowDepthTextureId,
                                    int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds,
                                    int shadowColorTextureCount, const int* shadowColorAltTextureIds) {
  if(activePack == nullptr) return false;
  const bool hasComputeDeferred =
      std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(), [activePack](std::size_t index) {
       return ComputeDispatcher::matchesStage(activePack->definition.passes[index].name, "deferred");
      });
  if(activePack->deferredPasses.empty() && !hasComputeDeferred) return false;
  // TEMP DIAGNOSTIC — pre-deferred sky-pixel readback. What does colortex1's sky hold
  // BEFORE the deferred dispatch (vs my [post-probe] after the whole chain)? Plus the
  // scene depth at that pixel (is it sky=1.0 or geometry?) and the depth texture id.
  {
   static int preDeferredFrames = 0;
   const int pre = preDeferredFrames++;
   if(pre < 3 || pre % 90 == 0) {
    auto& targets = activePack->colorTargets;
    const int px = std::max(1, targets.width() / 2);
    const int py = std::max(1, targets.height() * 3 / 4);
    float rgba[4]{};
    ::glReadPixels(px, py, 1, 1, 0x1908, 0x1406, rgba);
    float depth = -1.0f;
    if(targets.depthTexture() != 0) {
     targets.bindGbuffers();
     ::glReadPixels(px, py, 1, 1, 0x1902, 0x1406, &depth);
     targets.endGbuffers();
    }
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[pre-deferred-probe] frame=") + std::to_string(pre) + " colortex1 pre-deferred sky=(" +
            std::to_string(rgba[0]) + "," + std::to_string(rgba[1]) + "," + std::to_string(rgba[2]) +
            ") depth=" + std::to_string(depth));
   }
  }
  // END TEMP DIAGNOSTIC
  return CompositeRenderer(this).render(*activePack, activePack->deferredPasses, false, "deferred",
                                        shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                        shadowColorTextureIds, shadowColorTextureCount,
                                        shadowTargets_, shadowColorAltTextureIds);
}

bool Pipeline::renderPostProcess(PackInstance* activePack, PackInstance* /*basePack*/,
                                       int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                       const int* shadowColorTextureIds, int shadowColorTextureCount,
                                       const int* shadowColorAltTextureIds) {
 if(activePack == nullptr) return false;
 engineColorCorrect_ = engineOwnsColorCorrection(&activePack->definition);
 const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 const int screenW = minecraft != nullptr ? std::max(1, minecraft->displayWidth) : 1;
 const int screenH = minecraft != nullptr ? std::max(1, minecraft->displayHeight) : 1;
  if(engineColorCorrect_) {
   // Pre-create the color-space write FBO before the composite/final stage runs: the
   // call's purpose is the colorSpace_.rebuild() side effect (allocates the present +
   // write targets); the returned framebuffer is consumed by presentFinalToScreen later.
   (void)screenDrawFramebuffer(std::max(1, activePack->colorTargets.width()),
                               std::max(1, activePack->colorTargets.height()));
  }
 const bool hasComputeComposite =
     std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(), [activePack](std::size_t index) {
      const std::string& name = activePack->definition.passes[index].name;
      return ComputeDispatcher::matchesStage(name, "composite") ||
             ComputeDispatcher::matchesStage(name, "final");
     });
   if(!activePack->postPasses.empty() || hasComputeComposite || !activePack->setupPasses.empty()) {
    CompositeRenderer(this).render(*activePack, activePack->postPasses, true, "composite",
                                   shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                   shadowColorTextureIds, shadowColorTextureCount,
                                   shadowTargets_, shadowColorAltTextureIds);
   }
  // TEMP DIAGNOSTIC — [renderpearl-probe] exposure SSBO, compute inventory, fog and
  // shadow light-axis sanity. Runs after the composite stage has executed. DELETE
  // after investigation.
  {
   static int probeFrames = 0;
   const int probe = probeFrames++;
   if(probe < 5 || probe % 90 == 0) {
    const auto halfToFloat = [](std::uint16_t h) {
     const std::uint32_t sign = static_cast<std::uint32_t>(h & 0x8000u) << 16;
     const std::uint32_t exp = (h >> 10) & 0x1Fu;
     const std::uint32_t man = h & 0x3FFu;
     if(exp == 0) return 0.0f;
     std::uint32_t bits = sign | ((exp - 15 + 127) << 23) | (man << 13);
     float f = 0.0f;
     std::memcpy(&f, &bits, 4);
     return f;
    };
    std::string computeList;
    for(std::size_t index : activePack->computePasses) {
     const PackPass& pass = activePack->definition.passes[index];
     const bool compiled = programFromPack(*activePack, pass.program) != nullptr;
     if(!computeList.empty()) computeList += " ";
     computeList += pass.name + ":" + std::to_string(pass.order) + (compiled ? ":ok" : ":FAIL");
    }
    std::string exposure;
    if(activePack->bufferObjects[0].handle() != 0 && activePack->bufferBytes[0] >= 6 &&
       gl::GLCore::mapBufferRange != nullptr) {
      gl::GLCore::bindBuffer(0x90D2, activePack->bufferObjects[0].handle());
      if(const void* mapped = gl::GLCore::mapBufferRange(0x90D2, 0, 6, 0x0001); mapped != nullptr) {
      const int sumLog = *static_cast<const int*>(mapped);
      std::uint16_t expBits = 0;
      std::memcpy(&expBits, static_cast<const char*>(mapped) + 4, 2);
      exposure = "sum_log_luma=" + std::to_string(sumLog) + " exposure=" +
                 std::to_string(halfToFloat(expBits)) + " (raw0x" +
                 std::to_string(static_cast<unsigned>(expBits)) + ")";
      gl::GLCore::unmapBuffer(0x90D2);
     } else {
      exposure = "map FAILED";
     }
      gl::GLCore::bindBuffer(0x90D2, 0);
    } else {
     exposure = "buffer0 missing (handle=" + std::to_string(activePack->bufferObjects[0].handle()) +
                " bytes=" + std::to_string(activePack->bufferBytes[0]) + ")";
    }
    const float* fogColor = worldUniforms_.fogColor;
    const float* smv = worldUniforms_.shadowModelView;
    const float lightAxisWorld[3] = {smv[2], smv[6], smv[10]};
    net::minecraft::util::math::Matrix4f gbufferModelView;
    gbufferModelView.set(worldUniforms_.gbufferModelView);
    float axisView[3] = {0.0f, 0.0f, 0.0f};
    gbufferModelView.transformPoint(lightAxisWorld[0], lightAxisWorld[1], lightAxisWorld[2], axisView[0],
                                    axisView[1], axisView[2]);
    const float* slp = worldUniforms_.shadowLightPosition;
    float slpLen = std::sqrt(slp[0] * slp[0] + slp[1] * slp[1] + slp[2] * slp[2]);
    const float dot = slpLen > 1e-6f
                          ? (axisView[0] * slp[0] + axisView[1] * slp[1] + axisView[2] * slp[2]) / slpLen
                          : 0.0f;
    int shadowW = 0;
    if(shadowDepthTextureId > 0) {
     core::bindTexture(0x0DE1, shadowDepthTextureId);
     ::glGetTexLevelParameteriv(0x0DE1, 0, 0x1000, &shadowW);
    }
    std::string deferred =
        std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(),
                    [activePack](std::size_t index) {
                     return ComputeDispatcher::matchesStage(activePack->definition.passes[index].name, "deferred");
                    })
            ? "deferred:present"
            : "deferred:absent";
    const bool skybasic = programFromPack(*activePack, "gbuffers_skybasic") != nullptr;
    const bool finalProgram = programFromPack(*activePack, "final") != nullptr;
    ::net::minecraft::client::ClientLog::LOGGER.log(
        ::net::minecraft::util::logging::LogLevel::Info,
        std::string("[renderpearl-probe] frame=") + std::to_string(probe) + " computes[" + computeList + "] " +
            deferred + " skybasic=" + (skybasic ? "ok" : "MISSING") +
            " final=" + (finalProgram ? "ok" : "MISSING") + " " + exposure +
            " fogStart=" + std::to_string(worldUniforms_.fogStart) +
            " fogEnd=" + std::to_string(worldUniforms_.fogEnd) +
            " fogMode=" + std::to_string(worldUniforms_.fogMode) +
            " far=" + std::to_string(worldUniforms_.farPlane) +
            " fogColor=(" + std::to_string(fogColor[0]) + "," + std::to_string(fogColor[1]) + "," +
            std::to_string(fogColor[2]) + ")" + " axisView=(" + std::to_string(axisView[0]) + "," +
            std::to_string(axisView[1]) + "," + std::to_string(axisView[2]) + ")" +
            " shadowLightPos=(" + std::to_string(slp[0]) + "," + std::to_string(slp[1]) + "," +
            std::to_string(slp[2]) + ") dot(axis,light)=" + std::to_string(dot) +
            " shadowMapRes=" + std::to_string(shadowW) + " (uniform=" +
            std::to_string(worldUniforms_.shadowMapResolution) + ")");
   }
  }
  // END TEMP DIAGNOSTIC
   if(!packWroteToScreen_) presentFinalToScreen(activePack, screenW, screenH);
 finalizeEngineColorCorrection(screenW, screenH);
 return packWroteToScreen_;
}

bool Pipeline::blitColortex0ToScreen(PackInstance& pack, int screenWidth, int screenHeight) {
  render::ColorTargets& targets = pack.colorTargets;
  const unsigned int color0 = targets.readTexture(0);
  const int width = targets.width();
  const int height = targets.height();
  if(color0 == 0 || width <= 0 || height <= 0 || screenWidth <= 0 || screenHeight <= 0) return false;
 if(engineColorCorrect_) {
  if(screenDrawFramebuffer(width, height) == 0) return false;
  // Java FinalPassRenderer's copy fallback reads colortex0 through a fresh FBO and
  // blits into the color-space swap target when the engine owns the correction.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java
  presentReadFbo_.addColorAttachment(0, color0);
  presentReadFbo_.bindAsReadBuffer();
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::DrawFramebuffer),
                              colorSpace_.writeFramebuffer());
  core::viewport(0, 0, width, height);
  gl::GLCore::blitFramebuffer(0, 0, width, height, 0, 0, width, height, gl::attrib::ColorBufferBit,
                              gl::filter::Nearest);
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::ReadFramebuffer), 0);
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
  return true;
 }
 if(gl::GLCore::blitFramebuffer == nullptr) {
  return false;
 }
  presentReadFbo_.addColorAttachment(0, color0);
  presentReadFbo_.bindAsReadBuffer();
  // TEMP DIAGNOSTIC — post-processing probe. DELETE after investigation.
  // Compares the presented colortex0 against the raw gbuffer colortex1 and dumps
  // the sky-bloom custom uniforms, once per pack load per stage and every 90 frames.
  static int postProbeFrames = 0;
  const int postProbe = postProbeFrames++;
  if(postProbe < 3 || postProbe % 90 == 0) {
   const int px = std::max(1, width / 2);
   const int py = std::max(1, height / 2);
   const int skyY = std::max(1, height * 3 / 4);
   const int leftX = std::max(1, width / 4);
   float px0c[4]{}, px0s[4]{}, px0l[4]{};
   ::glReadPixels(px, py, 1, 1, 0x1908, 0x1406, px0c);
   ::glReadPixels(px, skyY, 1, 1, 0x1908, 0x1406, px0s);
   ::glReadPixels(leftX, py, 1, 1, 0x1908, 0x1406, px0l);
   float px1c[4]{}, px1s[4]{}, px1l[4]{};
   if(targets.colorCount() > 1) {
    presentReadFbo_.addColorAttachment(0, targets.readTexture(1));
    presentReadFbo_.bindAsReadBuffer();
    ::glReadPixels(px, py, 1, 1, 0x1908, 0x1406, px1c);
    ::glReadPixels(px, skyY, 1, 1, 0x1908, 0x1406, px1s);
    ::glReadPixels(leftX, py, 1, 1, 0x1908, 0x1406, px1l);
    presentReadFbo_.addColorAttachment(0, color0);
    presentReadFbo_.bindAsReadBuffer();
   }
    std::string cu;
    for(const char* name : {"sunDirectionPlr", "skyColorLinear", "skyState", "shadowLightDirection"}) {
     const auto found = pack.customUniforms.values().find(name);
     if(found == pack.customUniforms.values().end()) continue;
     if(!cu.empty()) cu += " ";
     cu += name;
     for(int k = 0; k < 3; ++k) {
      std::ostringstream f;
      f << '=' << std::fixed << std::setprecision(3) << found->second.f[k];
      cu += f.str();
     }
    }
    // Scan the top half of both buffers for the brightest pixel (sun disc / halo).
    std::vector<float> skyScan(static_cast<std::size_t>(width) * (height / 2) * 4);
    float peak0[3]{}, peak1[3]{};
    int peak0x = -1, peak0y = -1, peak1x = -1, peak1y = -1;
    if(!skyScan.empty()) {
     ::glReadPixels(0, height / 2, width, height / 2, 0x1908, 0x1406, skyScan.data());
     for(int y = 0; y < height / 2; ++y) {
      for(int x = 0; x < width; ++x) {
       const std::size_t o = static_cast<std::size_t>((y * width + x) * 4);
       const float l = skyScan[o] + skyScan[o + 1] + skyScan[o + 2];
       if(peak0x < 0 || l > peak0[0] + peak0[1] + peak0[2]) {
        peak0[0] = skyScan[o]; peak0[1] = skyScan[o + 1]; peak0[2] = skyScan[o + 2];
        peak0x = x; peak0y = y;
       }
      }
     }
     if(targets.colorCount() > 1) {
      presentReadFbo_.addColorAttachment(0, targets.readTexture(1));
      presentReadFbo_.bindAsReadBuffer();
      ::glReadPixels(0, height / 2, width, height / 2, 0x1908, 0x1406, skyScan.data());
      presentReadFbo_.addColorAttachment(0, color0);
      presentReadFbo_.bindAsReadBuffer();
      for(int y = 0; y < height / 2; ++y) {
       for(int x = 0; x < width; ++x) {
        const std::size_t o = static_cast<std::size_t>((y * width + x) * 4);
        const float l = skyScan[o] + skyScan[o + 1] + skyScan[o + 2];
        if(peak1x < 0 || l > peak1[0] + peak1[1] + peak1[2]) {
         peak1[0] = skyScan[o]; peak1[1] = skyScan[o + 1]; peak1[2] = skyScan[o + 2];
         peak1x = x; peak1y = y;
        }
       }
      }
     }
    }
   ::net::minecraft::client::ClientLog::LOGGER.log(
       ::net::minecraft::util::logging::LogLevel::Info,
       std::string("[post-probe] frame=") + std::to_string(postProbe) + " colortex0 center=(" +
           std::to_string(px0c[0]) + "," + std::to_string(px0c[1]) + "," + std::to_string(px0c[2]) +
           ") sky=(" + std::to_string(px0s[0]) + "," + std::to_string(px0s[1]) + "," + std::to_string(px0s[2]) +
           ") left=(" + std::to_string(px0l[0]) + "," + std::to_string(px0l[1]) + "," + std::to_string(px0l[2]) +
           ") colortex1 center=(" + std::to_string(px1c[0]) + "," + std::to_string(px1c[1]) + "," +
           std::to_string(px1c[2]) + ") sky=(" + std::to_string(px1s[0]) + "," + std::to_string(px1s[1]) + "," +
           std::to_string(px1s[2]) + ") left=(" + std::to_string(px1l[0]) + "," + std::to_string(px1l[1]) + "," +
           std::to_string(px1l[2]) + ")" + (cu.empty() ? std::string() : std::string(" customs[") + cu + "]") +
           " sunPeak colortex0=(" + std::to_string(peak0x) + "," + std::to_string(peak0y) + ") rgb=(" +
           std::to_string(peak0[0]) + "," + std::to_string(peak0[1]) + "," + std::to_string(peak0[2]) +
           ") colortex1=(" + std::to_string(peak1x) + "," + std::to_string(peak1y) + ") rgb=(" +
           std::to_string(peak1[0]) + "," + std::to_string(peak1[1]) + "," + std::to_string(peak1[2]) + ")");
  }
  // END TEMP DIAGNOSTIC
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::DrawFramebuffer), 0);
  core::viewport(0, 0, screenWidth, screenHeight);
  gl::GLCore::blitFramebuffer(0, 0, width, height, 0, 0, screenWidth, screenHeight, gl::attrib::ColorBufferBit,
                              gl::filter::Nearest);
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::ReadFramebuffer), 0);
  gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
  // TEMP DIAGNOSTIC — screen readback to confirm the blit presents colortex0.
  if(postProbe < 3 || postProbe % 90 == 0) {
   float scr[4]{};
   ::glReadPixels(0, 0, 1, 1, 0x1908, 0x1406, scr);
   ::net::minecraft::client::ClientLog::LOGGER.log(
       ::net::minecraft::util::logging::LogLevel::Info,
       std::string("[post-probe] screen BL=(0,0)=(") + std::to_string(scr[0]) + "," + std::to_string(scr[1]) + "," +
           std::to_string(scr[2]) + ")");
  }
  // END TEMP DIAGNOSTIC
  return true;
}

void Pipeline::presentFinalToScreen(PackInstance* scenePack, int screenWidth, int screenHeight) {
  // Owned by the final pass runner (FinalPassRenderer.java renderFinalPass); the
  // Pipeline entry is kept as a thin forwarder so external callers are untouched.
  FinalPassRenderer(this).presentToScreen(scenePack, screenWidth, screenHeight);
}

}
