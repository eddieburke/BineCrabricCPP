#include "net/minecraft/client/render/Pipeline.hpp"
#include "net/minecraft/client/render/ColorSpace.hpp"
#include "net/minecraft/client/render/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/FrameData.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/Catalog.hpp"
#include "net/minecraft/client/render/Compiler.hpp"
#include "net/minecraft/client/render/Resources.hpp"
#include "net/minecraft/client/render/PassIndex.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/ShaderFail.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"
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
using pack_catalog::lower;
using shaderpack::uploadIdentityDrawMatrices;
using shaderpack::uploadShaderUniforms;
using shaderpack::updateCenterDepthSmooth;
using shaderpack::updateWetnessSmooth;
void uploadUniforms(const gl::ShaderProgram& program, const FrameUniformSet& values) {
 uploadShaderUniforms(program, values, true);
}

void uploadRgbaStub(unsigned int& texture, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
 if(texture == 0) {
  texture = static_cast<unsigned int>(core::genTexture());
  if(texture == 0) return;
 }
 const unsigned char pixel[4] = {r, g, b, a};
 core::bindTexture(static_cast<int>(texture));
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
                      std::unordered_map<std::string, int>& colorImages, bool = false) {
 eraseColortexKeys(textures);
 eraseColortexKeys(colorImages);
 targets.fillReadSamplers(textures);
 targets.fillImageBindings(colorImages);
}

template <typename CompileFn>
bool dispatchSetupIfNeeded(PackInstance& pack, const FrameUniformSet& uniforms, int width, int height,
                           std::unordered_map<std::string, int>& textures,
                           std::unordered_map<std::string, int>& colorImages,
                           std::unordered_map<std::string, int>& volumes, CompileFn&& compileFn) {
 if(!gl::GLCore::computeSupported || (pack.setupWidth == width && pack.setupHeight == height)) {
  return true;
 }
 for(std::size_t passIndex : pack.setupPasses) {
  if(!compute::dispatch(pack, pack.definition.passes[passIndex], uniforms, textures, colorImages,
                                  volumes, width, height, !pack.definition.allowConcurrentCompute,
                                  compileFn)) {
   return false;
  }
 }
 if(pack.definition.allowConcurrentCompute && !pack.setupPasses.empty()) {
  gl::GLCore::memoryBarrier(compute::kBarrierBits);
 }
 pack.setupWidth = width;
 pack.setupHeight = height;
 return true;
}
}

Pipeline::Pipeline(option::GameOptions* options) : options_(options) {}

Pipeline::~Pipeline() {
 if(lightmapTexture_ != 0) core::deleteTexture(lightmapTexture_);
 if(normalFallbackTexture_ != 0) core::deleteTexture(normalFallbackTexture_);
 if(specularFallbackTexture_ != 0) core::deleteTexture(specularFallbackTexture_);
 if(presentReadFbo_ != 0 && gl::GLCore::deleteFramebuffers != nullptr) {
  gl::GLCore::deleteFramebuffers(1, &presentReadFbo_);
  presentReadFbo_ = 0;
 }
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
 if(!glutil::hasGlContext()) return;
 glutil::releaseSamplers(glutil::maxTextureUnits());
 if(gl::GLCore::bindFramebuffer != nullptr) {
  gl::GLCore::bindFramebuffer(0x8D40, 0);
 }
 core::activeTexture(gl::tex::Texture0);
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
  return compute::matchesStage(activePack->definition.passes[index].name, "deferred");
 });
}

bool Pipeline::activeHasPostProcess(const PackDefinition* activeDef,
                                           const PackInstance* activePack) const {
 if(activeDef == nullptr || activePack == nullptr) return false;
 return !activePack->postPasses.empty() || !activePack->deferredPasses.empty() ||
        !activePack->computePasses.empty() || !activePack->setupPasses.empty() ||
        !activePack->beginPasses.empty() || !activePack->shadowCompositePasses.empty() ||
        !activePack->preparePasses.empty();
}

void Pipeline::ensurePbrFallbackTextures() {
 if(normalFallbackTexture_ == 0) uploadRgbaStub(normalFallbackTexture_, 127, 127, 255, 255);
 if(specularFallbackTexture_ == 0) uploadRgbaStub(specularFallbackTexture_, 0, 0, 0, 0);
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
   logOnce(*pack, "clearing programs (labPBR format changed)", LogLevel::Info);
   pack->compiledPrograms.clear();
   pack->logged.clear();
   if(pack->programs != nullptr) pack->programs->clear();
  }
 };
 apply(basePack);
 for(auto& pack : packs) apply(pack.get());
}

void Pipeline::applyBlockIds(const PackDefinition* definition) {
 std::array<int, 256> ids{};
 for(int i = 0; i < 256; ++i) {
  ids[static_cast<std::size_t>(i)] = i;
 }
 if(definition != nullptr) {
  for(int id = 0; id < net::minecraft::block::Block::BLOCK_COUNT; ++id) {
   net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
   if(block == nullptr) {
    continue;
   }
   std::string name = pack_catalog::lower(block->getTranslationKey());
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

void Pipeline::prepareFrame(net::minecraft::World* world, PackInstance* activePack,
                                   PackInstance* basePack,
                                   const std::vector<std::unique_ptr<PackInstance>>& packs) {
 ensurePbrFallbackTextures();
 refreshResourcePackState(basePack, packs);

 const auto activateDimension = [this, activePack](PackInstance& pack, const net::minecraft::World* w) {
  std::string key = "*";
  if(w != nullptr && w->dimension != nullptr) {
   if(w->dimension->id == 0) {
    key = "minecraft:overworld";
   } else if(w->dimension->id == -1) {
    key = "minecraft:the_nether";
   } else if(w->dimension->id == 1) {
    key = "minecraft:the_end";
   } else {
    key = "minecraft:dimension_" + std::to_string(w->dimension->id);
   }
  }
  const auto exact = pack.rootDefinition.dimensionDefinitions.find(key);
  const auto fallback = pack.rootDefinition.dimensionDefinitions.find("*");
  const PackDefinition* selected =
      exact != pack.rootDefinition.dimensionDefinitions.end()
          ? exact->second.get()
      : fallback != pack.rootDefinition.dimensionDefinitions.end() ? fallback->second.get()
                                                                   : &pack.rootDefinition;
  const std::string selectedKey = selected == &pack.rootDefinition
                                      ? std::string{}
                                  : exact != pack.rootDefinition.dimensionDefinitions.end() ? key
                                                                                            : "*";
  if(pack.dimensionKey == selectedKey) return;
  // Drop any bound ShaderProgram* before ProgramCache wipe — otherwise uniform
  // upload / draw can call through a freed program (wild RIP on respawn/reload).
  core::setActiveProgram(nullptr);
  if(gl::GLCore::useProgram != nullptr) gl::GLCore::useProgram(0);
  logOnce(pack,
          "clearing programs (dimension '" + pack.dimensionKey + "' -> '" + selectedKey + "')",
          LogLevel::Info);
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
   if(selected->shadowMapResolution > 0) pack.definition.shadowMapResolution = selected->shadowMapResolution;
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
  pack.customUniforms.setOptions(pack.settings);
  pack.customUniforms.compile(pack.definition.customUniforms, customError);
  pack.compiledPrograms.clear();
  pack.logged.clear();
  pack.programs = std::make_unique<gl::ProgramCache>();
  pack.programEnabledCache.clear();
  PackPassBuckets buckets;
  indexPackPasses(pack.definition, pack.settings, buckets);
  pack.applyPassBuckets(std::move(buckets));
  if(&pack == activePack) {
   applyBlockIds(&pack.definition);
  }
 };

 if(activePack != nullptr) activateDimension(*activePack, world);
 if(basePack != nullptr) activateDimension(*basePack, world);

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
   worldUniforms_.heldItemId = render::resolveShaderObjectId(
       held->itemId < net::minecraft::block::Block::BLOCK_COUNT ? "block" : "item", name, 0);
  }
 }
 if(minecraft != nullptr) {
  core::setTextureFilteringMode(minecraft->options.mipmapLinear ? 1 : 0);
  worldUniforms_.textureFilteringMode = minecraft->options.mipmapLinear ? 1 : 0;
 }
 core::advanceProgramUniforms();
 updateLightmap(world);

 // activateDimension may have destroyed colorTargets after beginSceneCapture ensured
 // them. Rebuild before setup/gbuffers so RENDERTARGETS=[1,2] packs (RenderPearl) keep
 // colorCount>=needed and setup does not latch a missing-image dispatch for the session.
 if(activePack != nullptr && activeHasPostProcess(&activePack->definition, activePack)) {
  ensureSceneTargets(activePack, width, height);
 }

 const bool resourcesReady = activePack != nullptr &&
                             PackResources::ensure(*activePack, width, height, lightmapTexture_,
                                                         [](const PackInstance& p, const std::string& path) {
                                                          return PackCompiler::readText(p, path);
                                                         });
 if(resourcesReady) {
  for(const CustomImage& declaration : activePack->definition.images) {
   const auto found = activePack->images.find(declaration.name);
   if(!declaration.clearEachFrame || found == activePack->images.end() || found->second.texture == 0) {
    continue;
   }
   // https://shaders.properties/current/reference/buffers/custom_images/
   if(gl::GLCore::clearTexImage != nullptr) {
    gl::GLCore::clearTexImage(found->second.texture, 0, glutil::pixelFormat(declaration.format),
                              glutil::pixelType(declaration.pixelType), nullptr);
   } else if(gl::GLCore::genFramebuffers != nullptr && gl::GLCore::bindFramebuffer != nullptr &&
             gl::GLCore::framebufferTexture2D != nullptr) {
    static unsigned int clearFbo = 0;
    if(clearFbo == 0) gl::GLCore::genFramebuffers(1, &clearFbo);
    if(clearFbo != 0) {
     int prevDraw = 0;
     ::glGetIntegerv(0x8CA6, &prevDraw);
     gl::GLCore::bindFramebuffer(0x8D40, clearFbo);
     gl::GLCore::framebufferTexture2D(0x8D40, 0x8CE0, glutil::kTexture2D, found->second.texture, 0);
     const float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
     if(gl::GLCore::clearBufferfv != nullptr) {
      gl::GLCore::clearBufferfv(0x1800, 0, zero);
     } else {
      ::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      ::glClear(0x4000);
     }
     gl::GLCore::bindFramebuffer(0x8D40, static_cast<unsigned int>(prevDraw));
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

void Pipeline::setFrameUniforms(const FrameUniformSet& frame, const PackDefinition* activeDef,
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
 if(!glutil::hasGlContext()) return;
 const bool lit = world != nullptr && world->dimension != nullptr;
 const float brightness = options_ != nullptr ? std::clamp(options_->brightness, 0.0f, 1.0f) : 0.0f;
 const int ambient = world != nullptr ? world->ambientDarkness : 0;
 if(lightmapTexture_ != 0 && lightmapLit_ == lit && lightmapBrightness_ == brightness &&
    lightmapAmbient_ == ambient) {
  return;
 }
 const int previousUnit = std::max(0, core::getActiveTextureUnit());
 constexpr int kAuxUnit = 1;
 core::activeTexture(gl::tex::Texture0 + kAuxUnit);
 if(lightmapTexture_ == 0) {
  lightmapTexture_ = core::genTexture();
  if(lightmapTexture_ == 0) {
   core::activeTexture(gl::tex::Texture0 + previousUnit);
   return;
  }
  core::bindTexture(static_cast<int>(lightmapTexture_));
  ::glTexParameteri(glutil::kTexture2D, 0x2801, 0x2601);
  ::glTexParameteri(glutil::kTexture2D, 0x2800, 0x2601);
  ::glTexParameteri(glutil::kTexture2D, 0x2802, 0x812F);
  ::glTexParameteri(glutil::kTexture2D, 0x2803, 0x812F);
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
 core::bindTexture(static_cast<int>(lightmapTexture_));
 ::glTexImage2D(glutil::kTexture2D, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, pixels.data());
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
   if(found != activePack->definition.targets.end()) format = glutil::parseFormat(found->second.format);
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
  const ColorFormat format = glutil::parseFormat(target.format);
  if(!activePack->colorTargets.ensureNamed(name, tw, th, format)) {
   logOnce(*activePack,
           "size.buffer." + name + " allocate failed format=" + glutil::colorFormatName(format) +
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
   slots << name << '=' << glutil::colorFormatName(activePack->colorTargets.formatOf(name));
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

void Pipeline::destroyScene(PackInstance* activePack) {
 if(activePack != nullptr) activePack->colorTargets.destroy();
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
 scene.clearColors(clear, colors);
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
 if(activePack->depthTextures[index] == 0) {
  activePack->depthTextures[index] = core::genTexture();
  activePack->depthTextureW[index] = 0;
  activePack->depthTextureH[index] = 0;
 }
 if(activePack->depthTextures[index] == 0) return;
 core::bindTexture(static_cast<int>(activePack->depthTextures[index]));
 if(activePack->depthTextureW[index] != width || activePack->depthTextureH[index] != height) {
  ::glTexImage2D(glutil::kTexture2D, 0, 0x81A6, width, height, 0, 0x1902, 0x1405, nullptr);
  ::glTexParameteri(glutil::kTexture2D, 0x2801, 0x2600);
  ::glTexParameteri(glutil::kTexture2D, 0x2800, 0x2600);
  ::glTexParameteri(glutil::kTexture2D, 0x2802, 0x812F);
  ::glTexParameteri(glutil::kTexture2D, 0x2803, 0x812F);
  activePack->depthTextureW[index] = width;
  activePack->depthTextureH[index] = height;
 }
 ::glCopyTexSubImage2D(glutil::kTexture2D, 0, 0, 0, 0, 0, width, height);
}

gl::ShaderProgram* Pipeline::programFromPack(PackInstance& pack, const std::string& key) {
 return pack.summary.valid && pack.definition.programs.contains(key)
            ? PackCompiler::compile(pack, key,
                                    [this](PackInstance& p, const std::string& m, LogLevel level) {
                                     logOnce(p, m, level);
                                    })
            : nullptr;
}

gl::ShaderProgram* Pipeline::worldProgram(const std::string& key, PackInstance* activePack,
                                                PackInstance* basePack) {
 lastWorldProgramKey_ = key;
 PackInstance* pack = interfaceProgramsActive() ? basePack : activePack;
 if(pack == nullptr) pack = basePack;
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
 std::string programKey =
     shadowPass ? resolveIrisShadowProgramKey(key, pack->definition.programs) : key;
 if(programKey.empty()) return nullptr;

 if(!isProgramEnabledCached(pack->definition, pack->settings, programKey, pack->programEnabledCache)) {
  if(shadowPass && programKey != "shadow" &&
     isProgramEnabledCached(pack->definition, pack->settings, "shadow", pack->programEnabledCache) &&
     pack->definition.programs.contains("shadow")) {
   programKey = "shadow";
  } else {
   return nullptr;
  }
 }
 return programFromPack(*pack, programKey);
}

bool Pipeline::renderBegin(PackInstance* activePack) {
 if(activePack == nullptr) return false;
 activePack->publishedTextures.clear();
 shadowDepthTexture_ = -1;
 shadowOpaqueDepthTexture_ = -1;
 shadowColorTextureCount_ = 0;
 return runPasses(*activePack, activePack->beginPasses, false, "begin", -1, -1, nullptr, 0);
}

bool Pipeline::renderPreWorld(PackInstance* activePack, int shadowDepthTextureId,
                                     int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds,
                                     int shadowColorTextureCount) {
 if(activePack == nullptr) return false;
 const auto hasCompute = [activePack](const std::string& stage) {
  return std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(),
                     [activePack, &stage](std::size_t index) {
                      return compute::matchesStage(activePack->definition.passes[index].name, stage);
                     });
 };
 {
  const auto& def = activePack->definition;
  // https://shaders.properties/current/reference/constants/shadow_mipmaps/
  auto generate = [&def](int textureId, bool nearest) {
   if(textureId <= 0 || gl::GLCore::generateMipmap == nullptr) return;
   core::bindTexture(glutil::kTexture2D, textureId);
   const int minFilter = nearest ? gl::filter::NearestMipmapNearest : gl::filter::LinearMipmapLinear;
   const int magFilter = nearest ? gl::filter::Nearest : gl::filter::Linear;
   ::glTexParameteri(glutil::kTexture2D, gl::tex::MinFilter, minFilter);
   ::glTexParameteri(glutil::kTexture2D, gl::tex::MagFilter, magFilter);
   gl::GLCore::generateMipmap(glutil::kTexture2D);
  };
  if(def.shadowtexMipmap[0]) generate(shadowDepthTextureId, def.shadowtexNearest[0]);
  if(def.shadowtexMipmap[1]) generate(shadowOpaqueDepthTextureId, def.shadowtexNearest[1]);
  for(int i = 0; i < 8 && i < shadowColorTextureCount; ++i) {
   if(def.shadowcolorMipmap[i] && shadowColorTextureIds != nullptr) {
    generate(shadowColorTextureIds[i], def.shadowcolorNearest[i]);
   }
  }
 }
 bool rendered = false;
 if(!activePack->shadowCompositePasses.empty() || hasCompute("shadowcomp")) {
  rendered = runPasses(*activePack, activePack->shadowCompositePasses, false, "shadowcomp",
                       shadowDepthTextureId, shadowOpaqueDepthTextureId, shadowColorTextureIds,
                       shadowColorTextureCount) ||
             rendered;
 }
 if(!activePack->preparePasses.empty() || hasCompute("prepare")) {
  rendered = runPasses(*activePack, activePack->preparePasses, false, "prepare", shadowDepthTextureId,
                       shadowOpaqueDepthTextureId, shadowColorTextureIds, shadowColorTextureCount) ||
             rendered;
 }
 shadowDepthTexture_ = shadowDepthTextureId;
 shadowOpaqueDepthTexture_ = shadowOpaqueDepthTextureId;
 shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
 for(int index = 0; index < shadowColorTextureCount_; ++index) {
  shadowColorTextures_[index] = shadowColorTextureIds == nullptr ? -1 : shadowColorTextureIds[index];
 }
 worldUniforms_.shadowAvailable = shadowDepthTextureId >= 0 ? 1 : 0;
 worldUniforms_.normalAvailable = activePack->colorTargets.colorCount() > 1 ? 1 : 0;
 core::advanceProgramUniforms();
 return rendered;
}

bool Pipeline::renderDeferred(PackInstance* activePack, int shadowDepthTextureId,
                                    int shadowOpaqueDepthTextureId, const int* shadowColorTextureIds,
                                    int shadowColorTextureCount) {
 if(activePack == nullptr) return false;
 const bool hasComputeDeferred =
     std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(), [activePack](std::size_t index) {
      return compute::matchesStage(activePack->definition.passes[index].name, "deferred");
     });
 if(activePack->deferredPasses.empty() && !hasComputeDeferred) return false;

 return runPasses(*activePack, activePack->deferredPasses, false, "deferred", shadowDepthTextureId,
                  shadowOpaqueDepthTextureId, shadowColorTextureIds, shadowColorTextureCount);
}

bool Pipeline::renderPostProcess(PackInstance* activePack, PackInstance* /*basePack*/,
                                       int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                       const int* shadowColorTextureIds, int shadowColorTextureCount) {
 if(activePack == nullptr) return false;
 engineColorCorrect_ = engineOwnsColorCorrection(&activePack->definition);
 const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 const int screenW = minecraft != nullptr ? std::max(1, minecraft->displayWidth) : 1;
 const int screenH = minecraft != nullptr ? std::max(1, minecraft->displayHeight) : 1;
 if(engineColorCorrect_) {
  screenDrawFramebuffer(std::max(1, activePack->colorTargets.width()),
                        std::max(1, activePack->colorTargets.height()));
 }
 const bool hasComputeComposite =
     std::any_of(activePack->computePasses.begin(), activePack->computePasses.end(), [activePack](std::size_t index) {
      const std::string& name = activePack->definition.passes[index].name;
      return compute::matchesStage(name, "composite") ||
             compute::matchesStage(name, "final");
     });
 if(!activePack->postPasses.empty() || hasComputeComposite || !activePack->setupPasses.empty()) {
  runPasses(*activePack, activePack->postPasses, true, "composite", shadowDepthTextureId,
            shadowOpaqueDepthTextureId, shadowColorTextureIds, shadowColorTextureCount);
 }
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
  if(presentReadFbo_ == 0) gl::GLCore::genFramebuffers(1, &presentReadFbo_);
  if(presentReadFbo_ == 0) return false;
  constexpr unsigned kReadFramebuffer = 0x8CA8;
  constexpr unsigned kDrawFramebuffer = 0x8CA9;
  constexpr unsigned kColorAttachment0 = 0x8CE0;
  constexpr unsigned kTexture2D = 0x0DE1;
  constexpr unsigned kColorBufferBit = 0x00004000;
  constexpr unsigned kNearest = 0x2600;
  gl::GLCore::bindFramebuffer(kReadFramebuffer, presentReadFbo_);
  gl::GLCore::framebufferTexture2D(kReadFramebuffer, kColorAttachment0, kTexture2D, color0, 0);
  gl::GLCore::bindFramebuffer(kDrawFramebuffer, colorSpace_.writeFramebuffer());
  core::viewport(0, 0, width, height);
  gl::GLCore::blitFramebuffer(0, 0, width, height, 0, 0, width, height, kColorBufferBit, kNearest);
  gl::GLCore::bindFramebuffer(kReadFramebuffer, 0);
  gl::GLCore::bindFramebuffer(0x8D40, 0);
  return true;
 }
 if(gl::GLCore::blitFramebuffer == nullptr || gl::GLCore::genFramebuffers == nullptr ||
    gl::GLCore::bindFramebuffer == nullptr || gl::GLCore::framebufferTexture2D == nullptr) {
  return false;
 }
 if(presentReadFbo_ == 0) {
  gl::GLCore::genFramebuffers(1, &presentReadFbo_);
  if(presentReadFbo_ == 0) return false;
 }
 constexpr unsigned kFramebuffer = 0x8D40;
 constexpr unsigned kReadFramebuffer = 0x8CA8;
 constexpr unsigned kDrawFramebuffer = 0x8CA9;
 constexpr unsigned kColorAttachment0 = 0x8CE0;
 constexpr unsigned kTexture2D = 0x0DE1;
 constexpr unsigned kColorBufferBit = 0x00004000;
 constexpr unsigned kNearest = 0x2600;
 gl::GLCore::bindFramebuffer(kReadFramebuffer, presentReadFbo_);
 gl::GLCore::framebufferTexture2D(kReadFramebuffer, kColorAttachment0, kTexture2D, color0, 0);
 gl::GLCore::bindFramebuffer(kDrawFramebuffer, 0);
 core::viewport(0, 0, screenWidth, screenHeight);
 gl::GLCore::blitFramebuffer(0, 0, width, height, 0, 0, screenWidth, screenHeight, kColorBufferBit, kNearest);
 gl::GLCore::bindFramebuffer(kReadFramebuffer, 0);
 gl::GLCore::bindFramebuffer(kFramebuffer, 0);
 return true;
}

void Pipeline::presentFinalToScreen(PackInstance* scenePack, int screenWidth, int screenHeight) {
 if(packWroteToScreen_ || scenePack == nullptr || !scenePack->colorTargets.valid()) return;
 if(scenePack->colorTargets.readTexture(0) == 0 || !glutil::hasGlContext()) return;

 if(!scenePack->definition.programs.contains("final")) {
  logOnce(*scenePack, "no final program; presenting via colortex0 blit", LogLevel::Info);
  if(!blitColortex0ToScreen(*scenePack, screenWidth, screenHeight)) {
   shaderFatal("Shader pack present failed",
               "no final program and colortex0 present failed for pack '" +
                   (!scenePack->summary.name.empty() ? scenePack->summary.name : scenePack->definition.name) +
                   "'");
  }
  packWroteToScreen_ = true;
  return;
 }

 gl::ShaderProgram* program = programFromPack(*scenePack, "final");
 if(program == nullptr || !program->valid()) {
  shaderFatal("Shader pack present failed",
              "final program unusable for pack '" +
                  (!scenePack->summary.name.empty() ? scenePack->summary.name : scenePack->definition.name) +
                  "' (no cross-pack fallback)");
 }

 render::ColorTargets& targets = scenePack->colorTargets;
 const int width = targets.width();
 const int height = targets.height();
 if(width <= 0 || height <= 0) return;
 if(!PackResources::ensure(*scenePack, width, height, lightmapTexture_,
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
 targets.fillReadSamplers(textures);
 textures["depthtex0"] = static_cast<int>(targets.depthTexture());
 const unsigned int drawFbo = screenDrawFramebuffer(width, height);
 gl::GLCore::bindFramebuffer(0x8D40, drawFbo);
 core::viewport(0, 0, engineColorCorrect_ ? width : screenWidth,
                engineColorCorrect_ ? height : screenHeight);
 glutil::applyBufferBlends(scenePack->definition, "final");
 glutil::applyAlphaTest(scenePack->definition, "final");
 program->bind();

 std::unordered_map<std::string, int> volumeTextures;
 glutil::putShadowTextures(textures, shadowDepthTexture_, shadowOpaqueDepthTexture_, shadowColorTextures_,
                           shadowColorTextureCount_, &scenePack->definition);
 PackResources::addTextures(*scenePack, "composite", textures, volumeTextures);
 glutil::bindSamplers(*program, textures, volumeTextures, glutil::maxTextureUnits(),
                      &scenePack->definition);
 PackResources::bind(*scenePack, *program, 0);

 FrameUniformSet frameUniforms = worldUniforms_;
 frameUniforms.viewWidth = static_cast<float>(width);
 frameUniforms.viewHeight = static_cast<float>(height);
 frameUniforms.aspectRatio = frameUniforms.viewWidth / std::max(frameUniforms.viewHeight, 1.0f);
 frameUniforms.normalAvailable = targets.colorCount() > 1 ? 1 : 0;
 uploadUniforms(*program, frameUniforms);
 uploadIdentityDrawMatrices(*program);
 scenePack->customUniforms.upload(*program);
 program->bind();
 core::drawFullscreen();
 core::unlockBlend();
 packWroteToScreen_ = true;
 glutil::releaseSamplers(glutil::maxTextureUnits());
 core::activeTexture(gl::tex::Texture0);
}

bool Pipeline::runPasses(PackInstance& pack, const std::vector<std::size_t>& passes, bool present,
                               const std::string& stage, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                               const int* shadowColorTextureIds, int shadowColorTextureCount) {
 render::ColorTargets& targets = pack.colorTargets;
 const int shadowMapResolution = static_cast<int>(worldUniforms_.shadowMapResolution);
 const float farPlane = worldUniforms_.farPlane;
 const int width = stage == "shadowcomp" && shadowMapResolution > 0 ? shadowMapResolution : targets.width();
 const int height = stage == "shadowcomp" && shadowMapResolution > 0 ? shadowMapResolution : targets.height();
 if(!pack.summary.valid || pack.programs == nullptr ||
    (passes.empty() && pack.computePasses.empty() && pack.setupPasses.empty())) {
  return false;
 }
 if(!targets.valid() || targets.depthTexture() == 0 || !glutil::hasGlContext() || width <= 0 || height <= 0) {
  return false;
 }
 if(!PackResources::ensure(pack, width, height, lightmapTexture_,
                                 [](const PackInstance& p, const std::string& path) {
                                  return PackCompiler::readText(p, path);
                                 })) {
  logOnce(pack, "pack GPU resources could not be allocated", LogLevel::Severe);
  return false;
 }

 int destinationFramebuffer = 0;
 ::glGetIntegerv(0x8CA6, &destinationFramebuffer);
 std::vector<std::pair<std::size_t, gl::ShaderProgram*>> programChain;
 programChain.reserve(passes.size());
 for(std::size_t passIndex : passes) {
  if(passIndex >= pack.definition.passes.size()) continue;
  const PackPass& pass = pack.definition.passes[passIndex];
  if(pack.definition.programs.count(pass.program) == 0) {
   logOnce(pack, "pass '" + pass.name + "' references unknown program '" + pass.program + "'",
           LogLevel::Severe);
   return false;
  }
  gl::ShaderProgram* program = PackCompiler::compile(
      pack, pass.program, [this](PackInstance& p, const std::string& m, LogLevel level) {
       logOnce(p, m, level);
      });
  if(program == nullptr) {
   logOnce(pack, "pass '" + pass.name + "' program '" + pass.program + "' is unusable", LogLevel::Severe);
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
 refreshColorMaps(targets, textures, colorImages);
 textures["depthtex0"] = static_cast<int>(targets.depthTexture());
 // https://shaders.properties/current/reference/buffers/depthtex/
 // depthTextures[0] = pre-translucent (after hand); depthTextures[1] = pre-hand.
 if(pack.depthTextures[0] != 0) textures["depthtex1"] = static_cast<int>(pack.depthTextures[0]);
 if(pack.depthTextures[1] != 0) {
  textures["depthtex2"] = static_cast<int>(pack.depthTextures[1]);
 } else if(pack.depthTextures[0] != 0) {
  textures["depthtex2"] = static_cast<int>(pack.depthTextures[0]);
 }
 targets.applyPreFlips(pack.definition, stage);
 refreshColorMaps(targets, textures, colorImages);
 for(const auto& [name, texture] : pack.publishedTextures) {
  textures[name] = texture;
 }
 const int opaqueShadow =
     shadowOpaqueDepthTextureId >= 0 ? shadowOpaqueDepthTextureId : shadowDepthTextureId;
 glutil::putShadowTextures(textures, shadowDepthTextureId, opaqueShadow, shadowColorTextureIds,
                           shadowColorTextureCount, &pack.definition);
 for(int i = 0; i < std::min(shadowColorTextureCount, 8); ++i) {
  if(shadowColorTextureIds != nullptr && shadowColorTextureIds[i] >= 0) {
   colorImages["shadowcolor" + std::to_string(i)] = shadowColorTextureIds[i];
  }
 }
 std::unordered_map<std::string, int> volumeTextures;
 PackResources::addTextures(pack, stage, textures, volumeTextures);

 FrameUniformSet frameUniforms = worldUniforms_;
 frameUniforms.viewWidth = static_cast<float>(width);
 frameUniforms.viewHeight = static_cast<float>(height);
 frameUniforms.aspectRatio = frameUniforms.viewWidth / std::max(frameUniforms.viewHeight, 1.0f);
 frameUniforms.farPlane = farPlane;
 frameUniforms.shadowMapResolution = static_cast<float>(shadowMapResolution);
 frameUniforms.shadowAvailable = shadowDepthTextureId >= 0 ? 1 : 0;
 frameUniforms.normalAvailable = targets.colorCount() > 1 ? 1 : 0;

 const bool computeReady = gl::GLCore::computeSupported;
 auto compileFn = [this](PackInstance& p, const std::string& name) {
  return PackCompiler::compile(p, name, [this](PackInstance& p2, const std::string& m, LogLevel level) {
   logOnce(p2, m, level);
  });
 };
 if(computeReady &&
    !dispatchSetupIfNeeded(pack, frameUniforms, width, height, textures, colorImages, volumeTextures, compileFn)) {
  glutil::releaseSamplers(glutil::maxTextureUnits());
  return false;
 }

 // https://github.com/IrisShaders/ShaderDoc/blob/master/passes/compute.md
 const bool concurrent = pack.definition.allowConcurrentCompute;
 std::vector<std::size_t> stageComputes;
 if(computeReady) {
  for(std::size_t passIndex : pack.computePasses) {
   const PackPass& compute = pack.definition.passes[passIndex];
   if(!compute::matchesStage(compute.name, stage) &&
      !(stage == "composite" && compute::matchesStage(compute.name, "final"))) {
    continue;
   }
   stageComputes.push_back(passIndex);
  }
  std::stable_sort(stageComputes.begin(), stageComputes.end(), [&pack](std::size_t a, std::size_t b) {
   const std::string& na = pack.definition.passes[a].name;
   const std::string& nb = pack.definition.passes[b].name;
   const std::string pa = compute::computeParentName(na);
   const std::string pb = compute::computeParentName(nb);
   if(pa != pb) return compute::lessComputeParent(pa, pb);
   return compute::lessComputeOrder(pack.definition.passes[a], pack.definition.passes[b]);
  });
 }

 std::vector<bool> computeDispatched(pack.definition.passes.size(), false);
 bool ranCompute = false;

 const auto prepareComputeBinds = [&](bool writeSideImages) {
  if(gl::GLCore::bindFramebuffer != nullptr) gl::GLCore::bindFramebuffer(0x8D40, 0);
  if(gl::GLCore::memoryBarrier != nullptr) {
   gl::GLCore::memoryBarrier(compute::kBarrierBits);
  }
  refreshColorMaps(targets, textures, colorImages, writeSideImages);
  glutil::refreshTextureAliases(textures, pack.definition.usesWaterShadow);
 };

 const auto dispatchParentComputes = [&](const std::string& parent) -> bool {
  std::vector<std::string> writeBuffers;
  writeBuffers.reserve(8);
  for(std::size_t passIndex : stageComputes) {
   if(compute::computeParentName(pack.definition.passes[passIndex].name) != parent) continue;
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
    wb << writeBuffers[i] << '=' << glutil::colorFormatName(targets.formatOf(writeBuffers[i]));
   }
  logOnce(pack, "compute parent '" + parent + "' writeBuffers=[" + wb.str() + "] concurrent=" +
                    (concurrent ? "true" : "false"));
 }
 for(const std::string& name : writeBuffers) {
  targets.prepareWrite(name);
 }
 prepareComputeBinds(!writeBuffers.empty());
 bool any = false;
 for(std::size_t passIndex : stageComputes) {
  if(computeDispatched[passIndex]) continue;
  if(compute::computeParentName(pack.definition.passes[passIndex].name) != parent) continue;
  if(!compute::dispatch(pack, pack.definition.passes[passIndex], frameUniforms, textures,
                                  colorImages, volumeTextures, width, height, !concurrent, compileFn)) {
   return false;
  }
   computeDispatched[passIndex] = true;
   any = true;
   ranCompute = true;
  }
  if(any && concurrent && gl::GLCore::memoryBarrier != nullptr) {
   gl::GLCore::memoryBarrier(compute::kBarrierBits);
  }
  if(any) {
   for(const std::string& name : writeBuffers) {
    targets.flipIfEnabled(pack.definition, parent, name);
   }
   refreshColorMaps(targets, textures, colorImages, false);
   logOnce(pack, "compute parent '" + parent + "' flipped writeBuffers and refreshed samplers");
  }
  return true;
 };

 const auto dispatchOrphansBefore = [&](const std::string* nextRasterPass) -> bool {
  for(std::size_t passIndex : stageComputes) {
   if(computeDispatched[passIndex]) continue;
   const std::string parent = compute::computeParentName(pack.definition.passes[passIndex].name);
   const bool hasRaster = std::any_of(programChain.begin(), programChain.end(), [&](const auto& entry) {
    return pack.definition.passes[entry.first].name == parent;
   });
   if(hasRaster) continue;
   if(nextRasterPass != nullptr && !compute::lessComputeParent(parent, *nextRasterPass)) break;
   if(!dispatchParentComputes(parent)) return false;
  }
  return true;
 };

 if(computeReady && programChain.empty()) {
  if(!dispatchOrphansBefore(nullptr)) {
   glutil::releaseSamplers(glutil::maxTextureUnits());
   return false;
  }
 }
 if(programChain.empty()) {
  if(present) {
   packWroteToScreen_ = false;
  } else if(gl::GLCore::bindFramebuffer != nullptr) {
   gl::GLCore::bindFramebuffer(0x8D40, static_cast<unsigned int>(destinationFramebuffer));
   core::viewport(0, 0, width, height);
  }
  glutil::releaseSamplers(glutil::maxTextureUnits());
  core::activeTexture(gl::tex::Texture0);
  shadowDepthTexture_ = shadowDepthTextureId;
  shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
  for(int index = 0; index < shadowColorTextureCount_; ++index) {
   shadowColorTextures_[index] = shadowColorTextureIds == nullptr ? -1 : shadowColorTextureIds[index];
  }
  return present ? false : ranCompute;
 }

 bool wroteToScreen = false;
 bool executed = false;
 for(const auto& [passIndex, program] : programChain) {
  const PackPass& pass = pack.definition.passes[passIndex];
  if(computeReady && !dispatchOrphansBefore(&pass.name)) {
   glutil::releaseSamplers(glutil::maxTextureUnits());
   return false;
  }
  if(computeReady) {
   if(!dispatchParentComputes(pass.name)) {
    glutil::releaseSamplers(glutil::maxTextureUnits());
    return false;
   }
  }
  const std::string output = pass.outputs.empty() ? "screen" : pass.outputs.front();
  const bool toScreen =
      pack_catalog::lower(output) == "screen" || pass.name == "final" ||
      pass.program.rfind("final", 0) == 0;
  for(const std::string& buffer : pass.mipmapBuffers) {
   const auto tex = textures.find(buffer);
   if(tex == textures.end() || tex->second <= 0 || gl::GLCore::generateMipmap == nullptr) continue;
   core::activeTexture(gl::tex::Texture0);
   core::bindTexture(tex->second);
   ::glTexParameteri(glutil::kTexture2D, 0x2801, 0x2703);
   ::glTexParameteri(glutil::kTexture2D, 0x2800, 0x2601);
   gl::GLCore::generateMipmap(glutil::kTexture2D);
  }
  if(!toScreen) {
   std::vector<std::string> outputs = pass.outputs.empty() ? std::vector<std::string>{output} : pass.outputs;
   for(const std::string& name : outputs) {
    const auto declared = pack.definition.targets.find(name);
    ColorFormat format = ColorFormat::Rgba8;
    int tw = width;
    int th = height;
    if(declared != pack.definition.targets.end()) {
     format = glutil::parseFormat(declared->second.format);
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
      logOnce(pack, "pass '" + pass.name + "' could not allocate target '" + name + "'", LogLevel::Severe);
      return false;
     }
    }
    targets.prepareWrite(name);
   }
   refreshColorMaps(targets, textures, colorImages);
   if(!targets.bindWrite(outputs)) {
    logOnce(pack, "pass '" + pass.name + "' could not bind write targets", LogLevel::Severe);
    return false;
   }
  } else {
   const unsigned int drawFbo =
       present ? screenDrawFramebuffer(width, height) : static_cast<unsigned int>(destinationFramebuffer);
   gl::GLCore::bindFramebuffer(0x8D40, drawFbo);
   core::viewport(0, 0, width, height);
  }
  program->bind();
  glutil::applyBufferBlends(pack.definition, pass.program, program->drawBufferColortexIndices());
  glutil::applyAlphaTest(pack.definition, pass.program);
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
  glutil::refreshTextureAliases(textures, pack.definition.usesWaterShadow);
  PackResources::addTextures(pack, stage, textures, volumeTextures);
  glutil::bindSamplers(*program, textures, volumeTextures, glutil::maxTextureUnits(),
                       &pack.definition);
  const unsigned int nextImageUnit =
      glutil::bindColorImages(*program, colorImages, &pack.definition, &targets);
  PackResources::bind(pack, *program, nextImageUnit);
  uploadUniforms(*program, frameUniforms);
  uploadIdentityDrawMatrices(*program);
  pack.customUniforms.upload(*program);
  program->bind();
  core::drawFullscreen();
  core::unlockBlend();
  executed = true;
  if(toScreen) {
   wroteToScreen = fullViewport;
  } else {
   std::vector<std::string> outputs = pass.outputs.empty() ? std::vector<std::string>{output} : pass.outputs;
   for(const std::string& name : outputs) {
    targets.flipIfEnabled(pack.definition, pass.name, name);
    if(stage == "shadowcomp" || stage == "prepare") {
     pack.publishedTextures[name] = static_cast<int>(targets.readTexture(name));
    }
   }
   refreshColorMaps(targets, textures, colorImages);
  }
  core::activeTexture(gl::tex::Texture0);
 }
 if(computeReady && !dispatchOrphansBefore(nullptr)) {
  glutil::releaseSamplers(glutil::maxTextureUnits());
  return false;
 }
 if(present) packWroteToScreen_ = wroteToScreen;
 if(!present) {
  gl::GLCore::bindFramebuffer(0x8D40, static_cast<unsigned int>(destinationFramebuffer));
  core::viewport(0, 0, width, height);
 }
 glutil::releaseSamplers(glutil::maxTextureUnits());
 core::activeTexture(gl::tex::Texture0);
 shadowDepthTexture_ = shadowDepthTextureId;
 shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
 for(int index = 0; index < shadowColorTextureCount_; ++index) {
  shadowColorTextures_[index] = shadowColorTextureIds == nullptr ? -1 : shadowColorTextureIds[index];
 }
 return present ? wroteToScreen : executed;
}

}
