#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"
#include "net/minecraft/client/render/shaderpack/ComputeDispatcher.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderFrameData.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCatalog.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCompiler.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackResources.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPassScheduler.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderTexture.hpp"
#include "net/minecraft/client/render/RenderTargets.hpp"
#include "net/minecraft/client/render/shaderpack/WorldProgramBinder.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/resource/pack/TexturePack.hpp"
#include "net/minecraft/client/resource/pack/TexturePacks.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace net::minecraft::client::render::shaderpack {
namespace {
using ShaderPackCatalog::directoryResources;
using ShaderPackCatalog::lower;
using ShaderPackCatalog::packDirectoryStamp;
using ShaderPackCatalog::zipResources;
std::string defaultSettingValue(const PackSetting& setting) {
 return setting.defaultValue;
}
} // namespace
ShaderPackManager::ShaderPackManager(std::filesystem::path gameDirectory, option::GameOptions* options)
    : gameDirectory_(std::move(gameDirectory)), options_(options) {
 reload();
 startDirectoryWatcher();
 render::setWorldProgramResolver([this](const std::string& key) { return worldProgram(key); });
 render::setWorldPassDirectiveApplier([this](const std::string& key) {
  ShaderPackInstance* pack = interfaceProgramsActive() ? basePack_.get() : activePack();
  if(pack == nullptr) {
   pack = basePack_.get();
  }
  if(pack == nullptr) {
   return;
  }
  std::string programKey = key;
  if(RenderCameraState::instance().frame().shadowPass) {
   if(key == "gbuffers_terrain_solid") {
    programKey = "shadow_solid";
   } else if(key == "gbuffers_terrain_cutout" || key == "gbuffers_damagedblock") {
    programKey = "shadow_cutout";
   } else if(key == "gbuffers_water") {
    programKey = "shadow_water";
   } else if(key.rfind("gbuffers_entities", 0) == 0 || key == "gbuffers_item" || key == "gbuffers_beaconbeam" ||
             key == "gbuffers_lightning") {
    programKey = "shadow_entities";
   } else if(key.rfind("gbuffers_block", 0) == 0) {
    programKey = "shadow_block";
   } else {
    programKey = "shadow";
   }
   if(!pack->definition.programs.contains(programKey)) {
    programKey = "shadow";
   }
  }
  glutil::applyBufferBlends(pack->definition, programKey);
  glutil::applyAlphaTest(pack->definition, programKey);
 });
 render::setShaderObjectIdResolver([this](const std::string& kind, const std::string& name, int fallback) {
  const ShaderPackDefinition* definition = activeDefinition();
  if(definition == nullptr) {
   return fallback;
  }
  const auto& ids =
      kind == "entity" ? definition->entityIds : kind == "item" ? definition->itemIds
                                                                : definition->blockIds;
  const std::string key = lower(name);
  if(const auto found = ids.find(key); found != ids.end()) {
   return found->second;
  }
  if(const auto found = ids.find("minecraft:" + key); found != ids.end()) {
   return found->second;
  }
  return fallback;
 });
 core::setProgramUniformUploader([this](gl::ShaderProgram& program) {
  WorldProgramBindContext ctx{};
  ctx.uniforms = &worldUniforms_;
  ctx.lightmapTexture = &lightmapTexture_;
  const bool interfaceProgram = interfaceProgramsActive();
  ShaderPackInstance* pack = interfaceProgram ? basePack_.get() : activePack();
  if(pack == nullptr) {
   pack = basePack_.get();
  }
  ctx.noiseTexture = (!interfaceProgram && pack != nullptr) ? pack->noiseTexture : 0;
  ctx.shadowDepthTexture = interfaceProgram ? -1 : shadowDepthTexture_;
  ctx.shadowOpaqueDepthTexture = interfaceProgram ? -1 : shadowOpaqueDepthTexture_;
  ctx.shadowColorTextures = interfaceProgram ? nullptr : shadowColorTextures_;
  ctx.shadowColorTextureCount = interfaceProgram ? 0 : shadowColorTextureCount_;
  const bool shadowPass = RenderCameraState::instance().frame().shadowPass;
  ctx.bindTextureAtlases = !interfaceProgram && !shadowPass &&
                          lastWorldProgramKey_.rfind("gbuffers_", 0) == 0;
  ctx.normalTexture = normalFallbackTexture_;
  ctx.specularTexture = specularFallbackTexture_;
  if(ctx.bindTextureAtlases && net::minecraft::client::Minecraft::INSTANCE != nullptr) {
   auto& textureManager = net::minecraft::client::Minecraft::INSTANCE->textureManager;
   core::activeTexture(gl::tex::Texture0);
   const int diffuseTexture = core::boundTexture();
   const int normalTexture = textureManager.getCompanionTextureId(diffuseTexture, "_n");
   const int specularTexture = textureManager.getCompanionTextureId(diffuseTexture, "_s");
   if(normalTexture > 0) ctx.normalTexture = static_cast<unsigned int>(normalTexture);
   if(specularTexture > 0) ctx.specularTexture = static_cast<unsigned int>(specularTexture);
   textureManager.getTextureDimensionsForId(diffuseTexture, ctx.atlasWidth, ctx.atlasHeight);
  }
  ctx.clearShadowBindsWhenNoPack = interfaceProgram || pack == nullptr;
  ctx.pack = interfaceProgram ? nullptr : pack;
  bindWorldProgram(program, ctx);
 });
}
ShaderPackManager::~ShaderPackManager() {
 stopDirectoryWatcher();
 render::setWorldProgramResolver(nullptr);
 render::setWorldPassDirectiveApplier(nullptr);
 render::setShaderObjectIdResolver(nullptr);
 core::setProgramUniformUploader(nullptr);
 if(lightmapTexture_ != 0) core::deleteTexture(lightmapTexture_);
 if(normalFallbackTexture_ != 0) core::deleteTexture(normalFallbackTexture_);
 if(specularFallbackTexture_ != 0) core::deleteTexture(specularFallbackTexture_);
}
namespace {
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
void applyPassBuckets(ShaderPackInstance& pack, ShaderPassBuckets&& buckets) {
 pack.postPasses = std::move(buckets.postPasses);
 pack.deferredPasses = std::move(buckets.deferredPasses);
 pack.computePasses = std::move(buckets.computePasses);
 pack.beginPasses = std::move(buckets.beginPasses);
 pack.shadowCompositePasses = std::move(buckets.shadowCompositePasses);
 pack.preparePasses = std::move(buckets.preparePasses);
 pack.setupPasses = std::move(buckets.setupPasses);
}
template <typename CompileFn>
bool dispatchSetupIfNeeded(ShaderPackInstance& pack, const FrameUniformSet& uniforms, int width, int height,
                           std::unordered_map<std::string, int>& textures,
                           std::unordered_map<std::string, int>& colorImages,
                           std::unordered_map<std::string, int>& volumes, CompileFn&& compileFn) {
 if(!gl::GLCore::computeSupported || (pack.setupWidth == width && pack.setupHeight == height)) {
  return true;
 }
 for(std::size_t passIndex : pack.setupPasses) {
  if(!ComputeDispatcher::dispatch(pack, pack.definition.passes[passIndex], uniforms, textures, colorImages,
                                  volumes, width, height, !pack.definition.allowConcurrentCompute,
                                  compileFn)) {
   return false;
  }
 }
 if(pack.definition.allowConcurrentCompute && !pack.setupPasses.empty()) {
  gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
 }
 pack.setupWidth = width;
 pack.setupHeight = height;
 return true;
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
void ShaderPackManager::ensurePbrFallbackTextures() {
 if(normalFallbackTexture_ == 0) uploadRgbaStub(normalFallbackTexture_, 127, 127, 255, 255);
 if(specularFallbackTexture_ == 0) uploadRgbaStub(specularFallbackTexture_, 0, 0, 0, 0);
}
void ShaderPackManager::refreshResourcePackState() {
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
 const auto apply = [this](ShaderPackInstance* pack) {
  if(pack == nullptr) return;
  const auto setFormat = [this](auto&& self, ShaderPackDefinition& definition) -> void {
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
   pack->compiledPrograms.clear();
   pack->logged.clear();
   if(pack->programs != nullptr) pack->programs->clear();
  }
 };
 apply(basePack_.get());
 for(auto& pack : packs_) apply(pack.get());
}
void ShaderPackManager::reloadWorldMeshes() {
 if(net::minecraft::client::Minecraft::INSTANCE == nullptr ||
    net::minecraft::client::Minecraft::INSTANCE->worldRenderer == nullptr) {
  return;
 }
 net::minecraft::client::Minecraft::INSTANCE->worldRenderer->reload();
}
void ShaderPackManager::reload() {
 packs_.clear();
 summaries_.clear();
 activeIndex_ = kNoActivePack;
 const std::filesystem::path directory = gameDirectory_ / "shaderpacks";
 std::error_code ec;
 std::filesystem::create_directories(directory, ec);
 basePack_ = loadDirectoryPack(directory / "vanilla");
 std::vector<std::filesystem::path> archives;
 std::vector<std::filesystem::path> dirs;
 if(std::filesystem::is_directory(directory)) {
  for(const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
   if(ec) {
    break;
   }
   if(entry.is_directory(ec)) {
    if(lower(entry.path().filename().string()) != "vanilla") {
     dirs.push_back(entry.path());
    }
   } else if(entry.is_regular_file(ec) && lower(entry.path().extension().string()) == ".zip") {
    archives.push_back(entry.path());
   }
  }
 }
 std::sort(dirs.begin(), dirs.end());
 std::sort(archives.begin(), archives.end());
 for(const auto& path : dirs) {
  addDirectoryPack(path);
 }
 for(const auto& path : archives) {
  addZipPack(path);
 }
 const std::string selected = options_ != nullptr ? options_->shaderPack : std::string{};
 if(!selected.empty() && selected != "OFF") {
  for(std::size_t i = 0; i < packs_.size(); ++i) {
   if(packs_[i]->summary.key == selected || packs_[i]->summary.name == selected) {
    activeIndex_ = i;
    break;
   }
  }
 }
 refreshSummaries();
 const bool voxelization = activeDefinition() != nullptr && activeDefinition()->voxelizeLightBlocks;
 render::block::BlockRenderManager::setVoxelizeLightBlocks(voxelization);
 applyBlockIds(activeDefinition());
 reloadWorldMeshes();
 packDirectoryStamp_ = packDirectoryStamp(directory);
 watchedStamp_.store(packDirectoryStamp_, std::memory_order_relaxed);
 directoryChanged_.store(false, std::memory_order_relaxed);
 resetPresentState();
 refreshResourcePackState();
 warmBasePrograms();
}
void ShaderPackManager::startDirectoryWatcher() {
 stopDirectoryWatcher();
 directoryWatcher_ = std::jthread([this](const std::stop_token& stop) { directoryWatchLoop(stop); });
}
void ShaderPackManager::stopDirectoryWatcher() {
 if(directoryWatcher_.joinable()) {
  directoryWatcher_.request_stop();
  directoryWatcher_.join();
 }
}
void ShaderPackManager::directoryWatchLoop(const std::stop_token& stop) {
#ifdef _WIN32
 SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
 while(!stop.stop_requested()) {
  for(int i = 0; i < 20 && !stop.stop_requested(); ++i) {
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if(stop.stop_requested()) {
   break;
  }
  const std::uint64_t stamp = packDirectoryStamp(gameDirectory_ / "shaderpacks");
  const std::uint64_t previous = watchedStamp_.load(std::memory_order_relaxed);
  if(stamp != previous) {
   watchedStamp_.store(stamp, std::memory_order_relaxed);
   directoryChanged_.store(true, std::memory_order_release);
  }
 }
}
void ShaderPackManager::poll() {
 if(!directoryChanged_.exchange(false, std::memory_order_acq_rel)) {
  return;
 }
 packDirectoryStamp_ = watchedStamp_.load(std::memory_order_relaxed);
 reload();
}
void ShaderPackManager::activateDimension(ShaderPackInstance& pack, const net::minecraft::World* world) {
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
 const auto exact = pack.rootDefinition.dimensionDefinitions.find(key);
 const auto fallback = pack.rootDefinition.dimensionDefinitions.find("*");
 const ShaderPackDefinition* selected =
     exact != pack.rootDefinition.dimensionDefinitions.end()
         ? exact->second.get()
     : fallback != pack.rootDefinition.dimensionDefinitions.end() ? fallback->second.get()
                                                                  : &pack.rootDefinition;
 const std::string selectedKey = selected == &pack.rootDefinition
                                     ? std::string{}
                                 : exact != pack.rootDefinition.dimensionDefinitions.end() ? key
                                                                                           : "*";
 if(pack.dimensionKey == selectedKey) {
  return;
 }
 pack.clearGpuResources();
 pack.dimensionKey = selectedKey;
 pack.definition = pack.rootDefinition;
 if(selected != &pack.rootDefinition) {
  for(const auto& [name, program] : selected->programs) {
   pack.definition.programs[name] = program;
  }
  for(const auto& [name, target] : selected->targets) {
   pack.definition.targets[name] = target;
  }
  pack.definition.gbufferColorBuffers =
      std::max(pack.definition.gbufferColorBuffers, selected->gbufferColorBuffers);
  pack.definition.shadowColorBuffers = std::max(pack.definition.shadowColorBuffers, selected->shadowColorBuffers);
  if(selected->shadowMapResolution > 0) {
   pack.definition.shadowMapResolution = selected->shadowMapResolution;
  }
  if(!selected->customUniforms.empty()) {
   pack.definition.customUniforms = selected->customUniforms;
   std::string customError;
   pack.customUniforms.setOptions(pack.settings);
   pack.customUniforms.compile(pack.definition.customUniforms, customError);
  }
  for(const ShaderPass& pass : selected->passes) {
   const auto match = std::find_if(pack.definition.passes.begin(), pack.definition.passes.end(),
                                   [&pass](const ShaderPass& root) {
                                    return root.type == pass.type && root.name == pass.name;
                                   });
   if(match == pack.definition.passes.end()) {
    pack.definition.passes.push_back(pass);
   } else {
    *match = pass;
   }
  }
 }
 pack.compiledPrograms.clear();
 pack.programs = std::make_unique<gl::ProgramCache>();
 ShaderPassBuckets buckets;
 indexShaderPasses(pack.definition, pack.settings, buckets);
 applyPassBuckets(pack, std::move(buckets));
 if(&pack == activePack()) {
  applyBlockIds(&pack.definition);
 }
}
std::unique_ptr<ShaderPackInstance> ShaderPackManager::loadDirectoryPack(const std::filesystem::path& path) {
 auto pack = std::make_unique<ShaderPackInstance>();
 pack->path = path;
 pack->directory = true;
 pack->summary.key = path.filename().string();
 if(ShaderPackLoader::load(
        directoryResources(path),
        [&pack](std::string_view resource) { return ShaderPackCompiler::readText(*pack, std::string(resource)); },
        pack->definition, pack->sourceOptions, pack->summary.error)) {
  pack->summary.valid = true;
  pack->rootDefinition = pack->definition;
  pack->summary.name = path.filename().string();
  pack->summary.version = pack->definition.version;
  for(const PackSetting& setting : pack->definition.settings) {
   pack->settings.emplace(setting.key, defaultSettingValue(setting));
  }
  ShaderPassBuckets buckets;
  indexShaderPasses(pack->definition, pack->settings, buckets);
  applyPassBuckets(*pack, std::move(buckets));
  pack->programs = std::make_unique<gl::ProgramCache>();
  if(!pack->definition.customUniforms.empty()) {
   std::string customError;
   pack->customUniforms.setOptions(pack->settings);
   if(!pack->customUniforms.compile(pack->definition.customUniforms, customError)) {
    ClientLog::LOGGER.log(net::minecraft::util::logging::LogLevel::Warning,
                          "[shaderpack:" + pack->summary.name + "] " + customError);
   }
  }
 }
 if(pack->summary.name.empty()) {
  pack->summary.name = path.filename().string();
 }
 return pack;
}
void ShaderPackManager::addDirectoryPack(const std::filesystem::path& path) {
 packs_.push_back(loadDirectoryPack(path));
}
void ShaderPackManager::addZipPack(const std::filesystem::path& path) {
 auto pack = std::make_unique<ShaderPackInstance>();
 pack->path = path;
 pack->directory = false;
 pack->summary.key = path.filename().string();
 pack->zip = std::make_unique<resource::pack::ZippedTexturePack>(path);
 pack->zip->open();
 if(ShaderPackLoader::load(
        zipResources(*pack->zip),
        [&pack](std::string_view resource) { return ShaderPackCompiler::readText(*pack, std::string(resource)); },
        pack->definition, pack->sourceOptions, pack->summary.error)) {
  pack->summary.valid = true;
  pack->rootDefinition = pack->definition;
  pack->summary.name = path.filename().string();
  pack->summary.version = pack->definition.version;
  for(const PackSetting& setting : pack->definition.settings) {
   pack->settings.emplace(setting.key, defaultSettingValue(setting));
  }
  ShaderPassBuckets buckets;
  indexShaderPasses(pack->definition, pack->settings, buckets);
  applyPassBuckets(*pack, std::move(buckets));
  pack->programs = std::make_unique<gl::ProgramCache>();
  if(!pack->definition.customUniforms.empty()) {
   std::string customError;
   pack->customUniforms.setOptions(pack->settings);
   if(!pack->customUniforms.compile(pack->definition.customUniforms, customError)) {
    ClientLog::LOGGER.log(net::minecraft::util::logging::LogLevel::Warning,
                          "[shaderpack:" + pack->summary.name + "] " + customError);
   }
  }
 }
 if(pack->summary.name.empty()) {
  pack->summary.name = path.filename().string();
 }
 packs_.push_back(std::move(pack));
}
void ShaderPackManager::refreshSummaries() {
 summaries_.clear();
 summaries_.reserve(packs_.size());
 for(std::size_t i = 0; i < packs_.size(); ++i) {
  ShaderPackSummary summary = packs_[i]->summary;
  summary.selected = i == activeIndex_;
  summaries_.push_back(std::move(summary));
 }
}
bool ShaderPackManager::select(const std::string& key) {
 if(key.empty() || lower(key) == "off" || lower(key) == "none") {
  activeIndex_ = kNoActivePack;
  if(options_ != nullptr) {
   options_->shaderPack.clear();
   options_->save();
  }
  refreshSummaries();
  render::block::BlockRenderManager::setVoxelizeLightBlocks(false);
  applyBlockIds(basePack_ != nullptr && basePack_->summary.valid ? &basePack_->definition : nullptr);
  reloadWorldMeshes();
  resetPresentState();
  warmBasePrograms();
  return true;
 }
 for(std::size_t i = 0; i < packs_.size(); ++i) {
  if(packs_[i]->summary.key != key && packs_[i]->summary.name != key) {
   continue;
  }
  if(!packs_[i]->summary.valid) {
   return false;
  }
  activeIndex_ = i;
  if(options_ != nullptr) {
   options_->shaderPack = packs_[i]->summary.key;
   options_->save();
  }
  refreshSummaries();
  const bool voxelization = packs_[i]->definition.voxelizeLightBlocks;
  render::block::BlockRenderManager::setVoxelizeLightBlocks(voxelization);
  applyBlockIds(&packs_[i]->definition);
  reloadWorldMeshes();
  resetPresentState();
  warmBasePrograms();
  return true;
 }
 return false;
}
void ShaderPackManager::resetPresentState() {
 pipelinePhase_ = WorldPipelinePhase::None;
 packWroteToScreen_ = false;
 shadowDepthTexture_ = -1;
 shadowOpaqueDepthTexture_ = -1;
 shadowColorTextureCount_ = 0;
 std::fill(std::begin(shadowColorTextures_), std::end(shadowColorTextures_), 0);
 if(!glutil::hasGlContext()) {
  return;
 }
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
void ShaderPackManager::warmBasePrograms() {
 if(basePack_ == nullptr || !basePack_->summary.valid || !glutil::hasGlContext()) {
  return;
 }
 static constexpr const char* kKeys[] = {
     "gbuffers_basic",
     "gbuffers_terrain",
     "gbuffers_entities",
     "gbuffers_skybasic",
     "gbuffers_skytextured",
     "gbuffers_water",
     "gbuffers_hand",
     "gbuffers_gui",
     "gbuffers_gui_textured",
     "gbuffers_text",
     "gbuffers_item",
     "final",
 };
 for(const char* key : kKeys) {
  programFromPack(*basePack_, key);
 }
 resetPresentState();
 updateLightmap(net::minecraft::client::Minecraft::INSTANCE != nullptr
                    ? net::minecraft::client::Minecraft::INSTANCE->world
                    : nullptr);
}
bool ShaderPackManager::setSetting(const std::string& key, std::string value) {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr || pack->definition.settings.empty()) {
  return false;
 }
 for(const PackSetting& setting : pack->definition.settings) {
  if(setting.key == key) {
   std::string normalized;
   if(!glutil::normalizeSettingValue(setting, value, normalized)) {
    return false;
   }
   pack->settings[key] = std::move(normalized);
   pack->customUniforms.setOptions(pack->settings);
   if(!pack->definition.customUniforms.empty()) {
    std::string customError;
    pack->customUniforms.compile(pack->definition.customUniforms, customError);
   }
   if(pack->programs != nullptr) {
    pack->programs->clear();
    pack->compiledPrograms.clear();
    pack->logged.clear();
   }
   ShaderPassBuckets buckets;
   indexShaderPasses(pack->definition, pack->settings, buckets);
   applyPassBuckets(*pack, std::move(buckets));
   return true;
  }
 }
 return false;
}
std::string ShaderPackManager::settingValue(const std::string& key) const {
 const ShaderPackInstance* pack = activePack();
 if(pack == nullptr) {
  return {};
 }
 const auto found = pack->settings.find(key);
 return found == pack->settings.end() ? std::string() : found->second;
}
ShaderPackInstance* ShaderPackManager::activePack() noexcept {
 if(activeIndex_ < packs_.size()) {
  return packs_[activeIndex_].get();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_.get() : nullptr;
}
const ShaderPackInstance* ShaderPackManager::activePack() const noexcept {
 if(activeIndex_ < packs_.size()) {
  return packs_[activeIndex_].get();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_.get() : nullptr;
}
const ShaderPackDefinition* ShaderPackManager::activeDefinition() const noexcept {
 const ShaderPackInstance* pack = activePack();
 return pack != nullptr && pack->summary.valid ? &pack->definition : nullptr;
}
const ShaderPackDefinition* ShaderPackManager::meshDefinition() const noexcept {
 if(const ShaderPackDefinition* active = activeDefinition(); active != nullptr) {
  return active;
 }
 return basePack_ != nullptr && basePack_->summary.valid ? &basePack_->definition : nullptr;
}
bool ShaderPackManager::activeHasPostProcess() const {
 const ShaderPackDefinition* definition = activeDefinition();
 if(definition == nullptr) {
  return false;
 }
 const ShaderPackInstance* pack = activePack();
 return pack != nullptr &&
        (!pack->postPasses.empty() || !pack->deferredPasses.empty() || !pack->computePasses.empty() ||
         !pack->setupPasses.empty() || !pack->beginPasses.empty() || !pack->shadowCompositePasses.empty() ||
         !pack->preparePasses.empty());
}
int ShaderPackManager::shadowMapResolution() const {
 const ShaderPackDefinition* definition = activeDefinition();
 return definition == nullptr || !definition->shadowEnabled ? 0 : definition->shadowMapResolution;
}
int ShaderPackManager::shadowColorBuffers() const {
 const ShaderPackDefinition* definition = activeDefinition();
 return definition == nullptr ? 0 : definition->shadowColorBuffers;
}
gl::ShaderProgram* ShaderPackManager::programFromPack(ShaderPackInstance& pack, const std::string& key) {
 return pack.summary.valid && pack.definition.programs.contains(key)
            ? ShaderPackCompiler::compile(pack, key, [this](ShaderPackInstance& p, const std::string& m) {
               logOnce(p, m);
              })
            : nullptr;
}
gl::ShaderProgram* ShaderPackManager::worldProgram(const std::string& key) {
 lastWorldProgramKey_ = key;
 ShaderPackInstance* pack = interfaceProgramsActive() ? basePack_.get() : activePack();
 if(pack == nullptr) {
  pack = basePack_.get();
 }
 if(pack == nullptr) {
  return nullptr;
 }
 core::RenderStage renderStage = core::RenderStage::None;
 if(key == "gbuffers_terrain_solid") {
  renderStage = core::RenderStage::TerrainSolid;
 } else if(key == "gbuffers_terrain_cutout") {
  renderStage = core::RenderStage::TerrainSolid;
 } else if(key == "gbuffers_damagedblock") {
  renderStage = core::RenderStage::TerrainSolid;
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
 const bool shadowPass = RenderCameraState::instance().frame().shadowPass;
 std::string programKey = key;
 if(shadowPass) {
  if(key == "gbuffers_terrain_solid") {
   programKey = "shadow_solid";
  } else if(key == "gbuffers_terrain_cutout" || key == "gbuffers_damagedblock") {
   programKey = "shadow_cutout";
  } else if(key == "gbuffers_water") {
   programKey = "shadow_water";
  } else if(key.rfind("gbuffers_entities", 0) == 0 || key == "gbuffers_item" || key == "gbuffers_beaconbeam" ||
            key == "gbuffers_lightning") {
   programKey = "shadow_entities";
  } else if(key.rfind("gbuffers_block", 0) == 0) {
   programKey = "shadow_block";
  } else {
   programKey = "shadow";
  }
  if(!pack->definition.programs.contains(programKey)) {
   programKey = "shadow";
  }
 }
 if(!isProgramEnabled(pack->definition, pack->settings, programKey)) {
  if(shadowPass && programKey != "shadow" && isProgramEnabled(pack->definition, pack->settings, "shadow") &&
     pack->definition.programs.contains("shadow")) {
   programKey = "shadow";
  } else {
   return nullptr;
  }
 }
 gl::ShaderProgram* program = programFromPack(*pack, programKey);
 return program;
}
void ShaderPackManager::prepareFrame(net::minecraft::World* world) {
 ensurePbrFallbackTextures();
 refreshResourcePackState();
 ShaderPackInstance* pack = activePack();
 if(pack != nullptr) {
  activateDimension(*pack, world);
 }
 if(basePack_ != nullptr) {
  activateDimension(*basePack_, world);
 }
 warmBasePrograms();
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
   if(name.rfind("item.", 0) == 0 || name.rfind("tile.", 0) == 0) {
    name.erase(0, 5);
   }
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
 const bool resourcesReady = pack != nullptr &&
                             ShaderPackResources::ensure(*pack, width, height, lightmapTexture_,
                                                         [](const ShaderPackInstance& p, const std::string& path) {
                                                          return ShaderPackCompiler::readText(p, path);
                                                         });
 if(resourcesReady) {
  for(const CustomImage& declaration : pack->definition.images) {
   const auto found = pack->images.find(declaration.name);
   if(declaration.clearEachFrame && found != pack->images.end() && gl::GLCore::clearTexImage != nullptr) {
    gl::GLCore::clearTexImage(found->second.texture, 0, glutil::pixelFormat(declaration.format),
                              glutil::pixelType(declaration.pixelType), nullptr);
   }
  }
  std::unordered_map<std::string, int> textures;
  std::unordered_map<std::string, int> colorImages;
  std::unordered_map<std::string, int> volumes;
  ShaderPackResources::addTextures(*pack, "setup", textures, volumes);
  if(pack->colorTargets.valid()) {
   pack->colorTargets.fillImageBindings(colorImages);
  }
  dispatchSetupIfNeeded(*pack, worldUniforms_, width, height, textures, colorImages, volumes,
                        [this](ShaderPackInstance& p, const std::string& name) {
                         return ShaderPackCompiler::compile(
                             p, name, [this](ShaderPackInstance& p2, const std::string& m) { logOnce(p2, m); });
                        });
 }
}
void ShaderPackManager::refreshLightmap(net::minecraft::World* world) {
 updateLightmap(world);
}
void ShaderPackManager::setFrameUniforms(const FrameUniformSet& frame) {
 worldUniforms_ = frame;
 if(const ShaderPackDefinition* def = activeDefinition()) {
  worldUniforms_.wetness = updateWetnessSmooth(worldUniforms_.rainStrength, worldUniforms_.frameTime,
                                               def->wetnessHalflife, def->drynessHalflife);
 } else {
  worldUniforms_.wetness = worldUniforms_.rainStrength;
 }
 if(ShaderPackInstance* pack = activePack()) {
  pack->customUniforms.evaluate(worldUniforms_);
 }
 core::advanceProgramUniforms();
}
bool ShaderPackManager::renderBegin() {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr) {
  return false;
 }
 pack->publishedTextures.clear();
 shadowDepthTexture_ = -1;
 shadowOpaqueDepthTexture_ = -1;
 shadowColorTextureCount_ = 0;
 return runPasses(*pack, pack->beginPasses, false, "begin", -1, -1, nullptr, 0);
}
void ShaderPackManager::updateLightmap(const net::minecraft::World* world) {
 if(!glutil::hasGlContext()) {
  return;
 }
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
bool ShaderPackManager::hasDeferredPasses() const {
 const ShaderPackInstance* pack = activePack();
 if(pack == nullptr) {
  return false;
 }
 if(!pack->deferredPasses.empty()) {
  return true;
 }
 return std::any_of(pack->computePasses.begin(), pack->computePasses.end(), [pack](std::size_t index) {
  return ComputeDispatcher::matchesStage(pack->definition.passes[index].name, "deferred");
 });
}
bool ShaderPackManager::renderPreWorld(int shadowDepthTextureId,
                                       int shadowOpaqueDepthTextureId,
                                       const int* shadowColorTextureIds,
                                       int shadowColorTextureCount) {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr) {
  return false;
 }
 const auto hasCompute = [pack](const std::string& stage) {
  return std::any_of(pack->computePasses.begin(), pack->computePasses.end(), [pack, &stage](std::size_t index) {
   return ComputeDispatcher::matchesStage(pack->definition.passes[index].name, stage);
  });
 };
 {
  const auto& def = pack->definition;
  auto generate = [](int textureId) {
   if(textureId <= 0 || gl::GLCore::generateMipmap == nullptr) return;
   core::bindTexture(glutil::kTexture2D, textureId);
   gl::GLCore::generateMipmap(glutil::kTexture2D);
  };
  if(def.shadowtexMipmap[0]) generate(shadowDepthTextureId);
  if(def.shadowtexMipmap[1]) generate(shadowOpaqueDepthTextureId);
  for(int i = 0; i < 2 && i < shadowColorTextureCount; ++i)
   if(def.shadowcolorMipmap[i] && shadowColorTextureIds != nullptr) generate(shadowColorTextureIds[i]);
 }
 bool rendered = false;
 if(!pack->shadowCompositePasses.empty() || hasCompute("shadowcomp")) {
  rendered = runPasses(*pack, pack->shadowCompositePasses, false, "shadowcomp",
                       shadowDepthTextureId, shadowOpaqueDepthTextureId, shadowColorTextureIds,
                       shadowColorTextureCount) ||
             rendered;
 }
 if(!pack->preparePasses.empty() || hasCompute("prepare")) {
  rendered = runPasses(*pack, pack->preparePasses, false, "prepare", shadowDepthTextureId,
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
 worldUniforms_.normalAvailable = pack->colorTargets.colorCount() > 1 ? 1 : 0;
 core::advanceProgramUniforms();
 return rendered;
}
bool ShaderPackManager::renderDeferred(int shadowDepthTextureId,
                                       int shadowOpaqueDepthTextureId,
                                       const int* shadowColorTextureIds,
                                       int shadowColorTextureCount) {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr) {
  return false;
 }
 const bool hasComputeDeferred =
     std::any_of(pack->computePasses.begin(), pack->computePasses.end(), [pack](std::size_t index) {
      return ComputeDispatcher::matchesStage(pack->definition.passes[index].name, "deferred");
     });
 if(pack->deferredPasses.empty() && !hasComputeDeferred) {
  return false;
 }
 return runPasses(*pack, pack->deferredPasses, false, "deferred", shadowDepthTextureId,
                  shadowOpaqueDepthTextureId, shadowColorTextureIds, shadowColorTextureCount);
}
bool ShaderPackManager::renderPostProcess(int shadowDepthTextureId,
                                          int shadowOpaqueDepthTextureId,
                                          const int* shadowColorTextureIds,
                                          int shadowColorTextureCount) {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr) {
  return false;
 }
 const bool hasComputeComposite =
     std::any_of(pack->computePasses.begin(), pack->computePasses.end(), [pack](std::size_t index) {
      const std::string& name = pack->definition.passes[index].name;
      return ComputeDispatcher::matchesStage(name, "composite") ||
             ComputeDispatcher::matchesStage(name, "final");
     });
 if(!pack->postPasses.empty() || hasComputeComposite || !pack->setupPasses.empty()) {
  runPasses(*pack, pack->postPasses, true, "composite", shadowDepthTextureId, shadowOpaqueDepthTextureId,
            shadowColorTextureIds, shadowColorTextureCount);
 }
 if(!packWroteToScreen_) {
  const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
  const int screenW = minecraft != nullptr ? std::max(1, minecraft->displayWidth) : 1;
  const int screenH = minecraft != nullptr ? std::max(1, minecraft->displayHeight) : 1;
  presentFinalToScreen(screenW, screenH);
 }
 return packWroteToScreen_;
}
void ShaderPackManager::sampleCenterDepth() {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr || !pack->colorTargets.valid()) {
  return;
 }
 const auto& targets = pack->colorTargets;
 if(targets.width() <= 0 || targets.height() <= 0 || targets.depthTexture() == 0) {
  return;
 }
 bindScene();
 float depth = 1.0f;
 ::glReadPixels(targets.width() / 2, targets.height() / 2, 1, 1, 0x1902, 0x1406, &depth);
 endScene();
 const float nearPlane = worldUniforms_.nearPlane > 0.0f ? worldUniforms_.nearPlane : 0.05f;
 const float farPlane = worldUniforms_.farPlane > nearPlane ? worldUniforms_.farPlane : 256.0f;
 const float halfLife = activeDefinition() != nullptr ? activeDefinition()->centerDepthHalflife : 1.0f;
 worldUniforms_.centerDepthSmooth =
     updateCenterDepthSmooth(depth, nearPlane, farPlane, worldUniforms_.frameTime, halfLife);
}
void ShaderPackManager::captureOpaqueDepth() {
 captureDepth(1);
}
void ShaderPackManager::captureHandDepth() {
 captureDepth(0);
}
void ShaderPackManager::captureDepth(std::size_t index) {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr || !pack->colorTargets.valid() || index >= 2) {
  return;
 }
 const int width = pack->colorTargets.width();
 const int height = pack->colorTargets.height();
 if(width <= 0 || height <= 0 || pack->colorTargets.depthTexture() == 0) {
  return;
 }
 bindScene();
 if(pack->depthTextures[index] == 0) {
  pack->depthTextures[index] = core::genTexture();
  pack->depthTextureW[index] = 0;
  pack->depthTextureH[index] = 0;
 }
 if(pack->depthTextures[index] == 0) {
  return;
 }
 core::bindTexture(static_cast<int>(pack->depthTextures[index]));
 if(pack->depthTextureW[index] != width || pack->depthTextureH[index] != height) {
  ::glTexImage2D(glutil::kTexture2D, 0, 0x81A6, width, height, 0, 0x1902, 0x1405, nullptr);
  ::glTexParameteri(glutil::kTexture2D, 0x2801, 0x2600);
  ::glTexParameteri(glutil::kTexture2D, 0x2800, 0x2600);
  ::glTexParameteri(glutil::kTexture2D, 0x2802, 0x812F);
  ::glTexParameteri(glutil::kTexture2D, 0x2803, 0x812F);
  pack->depthTextureW[index] = width;
  pack->depthTextureH[index] = height;
 }
 ::glCopyTexSubImage2D(glutil::kTexture2D, 0, 0, 0, 0, 0, width, height);
}
void ShaderPackManager::logOnce(ShaderPackInstance& pack, const std::string& message) const {
 if(!pack.logged.insert(message).second) {
  return;
 }
 ClientLog::LOGGER.info("[shaderpack:" + pack.definition.name + "] " + message);
}
std::vector<render::ColorFormat> ShaderPackManager::sceneColorFormats() const {
 const ShaderPackInstance* pack = activePack();
 const int count = pack != nullptr && pack->summary.valid ? std::clamp(pack->definition.gbufferColorBuffers, 1, 32)
                                                          : 1;
 std::vector<render::ColorFormat> formats;
 formats.reserve(static_cast<std::size_t>(count));
 for(int i = 0; i < count; ++i) {
  render::ColorFormat format = render::ColorFormat::Rgba8;
  if(pack != nullptr) {
   const auto found = pack->definition.targets.find("colortex" + std::to_string(i));
   if(found != pack->definition.targets.end()) {
    format = glutil::parseFormat(found->second.format);
   }
  }
  formats.push_back(format);
 }
 return formats;
}
bool ShaderPackManager::ensureSceneTargets(int width, int height) {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr) {
  return false;
 }
 return pack->colorTargets.ensure(width, height, sceneColorFormats());
}
void ShaderPackManager::bindScene() {
 pipelinePhase_ = WorldPipelinePhase::World;
 if(ShaderPackInstance* pack = activePack(); pack != nullptr) {
  pack->colorTargets.bindGbuffers();
 }
}
void ShaderPackManager::endScene() {
 if(ShaderPackInstance* pack = activePack(); pack != nullptr) {
  pack->colorTargets.endGbuffers();
 }
}
void ShaderPackManager::destroyScene() {
 if(ShaderPackInstance* pack = activePack(); pack != nullptr) {
  pack->colorTargets.destroy();
 }
}
int ShaderPackManager::sceneColorCount() const {
 const ShaderPackInstance* pack = activePack();
 return pack != nullptr ? pack->colorTargets.colorCount() : 0;
}
unsigned int ShaderPackManager::sceneDepthTexture() const {
 const ShaderPackInstance* pack = activePack();
 return pack != nullptr ? pack->colorTargets.depthTexture() : 0u;
}
void ShaderPackManager::presentFinalToScreen(int screenWidth, int screenHeight) {
 if(packWroteToScreen_) {
  return;
 }
 ShaderPackInstance* scenePack = activePack();
 if(scenePack == nullptr || !scenePack->colorTargets.valid()) {
  return;
 }
 if(scenePack->colorTargets.readTexture(0) == 0 || !glutil::hasGlContext()) {
  return;
 }
 ShaderPackInstance* programPack = scenePack;
 gl::ShaderProgram* program = programFromPack(*scenePack, "final");
 if(program == nullptr && basePack_ != nullptr && basePack_->summary.valid) {
  program = programFromPack(*basePack_, "final");
  programPack = basePack_.get();
 }
 if(program == nullptr || !program->valid()) {
  return;
 }
 render::ColorTargets& targets = scenePack->colorTargets;
 const int width = targets.width();
 const int height = targets.height();
 if(width <= 0 || height <= 0) {
  return;
 }
 if(!ShaderPackResources::ensure(*programPack, width, height, lightmapTexture_,
                                 [](const ShaderPackInstance& p, const std::string& path) {
                                  return ShaderPackCompiler::readText(p, path);
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
 gl::GLCore::bindFramebuffer(0x8D40, 0);
 core::viewport(0, 0, screenWidth, screenHeight);
 glutil::applyBufferBlends(programPack->definition, "final");
 glutil::applyAlphaTest(programPack->definition, "final");
 program->bind();
 std::unordered_map<std::string, int> volumeTextures;
 glutil::refreshTextureAliases(textures);
 ShaderPackResources::addTextures(*scenePack, "composite", textures, volumeTextures);
 glutil::bindSamplers(*program, textures, volumeTextures, glutil::maxTextureUnits());
 ShaderPackResources::bind(*programPack, *program, 0);
 FrameUniformSet frameUniforms = worldUniforms_;
 frameUniforms.viewWidth = static_cast<float>(width);
 frameUniforms.viewHeight = static_cast<float>(height);
 frameUniforms.aspectRatio = frameUniforms.viewWidth / std::max(frameUniforms.viewHeight, 1.0f);
 frameUniforms.normalAvailable = targets.colorCount() > 1 ? 1 : 0;
 uploadShaderUniforms(*program, frameUniforms);
 uploadIdentityDrawMatrices(*program);
 programPack->customUniforms.upload(*program);
 program->bind();
 core::drawFullscreen();
 glutil::releaseSamplers(glutil::maxTextureUnits());
 core::activeTexture(gl::tex::Texture0);
}
void ShaderPackManager::clearScene(float fogR, float fogG, float fogB) {
 ShaderPackInstance* pack = activePack();
 if(pack == nullptr || !pack->colorTargets.valid()) {
  return;
 }
 auto& scene = pack->colorTargets;
 std::vector<bool> clear(static_cast<std::size_t>(scene.colorCount()), true);
 std::vector<std::array<float, 4>> colors(static_cast<std::size_t>(scene.colorCount()));
 for(int i = 0; i < scene.colorCount(); ++i) {
  if(i == 0) {
   colors[static_cast<std::size_t>(i)] = {fogR, fogG, fogB, 1.0f};
  } else if(i == 1) {
   colors[static_cast<std::size_t>(i)] = {1.0f, 1.0f, 1.0f, 1.0f};
  } else {
   colors[static_cast<std::size_t>(i)] = {0.0f, 0.0f, 0.0f, 0.0f};
  }
  const auto found = pack->definition.targets.find("colortex" + std::to_string(i));
  if(found == pack->definition.targets.end()) {
   continue;
  }
  clear[static_cast<std::size_t>(i)] = found->second.clear;
  if(found->second.customClearColor) {
   std::copy(std::begin(found->second.clearColor), std::end(found->second.clearColor),
             colors[static_cast<std::size_t>(i)].begin());
  }
 }
 scene.clearColors(clear, colors);
}
void ShaderPackManager::applyBlockIds(const ShaderPackDefinition* definition) {
 std::array<int, net::minecraft::block::Block::BLOCK_COUNT> ids{};
 for(int id = 0; id < static_cast<int>(ids.size()); ++id) {
  ids[static_cast<std::size_t>(id)] = id;
  if(definition == nullptr) {
   continue;
  }
  net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
  if(block == nullptr) {
   continue;
  }
  std::string name = lower(block->getTranslationKey());
  if(name.rfind("tile.", 0) == 0) {
   name.erase(0, 5);
  }
  if(const auto found = definition->blockIds.find(name); found != definition->blockIds.end()) {
   ids[static_cast<std::size_t>(id)] = found->second;
  } else if(const auto found = definition->blockIds.find("minecraft:" + name); found != definition->blockIds.end()) {
   ids[static_cast<std::size_t>(id)] = found->second;
  }
 }
 render::setShaderBlockIds(ids);
}
namespace {
void refreshColorMaps(render::ColorTargets& targets, std::unordered_map<std::string, int>& textures,
                      std::unordered_map<std::string, int>& colorImages) {
 textures.clear();
 colorImages.clear();
 targets.fillReadSamplers(textures);
 targets.fillImageBindings(colorImages);
}
} // namespace
bool ShaderPackManager::runPasses(ShaderPackInstance& pack, const std::vector<std::size_t>& passes, bool present,
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
 if(!targets.valid() || targets.depthTexture() == 0) {
  return false;
 }
 if(!glutil::hasGlContext() || width <= 0 || height <= 0) {
  return false;
 }
 if(!ShaderPackResources::ensure(pack, width, height, lightmapTexture_,
                                 [](const ShaderPackInstance& p, const std::string& path) {
                                  return ShaderPackCompiler::readText(p, path);
                                 })) {
  logOnce(pack, "pack GPU resources could not be allocated");
  return false;
 }
 int destinationFramebuffer = 0;
 ::glGetIntegerv(0x8CA6, &destinationFramebuffer);
 std::vector<std::pair<std::size_t, gl::ShaderProgram*>> programChain;
 programChain.reserve(passes.size());
 for(std::size_t passIndex : passes) {
  if(passIndex >= pack.definition.passes.size()) {
   continue;
  }
  const ShaderPass& pass = pack.definition.passes[passIndex];
  if(pack.definition.programs.count(pass.program) == 0) {
   logOnce(pack, "pass '" + pass.name + "' references unknown program '" + pass.program + "'");
   return false;
  }
  gl::ShaderProgram* program = ShaderPackCompiler::compile(pack, pass.program,
                                                           [this](ShaderPackInstance& p, const std::string& m) {
                                                            logOnce(p, m);
                                                           });
  if(program == nullptr) {
   logOnce(pack, "pass '" + pass.name + "' program '" + pass.program + "' is unusable");
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
   if(valid) {
    core::viewport(saved[0], saved[1], saved[2], saved[3]);
   }
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
 if(pack.depthTextures[0] != 0) {
  textures["depthtex1"] = static_cast<int>(pack.depthTextures[0]);
 }
 if(pack.depthTextures[1] != 0) {
  textures["depthtex2"] = static_cast<int>(pack.depthTextures[1]);
 } else if(pack.depthTextures[0] != 0) {
  textures["depthtex2"] = static_cast<int>(pack.depthTextures[0]);
 }
 if(shadowDepthTextureId >= 0) {
  textures["shadowtex0"] = shadowDepthTextureId;
  textures["shadowtex0HW"] = shadowDepthTextureId;
  const int opaqueDepth =
      shadowOpaqueDepthTextureId >= 0 ? shadowOpaqueDepthTextureId : shadowDepthTextureId;
  textures["shadowtex1"] = opaqueDepth;
  textures["shadowtex1HW"] = opaqueDepth;
 }
 targets.applyPreFlips(pack.definition, stage);
 refreshColorMaps(targets, textures, colorImages);
 for(const auto& [name, texture] : pack.publishedTextures) {
  textures[name] = texture;
 }
 for(int i = 0; i < std::min(shadowColorTextureCount, 8); ++i) {
  if(shadowColorTextureIds != nullptr && shadowColorTextureIds[i] >= 0) {
   textures["shadowcolor" + std::to_string(i)] = shadowColorTextureIds[i];
   colorImages["shadowcolor" + std::to_string(i)] = shadowColorTextureIds[i];
  }
 }
 std::unordered_map<std::string, int> volumeTextures;
 glutil::refreshTextureAliases(textures);
 ShaderPackResources::addTextures(pack, stage, textures, volumeTextures);
 FrameUniformSet frameUniforms = worldUniforms_;
 frameUniforms.viewWidth = static_cast<float>(width);
 frameUniforms.viewHeight = static_cast<float>(height);
 frameUniforms.aspectRatio = frameUniforms.viewWidth / std::max(frameUniforms.viewHeight, 1.0f);
 frameUniforms.farPlane = farPlane;
 frameUniforms.shadowMapResolution = static_cast<float>(shadowMapResolution);
 frameUniforms.shadowAvailable = shadowDepthTextureId >= 0 ? 1 : 0;
 frameUniforms.normalAvailable = targets.colorCount() > 1 ? 1 : 0;
 const bool computeReady = gl::GLCore::computeSupported;
 auto compileFn = [this](ShaderPackInstance& p, const std::string& name) {
  return ShaderPackCompiler::compile(p, name, [this](ShaderPackInstance& p2, const std::string& m) {
   logOnce(p2, m);
  });
 };
 if(computeReady &&
    !dispatchSetupIfNeeded(pack, frameUniforms, width, height, textures, colorImages, volumeTextures,
                           compileFn)) {
  glutil::releaseSamplers(glutil::maxTextureUnits());
  return false;
 }
 std::vector<bool> computeDispatched(pack.definition.passes.size(), false);
 bool computeOnlyStage = false;
 struct ComputeGroup {
  std::string parent;
  std::vector<std::size_t> indices;
 };
 std::vector<ComputeGroup> computeGroups;
 if(computeReady) {
  for(std::size_t passIndex : pack.computePasses) {
   const ShaderPass& compute = pack.definition.passes[passIndex];
   if(!ComputeDispatcher::matchesStage(compute.name, stage) &&
      !(stage == "composite" && ComputeDispatcher::matchesStage(compute.name, "final"))) {
    continue;
   }
   const std::string parent = ComputeDispatcher::computeParentName(compute.name);
   auto group = std::find_if(computeGroups.begin(), computeGroups.end(),
                             [&parent](const ComputeGroup& g) { return g.parent == parent; });
   if(group == computeGroups.end()) {
    computeGroups.push_back(ComputeGroup{parent, {passIndex}});
   } else {
    group->indices.push_back(passIndex);
   }
  }
  std::sort(computeGroups.begin(), computeGroups.end(), [](const ComputeGroup& a, const ComputeGroup& b) {
   return ComputeDispatcher::lessComputeParent(a.parent, b.parent);
  });
 }
 const auto hasRasterProgram = [&programChain, &pack](const std::string& parent) {
  return std::any_of(programChain.begin(), programChain.end(), [&pack, &parent](const auto& entry) {
   return pack.definition.passes[entry.first].name == parent;
  });
 };
 const auto dispatchComputeGroup = [&](ComputeGroup& group) {
  bool dispatched = false;
  std::sort(group.indices.begin(), group.indices.end(), [&pack](std::size_t a, std::size_t b) {
   return ComputeDispatcher::lessComputeOrder(pack.definition.passes[a], pack.definition.passes[b]);
  });
  refreshColorMaps(targets, textures, colorImages);
  glutil::refreshTextureAliases(textures);
  for(std::size_t passIndex : group.indices) {
   if(computeDispatched[passIndex]) {
    continue;
   }
   if(!ComputeDispatcher::dispatch(pack, pack.definition.passes[passIndex], frameUniforms, textures,
                                   colorImages, volumeTextures, width, height,
                                   !pack.definition.allowConcurrentCompute, compileFn)) {
    return false;
   }
   computeDispatched[passIndex] = true;
   dispatched = true;
  }
  if(dispatched && pack.definition.allowConcurrentCompute) {
   gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
  }
  if(dispatched) {
   refreshColorMaps(targets, textures, colorImages);
   glutil::refreshTextureAliases(textures);
   computeOnlyStage = true;
  }
  return true;
 };
 const auto dispatchOrphanComputesBefore = [&](const std::string* upperBound) {
  for(ComputeGroup& group : computeGroups) {
   if(hasRasterProgram(group.parent)) {
    continue;
   }
   if(upperBound != nullptr && !ComputeDispatcher::lessComputeParent(group.parent, *upperBound)) {
    break;
   }
   if(!dispatchComputeGroup(group)) {
    return false;
   }
  }
  return true;
 };
 if(computeReady && programChain.empty()) {
  if(!dispatchOrphanComputesBefore(nullptr)) {
   glutil::releaseSamplers(glutil::maxTextureUnits());
   return false;
  }
 }
 if(programChain.empty()) {
  if(present) {
   packWroteToScreen_ = false;
  }
  glutil::releaseSamplers(glutil::maxTextureUnits());
  core::activeTexture(gl::tex::Texture0);
  shadowDepthTexture_ = shadowDepthTextureId;
  shadowColorTextureCount_ = std::clamp(shadowColorTextureCount, 0, 8);
  for(int index = 0; index < shadowColorTextureCount_; ++index) {
   shadowColorTextures_[index] = shadowColorTextureIds == nullptr ? -1 : shadowColorTextureIds[index];
  }
  return present ? false : computeOnlyStage;
 }
 bool wroteToScreen = false;
 bool executed = false;
 for(const auto& [passIndex, program] : programChain) {
  const ShaderPass& pass = pack.definition.passes[passIndex];
  if(computeReady && !dispatchOrphanComputesBefore(&pass.name)) {
   glutil::releaseSamplers(glutil::maxTextureUnits());
   return false;
  }
  if(computeReady) {
   bool dispatchedAny = false;
   refreshColorMaps(targets, textures, colorImages);
   for(std::size_t computeIndex : pack.computePasses) {
    if(computeDispatched[computeIndex]) {
     continue;
    }
    if(!ComputeDispatcher::attachedToPass(pack.definition.passes[computeIndex].name, pass.name)) {
     continue;
    }
    if(!ComputeDispatcher::dispatch(pack, pack.definition.passes[computeIndex], frameUniforms, textures,
                                    colorImages, volumeTextures, width, height,
                                    !pack.definition.allowConcurrentCompute, compileFn)) {
     glutil::releaseSamplers(glutil::maxTextureUnits());
     return false;
    }
    computeDispatched[computeIndex] = true;
    dispatchedAny = true;
   }
   if(dispatchedAny && pack.definition.allowConcurrentCompute) {
    gl::GLCore::memoryBarrier(ComputeDispatcher::kBarrierBits);
   }
  }
  const std::string output = pass.outputs.empty() ? "screen" : pass.outputs.front();
  const bool toScreen =
      ShaderPackCatalog::lower(output) == "screen" || pass.name == "final" ||
      pass.program.rfind("final", 0) == 0;
  for(const std::string& buffer : pass.mipmapBuffers) {
   const auto tex = textures.find(buffer);
   if(tex == textures.end() || tex->second <= 0 || gl::GLCore::generateMipmap == nullptr) {
    continue;
   }
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
    render::ColorFormat format = render::ColorFormat::Rgba8;
    int tw = width;
    int th = height;
    if(declared != pack.definition.targets.end()) {
     format = glutil::parseFormat(declared->second.format);
     const ShaderTarget& tgt = declared->second;
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
      logOnce(pack, "pass '" + pass.name + "' could not allocate target '" + name + "'");
      return false;
     }
    }
    targets.prepareWrite(name);
   }
   refreshColorMaps(targets, textures, colorImages);
   if(!targets.bindWrite(outputs)) {
    logOnce(pack, "pass '" + pass.name + "' could not bind write targets");
    return false;
   }
  } else {
   gl::GLCore::bindFramebuffer(0x8D40, static_cast<unsigned int>(present ? 0 : destinationFramebuffer));
   core::viewport(0, 0, width, height);
  }
  program->bind();
  glutil::applyBufferBlends(pack.definition, pass.program);
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
  glutil::refreshTextureAliases(textures);
  ShaderPackResources::addTextures(pack, stage, textures, volumeTextures);
  glutil::bindSamplers(*program, textures, volumeTextures, glutil::maxTextureUnits());
  const unsigned int nextImageUnit = glutil::bindColorImages(*program, colorImages, &pack.definition);
  ShaderPackResources::bind(pack, *program, nextImageUnit);
  uploadShaderUniforms(*program, frameUniforms);
  uploadIdentityDrawMatrices(*program);
  pack.customUniforms.upload(*program);
  program->bind();
  core::drawFullscreen();
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
 if(computeReady && !dispatchOrphanComputesBefore(nullptr)) {
  glutil::releaseSamplers(glutil::maxTextureUnits());
  return false;
 }
 if(present) {
  packWroteToScreen_ = wroteToScreen;
 }
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
} // namespace net::minecraft::client::render::shaderpack
