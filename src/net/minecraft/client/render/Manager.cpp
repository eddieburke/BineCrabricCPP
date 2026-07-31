#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCatalog.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCompiler.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackLoader.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPassScheduler.hpp"
#include "net/minecraft/client/render/shaderpack/WorldProgramBinder.hpp"
#include "net/minecraft/client/render/PassIndex.hpp"
#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/world/World.hpp"
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
    : gameDirectory_(std::move(gameDirectory)), options_(options), pipeline_(options) {
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
   programKey = render::resolveIrisShadowProgramKey(key, pack->definition.programs);
   if(programKey.empty() || !pack->definition.programs.contains(programKey)) {
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
  ctx.uniforms = &pipeline_.worldUniforms();
  ctx.lightmapTexture = pipeline_.lightmapTexturePtr();
  const bool interfaceProgram = interfaceProgramsActive();
  ShaderPackInstance* pack = interfaceProgram ? basePack_.get() : activePack();
  if(pack == nullptr) {
   pack = basePack_.get();
  }
  ctx.noiseTexture = (!interfaceProgram && pack != nullptr) ? pack->noiseTexture : 0;
  ctx.shadowDepthTexture = interfaceProgram ? -1 : pipeline_.shadowDepthTexture();
  ctx.shadowOpaqueDepthTexture = interfaceProgram ? -1 : pipeline_.shadowOpaqueDepthTexture();
  ctx.shadowColorTextures = interfaceProgram ? nullptr : pipeline_.shadowColorTextures();
  ctx.shadowColorTextureCount = interfaceProgram ? 0 : pipeline_.shadowColorTextureCount();
  const bool shadowPass = RenderCameraState::instance().frame().shadowPass;
  ctx.bindTextureAtlases = !interfaceProgram && !shadowPass &&
                          pipeline_.lastWorldProgramKey().rfind("gbuffers_", 0) == 0;
  ctx.normalTexture = pipeline_.normalFallbackTexture();
  ctx.specularTexture = pipeline_.specularFallbackTexture();
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
 pipeline_.applyBlockIds(activeDefinition());
 reloadWorldMeshes();
 packDirectoryStamp_ = packDirectoryStamp(directory);
 watchedStamp_.store(packDirectoryStamp_, std::memory_order_relaxed);
 directoryChanged_.store(false, std::memory_order_relaxed);
 resetPresentState();
 pipeline_.refreshResourcePackState(basePack_.get(), packs_);
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
  pack->applyPassBuckets(std::move(buckets));
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
  pack->applyPassBuckets(std::move(buckets));
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
  pipeline_.applyBlockIds(basePack_ != nullptr && basePack_->summary.valid ? &basePack_->definition : nullptr);
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
  render::block::BlockRenderManager::setVoxelizeLightBlocks(packs_[i]->definition.voxelizeLightBlocks);
  pipeline_.applyBlockIds(&packs_[i]->definition);
  reloadWorldMeshes();
  resetPresentState();
  warmBasePrograms();
  return true;
 }
 return false;
}

void ShaderPackManager::resetPresentState() {
 pipeline_.reset();
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
  pipeline_.programFromPack(*basePack_, key);
 }
 resetPresentState();
 pipeline_.updateLightmap(net::minecraft::client::Minecraft::INSTANCE != nullptr
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
    pack->programEnabledCache.clear();
   }
   ShaderPassBuckets buckets;
   indexShaderPasses(pack->definition, pack->settings, buckets);
   pack->applyPassBuckets(std::move(buckets));
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
 return pipeline_.activeHasPostProcess(activeDefinition(), activePack());
}

bool ShaderPackManager::hasDeferredPasses() const {
 return pipeline_.hasDeferredPasses(activePack());
}

int ShaderPackManager::shadowMapResolution() const {
 const ShaderPackDefinition* definition = activeDefinition();
 return definition == nullptr || !definition->shadowEnabled ? 0 : definition->shadowMapResolution;
}

int ShaderPackManager::shadowColorBuffers() const {
 const ShaderPackDefinition* definition = activeDefinition();
 return definition == nullptr ? 0 : definition->shadowColorBuffers;
}

gl::ShaderProgram* ShaderPackManager::worldProgram(const std::string& key) {
 return pipeline_.worldProgram(key, activePack(), basePack_.get());
}

void ShaderPackManager::prepareFrame(net::minecraft::World* world) {
 pipeline_.prepareFrame(world, activePack(), basePack_.get(), packs_);
}

void ShaderPackManager::refreshLightmap(net::minecraft::World* world) {
 pipeline_.refreshLightmap(world);
}

void ShaderPackManager::setFrameUniforms(const FrameUniformSet& frame) {
 pipeline_.setFrameUniforms(frame, activeDefinition(), activePack());
}

bool ShaderPackManager::renderBegin() {
 return pipeline_.renderBegin(activePack());
}

bool ShaderPackManager::renderPreWorld(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                       const int* shadowColorTextureIds, int shadowColorTextureCount) {
 return pipeline_.renderPreWorld(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                 shadowColorTextureIds, shadowColorTextureCount);
}

bool ShaderPackManager::renderDeferred(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                       const int* shadowColorTextureIds, int shadowColorTextureCount) {
 return pipeline_.renderDeferred(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                 shadowColorTextureIds, shadowColorTextureCount);
}

bool ShaderPackManager::renderPostProcess(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                          const int* shadowColorTextureIds, int shadowColorTextureCount) {
 return pipeline_.renderPostProcess(activePack(), basePack_.get(), shadowDepthTextureId,
                                    shadowOpaqueDepthTextureId, shadowColorTextureIds,
                                    shadowColorTextureCount);
}

void ShaderPackManager::sampleCenterDepth() {
 pipeline_.sampleCenterDepth(activePack(), activeDefinition());
}

void ShaderPackManager::captureOpaqueDepth() {
 pipeline_.captureOpaqueDepth(activePack());
}

void ShaderPackManager::captureHandDepth() {
 pipeline_.captureHandDepth(activePack());
}

void ShaderPackManager::logOnce(ShaderPackInstance& pack, const std::string& message) const {
 if(!pack.logged.insert(message).second) {
  return;
 }
 const std::string& label =
     !pack.summary.name.empty() ? pack.summary.name
     : !pack.definition.name.empty() ? pack.definition.name
                                     : std::string("Shader pack");
 ClientLog::LOGGER.log(net::minecraft::util::logging::LogLevel::Warning,
                       "[shaderpack:" + label + "] " + message);
}

std::vector<render::ColorFormat> ShaderPackManager::sceneColorFormats() const {
 return pipeline_.sceneColorFormats(activePack());
}

bool ShaderPackManager::ensureSceneTargets(int width, int height) {
 return pipeline_.ensureSceneTargets(activePack(), width, height);
}

void ShaderPackManager::bindScene() {
 pipeline_.bindScene(activePack());
}

void ShaderPackManager::endScene() {
 pipeline_.endScene(activePack());
}

void ShaderPackManager::destroyScene() {
 pipeline_.destroyScene(activePack());
}

int ShaderPackManager::sceneColorCount() const {
 return pipeline_.sceneColorCount(activePack());
}

unsigned int ShaderPackManager::sceneDepthTexture() const {
 return pipeline_.sceneDepthTexture(activePack());
}

void ShaderPackManager::clearScene(float fogR, float fogG, float fogB) {
 pipeline_.clearScene(activePack(), fogR, fogG, fogB);
}
} // namespace net::minecraft::client::render::shaderpack
