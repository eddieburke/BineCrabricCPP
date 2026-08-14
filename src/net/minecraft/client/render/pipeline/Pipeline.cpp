#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
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
#include "net/minecraft/client/debug/VTuneTrace.hpp"
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
bool fogClassForKey(const std::string& key) {
 if(key.rfind("shadow", 0) == 0) return false;
 if(key == "gbuffers_gui" || key == "gbuffers_gui_textured") return false;
 return true;
}
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
} // namespace
Pipeline::Pipeline(option::GameOptions* options) : options_(options) {}
Pipeline::~Pipeline() {
 stopDirectoryWatcher();
 presentReadFbo_.destroy();
 colorSpace_.destroy();
}
void Pipeline::logOnce(PackInstance& pack, const std::string& message,
                       ::net::minecraft::util::logging::LogLevel level) const {
 if(!pack.logged.insert(message).second) return;
 const std::string& label =
     !pack.summary.name.empty()      ? pack.summary.name
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
bool Pipeline::engineOwnsColorCorrection(const PackDefinition& def) const {
 if(options_ == nullptr || options_->colorSpace == 0) return false;
 return !def.supportsColorCorrection;
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
 return !activePack->stagePlan(CompositeStage::Deferred).empty();
}
void Pipeline::ensurePbrFallbackTextures() {
 if(!normalFallbackTexture_) uploadRgbaStub(normalFallbackTexture_, 127, 127, 255, 255);
 if(!specularFallbackTexture_) uploadRgbaStub(specularFallbackTexture_, 0, 0, 0, 0);
}
void Pipeline::bindPbrTextures() {
 ensurePbrFallbackTextures();
 PbrTextures::onNewFrame();
}
void Pipeline::refreshResourcePackState(PackInstance* basePack,
                                        const std::vector<std::unique_ptr<PackInstance>>& packs) {
 const auto* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 const auto* selected =
     minecraft != nullptr && minecraft->texturePacks != nullptr ? minecraft->texturePacks->selected : nullptr;
 const std::string key = selected == nullptr ? std::string{} : selected->key.empty() ? selected->name
                                                                                     : selected->key;
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
void Pipeline::applyBlockIds(const PackDefinition& definition) {
 ++objectIdRevision_;
 std::array<int, 256> ids{};
 if(definition.hasBlockProperties)
  ids.fill(-1);
 else
  for(int i = 0; i < 256; ++i) ids[static_cast<std::size_t>(i)] = i;
 {
  for(int id = 0; id < net::minecraft::block::Block::BLOCK_COUNT; ++id) {
   net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
   if(block == nullptr) {
    continue;
   }
   std::string name = PackCatalog::lower(block->getTranslationKey());
   if(name.rfind("tile.", 0) == 0) {
    name.erase(0, 5);
   }
   const auto apply = [&](std::string_view candidate) {
    if(const auto found = definition.blockIds.find(std::string(candidate)); found != definition.blockIds.end()) {
     ids[static_cast<std::size_t>(id)] = found->second;
     return true;
    }
    if(const auto found = definition.blockIds.find("minecraft:" + std::string(candidate));
       found != definition.blockIds.end()) {
     ids[static_cast<std::size_t>(id)] = found->second;
     return true;
    }
    return false;
   };
   if(block == net::minecraft::block::Block::LIT_REDSTONE_TORCH) {
    if(apply("redstone_torch:lit=true") || apply("redstone_wall_torch:lit=true")) continue;
   } else if(block == net::minecraft::block::Block::REDSTONE_TORCH) {
    if(apply("redstone_torch:lit=false") || apply("redstone_wall_torch:lit=false")) continue;
   }
   apply(name);
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
 core::setActiveProgram(nullptr);
 if(gl::GLCore::useProgram != nullptr) gl::GLCore::useProgram(0);
 const std::string transition =
     "clearing programs (dimension '" + pack.dimensionKey + "' -> '" + selectedKey + "')";
 pack.clearGpuResources();
 pack.dimensionKey = selectedKey;
 pack.definition = selected != &pack.rootDefinition ? *selected : pack.rootDefinition;
 if(selected != &pack.rootDefinition) {
  // Collections are rebuilt per folder, so re-base them on the root and overlay
  // the dimension's own entries.
  pack.definition.programs = pack.rootDefinition.programs;
  pack.definition.passes = pack.rootDefinition.passes;
  pack.definition.targets = pack.rootDefinition.targets;
  pack.definition.bufferBlends = pack.rootDefinition.bufferBlends;
  pack.definition.images = pack.rootDefinition.images;
  pack.definition.customTextures = pack.rootDefinition.customTextures;
  pack.definition.bufferObjects = pack.rootDefinition.bufferObjects;
  pack.definition.dimensionFolders = pack.rootDefinition.dimensionFolders;
  pack.definition.dimensionDefinitions = pack.rootDefinition.dimensionDefinitions;
  for(const auto& [name, program] : selected->programs) pack.definition.programs[name] = program;
  // A dimension folder's own scan can under-report a target's format (e.g. via a
  // default RENDERTARGETS output) even when the root scan already saw the real
  // const colortexNFormat declaration; never let that downgrade root's value.
  const auto realFormat = [](const std::string& format) {
   return !format.empty() && format != "RGBA" && format != "RGBA8";
  };
  for(const auto& [name, target] : selected->targets) {
   const auto existingTarget = pack.definition.targets.find(name);
   if(existingTarget == pack.definition.targets.end()) {
    pack.definition.targets[name] = target;
    continue;
   }
   const bool keepRootFormat = realFormat(existingTarget->second.format) && !realFormat(target.format);
   const std::string rootFormat = existingTarget->second.format;
   existingTarget->second = target;
   if(keepRootFormat) existingTarget->second.format = rootFormat;
  }
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
   if(match == pack.definition.passes.end())
    pack.definition.passes.push_back(pass);
   else
    *match = pass;
  }
 }
 std::string customError;
 if(!pack.rebuildRuntime(customError)) logOnce(pack, customError, LogLevel::Warning);
 logOnce(pack, transition, LogLevel::Info);
 if(applyIds) {
  applyBlockIds(pack.definition);
 }
 return true;
}
bool Pipeline::preparePackResources(PackInstance& pack, int width, int height) {
 if(!ensureSceneTargets(&pack, width, height)) return false;
 return ensurePackResources(pack, width, height, lightmapTexture_,
                            [](const PackInstance& candidate, const std::string& path) {
                             return PackCompiler::readText(candidate, path);
                            });
}
void Pipeline::prepareFrame(net::minecraft::World* world, PackInstance* activePack,
                            PackInstance* basePack) {
 bindPbrTextures();
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
  activePack->colorTargets.resetFlips();
  if(basePack != nullptr && basePack != activePack) {
   basePack->colorTargets.resetFlips();
  }
  for(const CustomImage& declaration : activePack->definition.images) {
   const auto found = activePack->images.find(declaration.name);
   if(!declaration.clearEachFrame || found == activePack->images.end() || !found->second.texture) {
    continue;
   }
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
  addPackTextures(*activePack, "setup", textures, volumes);
  if(activePack->colorTargets.valid()) {
   activePack->colorTargets.fillImageBindings(colorImages);
  }
  dispatchSetupIfNeeded(*activePack, worldUniforms_, nullptr, width, height, textures, colorImages,
                        volumes, &activePack->colorTargets);
 }
}
void Pipeline::refreshLightmap(net::minecraft::World* world) {
 updateLightmap(world);
}
void Pipeline::setFrameUniforms(const PackUniformValues& frame, const PackDefinition& activeDef,
                                PackInstance* activePack) {
 worldUniforms_ = frame;
 worldUniforms_.wetness = updateWetnessSmooth(worldUniforms_.rainStrength, worldUniforms_.frameTime,
                                              activeDef.wetnessHalflife);
 if(activePack != nullptr) {
  activePack->customUniforms.evaluate(worldUniforms_);
 }
 core::advanceProgramUniforms();
}
void Pipeline::updateLightmap(const net::minecraft::World* world) {
 if(!hasGlContext()) return;
 const bool lit = world != nullptr && world->dimension != nullptr;
 const int ambient = world != nullptr ? world->ambientDarkness : 0;
 if(lightmapTexture_ && lightmapLit_ == lit && lightmapAmbient_ == ambient) {
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
  ::glTexParameteri(gl::cap::Texture2D, 0x2801, 0x2601);
  ::glTexParameteri(gl::cap::Texture2D, 0x2800, 0x2601);
  ::glTexParameteri(gl::cap::Texture2D, 0x2802, 0x812F);
  ::glTexParameteri(gl::cap::Texture2D, 0x2803, 0x812F);
 }
 std::array<std::uint8_t, 16 * 16 * 4> pixels{};
 for(int sky = 0; sky < 16; ++sky) {
  for(int block = 0; block < 16; ++block) {
   float value = 1.0f;
   if(lit) {
    const int effectiveSky = std::max(0, sky - ambient);
    const int level = std::clamp(std::max(block, effectiveSky), 0, 15);
    value = world->dimension->lightLevelToLuminance[static_cast<std::size_t>(level)];
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
 ::glTexImage2D(gl::cap::Texture2D, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, pixels.data());
 core::activeTexture(gl::tex::Texture0 + previousUnit);
 lightmapLit_ = lit;
 lightmapAmbient_ = ambient;
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
 const int gbufferCount = std::clamp(activePack->definition.gbufferColorBuffers, 1, kMaxColorAttachments);
 if(!activePack->colorTargets.ensure(width, height, formats, gbufferCount)) {
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
  if(i == 0)
   colors[static_cast<std::size_t>(i)] = {fogR, fogG, fogB, 1.0f};
  else if(i == 1)
   colors[static_cast<std::size_t>(i)] = {1.0f, 1.0f, 1.0f, 1.0f};
  else
   colors[static_cast<std::size_t>(i)] = {0.0f, 0.0f, 0.0f, 0.0f};
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
void Pipeline::sampleCenterDepth(PackInstance* activePack, const PackDefinition& activeDef) {
 if(!activeDef.usesCenterDepthSmooth) return;
 if(activePack == nullptr || !activePack->colorTargets.valid()) return;
 const auto& targets = activePack->colorTargets;
 if(targets.width() <= 0 || targets.height() <= 0 || targets.depthTexture() == 0) return;
 bindScene(activePack);
 const std::optional<float> depth = centerDepthSampler_.pollAndIssue(targets.width() / 2, targets.height() / 2);
 if(!depth.has_value()) return;
 const float nearPlane = worldUniforms_.nearPlane > 0.0f ? worldUniforms_.nearPlane : 0.05f;
 const float projectionFar = core::cameraFrame().farPlane;
 const float farPlane = projectionFar > nearPlane ? projectionFar : worldUniforms_.farPlane;
 const float halfLife = activeDef.centerDepthHalflife;
 worldUniforms_.centerDepthSmooth =
     updateCenterDepthSmooth(*depth, nearPlane, farPlane, worldUniforms_.frameTime, halfLife);
}
void Pipeline::captureOpaqueDepth(PackInstance* activePack) {
 captureDepth(activePack, 1);
}
void Pipeline::captureHandDepth(PackInstance* activePack) {
 captureDepth(activePack, 0);
}
void Pipeline::captureDepth(PackInstance* activePack, std::size_t index) {
 VT_TRACE_EVENT(index == 1 ? "depth/capture_opaque" : "depth/capture_hand");
 if(activePack == nullptr || !activePack->colorTargets.valid() || index >= 2) return;
 const int width = activePack->colorTargets.width();
 const int height = activePack->colorTargets.height();
 if(width <= 0 || height <= 0 || activePack->colorTargets.depthTexture() == 0) return;
 if(index == 1) {
  VT_TRACE_COUNTER("OpaqueDepthSnapshotPixels", width * height);
 } else {
  VT_TRACE_COUNTER("HandDepthSnapshotPixels", width * height);
 }
 bindScene(activePack);
 if(!activePack->depthTextures[index]) {
  activePack->depthTextures[index] = gl::GlTexture(core::genTexture());
  activePack->depthTextureW[index] = 0;
  activePack->depthTextureH[index] = 0;
 }
 if(!activePack->depthTextures[index]) return;
 core::bindTexture(static_cast<int>(activePack->depthTextures[index].handle()));
 if(activePack->depthTextureW[index] != width || activePack->depthTextureH[index] != height) {
  ::glTexImage2D(gl::cap::Texture2D, 0, 0x81A6, width, height, 0, 0x1902, 0x1405, nullptr);
  ::glTexParameteri(gl::cap::Texture2D, 0x2801, 0x2600);
  ::glTexParameteri(gl::cap::Texture2D, 0x2800, 0x2600);
  ::glTexParameteri(gl::cap::Texture2D, 0x2802, 0x812F);
  ::glTexParameteri(gl::cap::Texture2D, 0x2803, 0x812F);
  activePack->depthTextureW[index] = width;
  activePack->depthTextureH[index] = height;
 }
 ::glCopyTexSubImage2D(gl::cap::Texture2D, 0, 0, 0, 0, 0, width, height);
}
gl::ShaderProgram* Pipeline::worldProgram(WorldProgramId id, PackInstance* pack) {
 if(pack == nullptr) return nullptr;
 core::RenderStage renderStage = core::RenderStage::None;
 if(id == WorldProgramId::TerrainSolid) {
  renderStage = core::RenderStage::TerrainSolid;
 } else if(id == WorldProgramId::TerrainCutout || id == WorldProgramId::DamagedBlock) {
  renderStage = core::RenderStage::TerrainCutout;
 } else if(id == WorldProgramId::Entities || id == WorldProgramId::EntitiesTranslucent ||
           id == WorldProgramId::Item || id == WorldProgramId::Lightning) {
  renderStage = core::RenderStage::Entities;
 } else if(id == WorldProgramId::Block || id == WorldProgramId::BlockTranslucent) {
  renderStage = core::RenderStage::BlockEntities;
 } else if(id == WorldProgramId::Hand) {
  renderStage = core::RenderStage::HandSolid;
 } else if(id == WorldProgramId::TerrainTranslucent) {
  renderStage = core::RenderStage::TerrainTranslucent;
 } else if(id == WorldProgramId::Particles || id == WorldProgramId::ParticlesTranslucent) {
  renderStage = core::RenderStage::Particles;
 } else if(id == WorldProgramId::Clouds) {
  renderStage = core::RenderStage::Clouds;
 } else if(id == WorldProgramId::Weather) {
  renderStage = core::RenderStage::RainSnow;
 } else if(id == WorldProgramId::SkyBasic || id == WorldProgramId::SkyTextured) {
  renderStage = core::RenderStage::Sky;
 } else if(id == WorldProgramId::Line) {
  renderStage = core::renderStage();
 }
 core::setRenderStage(renderStage);
 const bool shadowPass = core::cameraFrame().shadowPass;
 const PackInstance::WorldProgramRuntime& runtime =
     pack->worldPrograms[static_cast<std::size_t>(id)][shadowPass ? 1u : 0u];
 gl::ShaderProgram* program = runtime.program;
 if(program != nullptr) program->setFogClass(fogClassForKey(runtime.resolvedKey));
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
 if(shadowTargets_ != nullptr && shadowColorFlipPack_ != activePack) {
  shadowColorFlipPack_ = activePack;
  initShadowColorFlips(*activePack);
 }
 return renderCompositePasses(*activePack, CompositeStage::Begin, false,
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
 {
  const auto& def = activePack->definition;
  auto generate = [&def](int textureId, bool nearest) {
   if(textureId <= 0 || gl::GLCore::generateMipmap == nullptr) return;
   core::bindTexture(gl::cap::Texture2D, textureId);
   const int minFilter = nearest ? gl::filter::NearestMipmapNearest : gl::filter::LinearMipmapLinear;
   const int magFilter = nearest ? gl::filter::Nearest : gl::filter::Linear;
   ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter, minFilter);
   ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, magFilter);
   gl::GLCore::generateMipmap(gl::cap::Texture2D);
  };
  for(int i = 0; i < 8 && i < shadowColorTextureCount; ++i) {
   if(def.shadowcolorMipmap[i] && shadowColorTextureIds != nullptr) {
    generate(shadowColorTextureIds[i], def.shadowcolorNearest[i]);
   }
  }
 }
 if(worldUniforms_.shadowMapResolution <= 0.0f && activePack->definition.shadowMapResolution > 0) {
  worldUniforms_.shadowMapResolution = static_cast<float>(activePack->definition.shadowMapResolution);
 }
 bool rendered = false;
 if(shadowTargets_ != nullptr && shadowDepthTextureId >= 0 &&
    !activePack->stagePlan(CompositeStage::ShadowComposite).empty()) {
  rendered = renderCompositePasses(*activePack, CompositeStage::ShadowComposite, false,
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
 if(shadowTargets_ != nullptr && shadowColorFlipPack_ != activePack) {
  shadowColorFlipPack_ = activePack;
  initShadowColorFlips(*activePack);
 }
 bool rendered = false;
 if(!activePack->stagePlan(CompositeStage::Prepare).empty()) {
  rendered = renderCompositePasses(*activePack, CompositeStage::Prepare, false,
                                   shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                   shadowColorTextureIds, shadowColorTextureCount,
                                   shadowTargets_, shadowColorAltTextures_) ||
             rendered;
  activePack->colorTargets.bindGbuffers();
 }
 shadowDepthTexture_ = shadowDepthTextureId;
 shadowOpaqueDepthTexture_ = shadowOpaqueDepthTextureId;
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
 if(activePack->stagePlan(CompositeStage::Deferred).empty()) return false;
 return renderCompositePasses(*activePack, CompositeStage::Deferred, false,
                              shadowDepthTextureId, shadowOpaqueDepthTextureId,
                              shadowColorTextureIds, shadowColorTextureCount,
                              shadowTargets_, shadowColorAltTextureIds);
}
bool Pipeline::renderPostProcess(PackInstance* activePack, PackInstance*,
                                 int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                 const int* shadowColorTextureIds, int shadowColorTextureCount,
                                 const int* shadowColorAltTextureIds) {
 if(activePack == nullptr) return false;
 engineColorCorrect_ = engineOwnsColorCorrection(activePack->definition);
 const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 const int screenW = minecraft != nullptr ? std::max(1, minecraft->displayWidth) : 1;
 const int screenH = minecraft != nullptr ? std::max(1, minecraft->displayHeight) : 1;
 if(engineColorCorrect_) {
  (void)screenDrawFramebuffer(std::max(1, activePack->colorTargets.width()),
                              std::max(1, activePack->colorTargets.height()));
 }
 if(!activePack->stagePlan(CompositeStage::Composite).empty() || !activePack->setupPlan.empty()) {
  renderCompositePasses(*activePack, CompositeStage::Composite, true,
                        shadowDepthTextureId, shadowOpaqueDepthTextureId,
                        shadowColorTextureIds, shadowColorTextureCount,
                        shadowTargets_, shadowColorAltTextureIds);
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
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::DrawFramebuffer), 0);
 core::viewport(0, 0, screenWidth, screenHeight);
 gl::GLCore::blitFramebuffer(0, 0, width, height, 0, 0, screenWidth, screenHeight, gl::attrib::ColorBufferBit,
                             gl::filter::Nearest);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::ReadFramebuffer), 0);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
 return true;
}
void Pipeline::presentFinalToScreen(PackInstance* scenePack, int screenWidth, int screenHeight) {
 if(packWroteToScreen_ || scenePack == nullptr || !scenePack->colorTargets.valid()) return;
 if(scenePack->colorTargets.readTexture(0) == 0 || !hasGlContext()) return;
 if(!scenePack->definition.programs.contains("final") ||
    !isProgramEnabledCached(scenePack->definition, scenePack->settings, "final",
                            scenePack->programEnabledCache)) {
  logOnce(*scenePack, "no final program (missing or disabled); presenting via colortex0 blit", LogLevel::Info);
  (void)blitColortex0ToScreen(*scenePack, screenWidth, screenHeight);
  packWroteToScreen_ = true;
  return;
 }
 gl::ShaderProgram* program = nullptr;
 const auto compiled = scenePack->compiledPrograms.find("final");
 if(compiled != scenePack->compiledPrograms.end() && !compiled->second.failed) {
  program = compiled->second.program;
 }
 if(program == nullptr || !program->valid()) {
  (void)blitColortex0ToScreen(*scenePack, screenWidth, screenHeight);
  packWroteToScreen_ = true;
  return;
 }
 render::ColorTargets& targets = scenePack->colorTargets;
 const int width = targets.width();
 const int height = targets.height();
 if(width <= 0 || height <= 0) return;
 if(!ensurePackResources(*scenePack, width, height, lightmapTexture_,
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
 const unsigned int drawFbo = screenDrawFramebuffer(width, height);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), drawFbo);
 core::viewport(0, 0, engineColorCorrect_ ? width : screenWidth,
                engineColorCorrect_ ? height : screenHeight);
 applyBufferBlends(scenePack->definition, "final", {});
 applyAlphaTest(scenePack->definition, "final");
 program->bind();
 std::unordered_map<std::string, int> volumeTextures;
 putShadowTextures(textures, shadowDepthTexture_, shadowOpaqueDepthTexture_,
                   shadowColorTextures_,
                   shadowColorTextureCount_, scenePack->definition);
 addPackTextures(*scenePack, "composite", textures, volumeTextures);
 bindSamplers(*program, textures, volumeTextures, maxTextureUnits(),
              scenePack->definition);
 bindPackResources(*scenePack, *program, 0);
 const PackUniformValues& frameUniforms = worldUniforms_;
 const PackViewportValues viewport{static_cast<float>(width),
                                   static_cast<float>(height),
                                   static_cast<float>(width) / std::max(static_cast<float>(height), 1.0f)};
 uploadShaderUniforms(*program, frameUniforms, true, &viewport);
 uploadIdentityDrawMatrices(*program);
 scenePack->customUniforms.upload(*program);
 program->bind();
 core::drawFullscreen();
 core::unlockBlend();
 packWroteToScreen_ = true;
 releaseSamplers(maxTextureUnits());
 core::activeTexture(gl::tex::Texture0);
}
} // namespace net::minecraft::client::render
