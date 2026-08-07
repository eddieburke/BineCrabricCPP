#include "net/minecraft/client/render/pipeline/Manager.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/render/PbrTextures.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
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
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
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
namespace net::minecraft::client::render {
namespace {
using PackCatalog::directoryResources;
using PackCatalog::lower;
using PackCatalog::packDirectoryStamp;
using PackCatalog::zipResources;
std::string defaultSettingValue(const PackSetting& setting) {
 return setting.defaultValue;
}
} // namespace
PackManager::PackManager(std::filesystem::path gameDirectory, option::GameOptions* options,
                         gl::ShaderCompileService& compiler)
    : gameDirectory_(std::move(gameDirectory)), options_(options), compiler_(compiler), pipeline_(options) {
  reload();
  startDirectoryWatcher();
 render::setWorldProgramResolver([this](std::string_view key) { return worldProgram(key); });
 render::setWorldPassDirectiveApplier([this]() {
  PackInstance* pack = renderPack();
  if(pack == nullptr) {
   return;
  }
  // Applies blend/alphaTest directives by the resolved source name
  // (ProgramDirectives.java:80-83).
  const std::string programKey = pipeline_.resolvedWorldProgramKey();
  if(programKey.empty()) {
   return;
  }
  gl::ShaderProgram* program = core::program();
  if(program == nullptr) return;
  const bool shadowPass = core::cameraFrame().shadowPass;
  const int colorCount = shadowPass ? pack->definition.shadowColorBuffers : pack->colorTargets.colorCount();
  if(!shadowPass || colorCount > 0) program->applyDrawBuffers(std::max(1, colorCount));
  applyBufferBlends(pack->definition, programKey, program->drawBufferColortexIndices());
  applyAlphaTest(pack->definition, programKey);
 });
 render::setShaderObjectIdResolver([this](const std::string& kind, const std::string& name, int fallback) {  const PackDefinition& definition = activeDefinition();
  const auto& ids =
      kind == "entity" ? definition.entityIds : kind == "item" ? definition.itemIds
                                                               : definition.blockIds;
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
   bindWorldProgram(program, makeWorldBindContext());
  });
  core::setProgramMaterialBinder([this](gl::ShaderProgram& program) {
   bindProgramMaterialTextures(program, makeWorldBindContext());
  });
}
WorldProgramBindContext PackManager::makeWorldBindContext() {
 WorldProgramBindContext ctx{};
 ctx.uniforms = &pipeline_.worldUniforms();
 ctx.lightmapTexture = pipeline_.lightmapTexture();
 ctx.overlayTexture = core::entityOverlayTexture();
 const bool interfaceProgram = pipeline_.interfaceProgramsActive();
 PackInstance* pack = renderPack();
 ctx.noiseTexture = (!interfaceProgram && pack != nullptr) ? pack->noiseTexture.handle() : 0;
 ctx.shadowDepthTexture = interfaceProgram ? -1 : pipeline_.shadowDepthTexture();
 ctx.shadowOpaqueDepthTexture = interfaceProgram ? -1 : pipeline_.shadowOpaqueDepthTexture();
 ctx.shadowColorTextures = interfaceProgram ? nullptr : pipeline_.shadowColorTextures();
 ctx.shadowColorTextureCount = interfaceProgram ? 0 : pipeline_.shadowColorTextureCount();
 const bool shadowPass = core::cameraFrame().shadowPass;
 ctx.bindTextureAtlases = !interfaceProgram && !shadowPass &&
                          pipeline_.lastWorldProgramKey().rfind("gbuffers_", 0) == 0;
 ctx.normalTexture = pipeline_.normalFallbackTexture();
 ctx.specularTexture = pipeline_.specularFallbackTexture();
 if(ctx.bindTextureAtlases && net::minecraft::client::Minecraft::INSTANCE != nullptr) {
  auto& textureManager = net::minecraft::client::Minecraft::INSTANCE->textureManager;
  core::activeTexture(gl::tex::Texture0);
  const int diffuseTexture = core::boundTexture();
  // ColorWheel: the holder carries LabPBR-mipmapped companion textures
  // (IrisRenderingPipeline.java:848).
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/IrisRenderingPipeline.java
  const render::PbrTextures::Holder holder =
      render::PbrTextures::getOrLoad(diffuseTexture, textureManager, pack->definition.labPbr);
  if(holder.normal > 0) ctx.normalTexture = static_cast<unsigned int>(holder.normal);
  if(holder.specular > 0) ctx.specularTexture = static_cast<unsigned int>(holder.specular);
  if(!textureManager.getTextureDimensionsForId(diffuseTexture, ctx.atlasWidth, ctx.atlasHeight)) {
   // Java CommonUniforms.atlasSize (CommonUniforms.java:81-93) reports (0,0)
   // for any texture it has not uploaded itself.
   ctx.atlasWidth = 0;
   ctx.atlasHeight = 0;
  }
 }
 ctx.clearShadowBindsWhenNoPack = interfaceProgram || pack == nullptr;
 ctx.pack = interfaceProgram ? nullptr : pack;
 return ctx;
}
PackManager::~PackManager() {
 stopDirectoryWatcher();
 render::setWorldProgramResolver(nullptr);
 render::setWorldPassDirectiveApplier(nullptr);
  render::setShaderObjectIdResolver(nullptr);
  core::setProgramUniformUploader(nullptr);
  core::setProgramMaterialBinder(nullptr);
}
void PackManager::reloadWorldMeshes() {
 if(net::minecraft::client::Minecraft::INSTANCE == nullptr ||
    net::minecraft::client::Minecraft::INSTANCE->worldRenderer == nullptr) {
  return;
 }
 net::minecraft::client::Minecraft::INSTANCE->worldRenderer->reload();
}
void PackManager::reload() {
 discardStagedPack();
 packs_.clear();
 summaries_.clear();
 activeIndex_ = kNoActivePack;
 pendingIndex_.reset();
  const std::filesystem::path directory = gameDirectory_ / "shaders";
  std::error_code ec;
  std::filesystem::create_directories(directory, ec);
  const std::filesystem::path vanillaDirectory = directory / "vanilla";  basePack_ = std::filesystem::is_directory(vanillaDirectory, ec) ? loadPack(vanillaDirectory, true)
                                                                  : loadEmbeddedVanillaPack();
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
  packs_.push_back(loadPack(path, true));
 }
 for(const auto& path : archives) {
  packs_.push_back(loadPack(path, false));
 }
 const std::string selected = options_ != nullptr ? options_->shaderPack : std::string{};
 if(!selected.empty() && selected != "OFF") {
  for(std::size_t i = 0; i < packs_.size(); ++i) {
   if(packs_[i]->summary.key == selected || packs_[i]->summary.name == selected) {
    pendingIndex_ = i;
    break;
   }
  }
 }
 refreshSummaries();
 render::block::BlockRenderManager::setVoxelizeLightBlocks(false);
 pipeline_.applyBlockIds(activeDefinition());
 reloadWorldMeshes();
 packDirectoryStamp_ = packDirectoryStamp(directory);
 watchedStamp_.store(packDirectoryStamp_, std::memory_order_relaxed);
 directoryChanged_.store(false, std::memory_order_relaxed);
 pipeline_.reset();
 pipeline_.refreshResourcePackState(basePack_.get(), packs_);
 warmBasePrograms();
 prewarmPacks();
 preparePendingPack(net::minecraft::client::Minecraft::INSTANCE != nullptr
                        ? net::minecraft::client::Minecraft::INSTANCE->world
                        : nullptr);
}
void PackManager::startDirectoryWatcher() {
 stopDirectoryWatcher();
 directoryWatcher_ = std::jthread([this](const std::stop_token& stop) { directoryWatchLoop(stop); });
}
void PackManager::stopDirectoryWatcher() {
 if(directoryWatcher_.joinable()) {
  directoryWatcher_.request_stop();
  directoryWatcher_.join();
 }
}
void PackManager::directoryWatchLoop(const std::stop_token& stop) {
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
  const std::uint64_t stamp = packDirectoryStamp(gameDirectory_ / "shaders");
  const std::uint64_t previous = watchedStamp_.load(std::memory_order_relaxed);
  if(stamp != previous) {
   watchedStamp_.store(stamp, std::memory_order_relaxed);
   directoryChanged_.store(true, std::memory_order_release);
  }
 }
}
void PackManager::poll() {
 if(!directoryChanged_.exchange(false, std::memory_order_acq_rel)) {
  return;
 }
 packDirectoryStamp_ = watchedStamp_.load(std::memory_order_relaxed);
 reload();
}
std::unique_ptr<PackInstance> PackManager::loadEmbeddedVanillaPack() {
 auto pack = std::make_unique<PackInstance>();
 pack->shaderCompiler = &compiler_;
 pack->embedded = true;
 pack->summary.key = "vanilla";
 pack->summary.name = "vanilla";
 const std::vector<std::string> resources = VanillaPackEmbed::resources();
 if(PackLoader::load(
        resources,
        [&pack](std::string_view resource) { return PackCompiler::cachedText(*pack, std::string(resource)); },
        pack->definition, pack->sourceOptions, pack->summary.error)) {
  pack->summary.valid = true;
  pack->rootDefinition = pack->definition;
  initializePackRuntime(*pack);
 }
 return pack;
}
std::unique_ptr<PackInstance> PackManager::loadPack(const std::filesystem::path& path, bool directory) {
 auto pack = std::make_unique<PackInstance>();
 pack->shaderCompiler = &compiler_;
 pack->path = path;
 pack->directory = directory;
 pack->summary.key = path.filename().string();
 std::vector<std::string> resources;
 if(directory) {
  resources = directoryResources(path);
 } else {
  pack->zip = std::make_unique<resource::pack::ZippedTexturePack>(path);
  pack->zip->open();
  resources = zipResources(*pack->zip);
 }
 if(PackLoader::load(
        resources,
        [&pack](std::string_view resource) { return PackCompiler::cachedText(*pack, std::string(resource)); },
        pack->definition, pack->sourceOptions, pack->summary.error)) {
  pack->summary.valid = true;
  pack->rootDefinition = pack->definition;
  pack->summary.name = path.filename().string();
  initializePackRuntime(*pack);
 }
 if(pack->summary.name.empty()) {
  pack->summary.name = path.filename().string();
 }
 return pack;
}
void PackManager::initializePackRuntime(PackInstance& pack) {
 for(const PackSetting& setting : pack.definition.settings) {
  pack.settings.try_emplace(setting.key, defaultSettingValue(setting));
 }
 std::string customError;
 if(!pack.rebuildRuntime(customError)) logOnce(pack, customError);
}
void PackManager::refreshSummaries() {
 summaries_.clear();
 summaries_.reserve(packs_.size());
 const std::size_t selectedIndex = pendingIndex_.value_or(activeIndex_);
 for(std::size_t i = 0; i < packs_.size(); ++i) {
  PackSummary summary = packs_[i]->summary;
  summary.selected = i == selectedIndex;
  summaries_.push_back(std::move(summary));
 }
}
bool PackManager::select(const std::string& key) {
 discardStagedPack();
 if(key.empty() || lower(key) == "off" || lower(key) == "none") {
  cancelPendingPack();
  if(options_ != nullptr) {
   options_->shaderPack.clear();
   options_->save();
  }
  activatePack(kNoActivePack);
  return true;
 }
 for(std::size_t i = 0; i < packs_.size(); ++i) {
  if(packs_[i]->summary.key != key && packs_[i]->summary.name != key) {
   continue;
  }
  if(!packs_[i]->summary.valid) {
   return false;
  }
  cancelPendingPack();
  if(options_ != nullptr) {
   options_->shaderPack = packs_[i]->summary.key;
   options_->save();
  }
  if(i == activeIndex_) {
   refreshSummaries();
   return true;
  }
  pendingIndex_ = i;
  refreshSummaries();
  preparePendingPack(net::minecraft::client::Minecraft::INSTANCE != nullptr
                         ? net::minecraft::client::Minecraft::INSTANCE->world
                         : nullptr);
  return true;
 }
 return false;
}
void PackManager::warmBasePrograms() {
 if(basePack_ == nullptr || !basePack_->summary.valid || !hasGlContext()) {
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
 pipeline_.reset();
 pipeline_.updateLightmap(net::minecraft::client::Minecraft::INSTANCE != nullptr
                              ? net::minecraft::client::Minecraft::INSTANCE->world
                              : nullptr);
}
void PackManager::prewarmPacks() {
 const auto warm = [this](PackInstance* pack) {
  if(pack == nullptr || !pack->summary.valid || pack->programs == nullptr) {
   return;
  }
  PackCompiler::buildPrewarmQueue(*pack);
 };
 warm(basePack_.get());
 PackInstance* current = activePack();
 if(current != basePack_.get()) warm(current);
}
void PackManager::advancePackActivation() {
 PackInstance* current = activePack();
 if(pendingIndex_.has_value() && *pendingIndex_ < packs_.size()) {
  PackInstance* pending = packs_[*pendingIndex_].get();
  if(pending != current) {
   PackCompiler::buildPrewarmQueue(*pending);
   PackCompiler::prewarmStep(*pending, logFn());
   if(packReady(*pending)) {
    activatePack(*pendingIndex_);
   }
  }
 }
 if(stagedPack_ != nullptr) {
  PackCompiler::buildPrewarmQueue(*stagedPack_);
  PackCompiler::prewarmStep(*stagedPack_, logFn());
  if(packReady(*stagedPack_)) {
   commitStagedPack();
  }
 }
}
bool PackManager::setSetting(const std::string& key, std::string value) {
 return setSettings({{key, std::move(value)}});
}
bool PackManager::setSettings(const std::vector<std::pair<std::string, std::string>>& values) {
 if(values.empty()) {
  return false;
 }
 PackInstance* pack = selectedPack();
 if(pack == nullptr || pack->definition.settings.empty()) {
  return false;
 }
 std::unordered_map<std::string, std::string> merged = pack->settings;
 const bool hasProfileOption = std::any_of(pack->definition.settings.begin(), pack->definition.settings.end(),
                                           [](const PackSetting& setting) { return setting.key == "profile"; });
 std::string profileName;
 if(hasProfileOption) {
  for(const auto& [key, value] : values) {
   if(key == "profile") {
    profileName = value;
    break;
   }
  }
  const std::string currentProfile = [&] {
   const auto existing = merged.find("profile");
   return existing != merged.end() ? existing->second : std::string{};
  }();
  if(profileName.empty()) profileName = currentProfile;
  if(profileName != currentProfile) {
   merged.clear();
   for(const PackSetting& setting : pack->definition.settings) {
    merged[setting.key] = defaultSettingValue(setting);
   }
   if(!profileName.empty() && profileName != "Default") {
    for(const PackProfile& preset : pack->definition.profiles) {
     if(preset.name != profileName) continue;
     for(const auto& [key, value] : preset.values) {
      for(const PackSetting& setting : pack->definition.settings) {
       if(setting.key != key) continue;
       std::string normalized;
       if(!normalizeSettingValue(setting, value, normalized)) break;
       merged[key] = std::move(normalized);
       break;
      }
     }
     break;
    }
   }
  }
 }
 bool changed = false;
 for(const auto& [key, value] : values) {
  if(key == "profile") continue;
  for(const PackSetting& setting : pack->definition.settings) {
   if(setting.key != key) continue;
   std::string normalized;
   if(!normalizeSettingValue(setting, value, normalized)) break;
   if(const auto existing = merged.find(key); existing != merged.end() && existing->second == normalized) {
    break;
   }
   merged[key] = std::move(normalized);
   changed = true;
   break;
  }
 }
 if(hasProfileOption) {
  if(profileName.empty()) profileName = "Default";
  if(const auto existing = merged.find("profile"); existing == merged.end() || existing->second != profileName) {
   merged["profile"] = profileName;
   changed = true;
  }
 }
 if(!changed) {
  return false;
 }
 if(pendingIndex_.has_value()) {
  PackInstance* target = packs_[*pendingIndex_].get();
  if(target != nullptr) {
   target->settings = merged;
   std::string customError;
   if(!target->rebuildRuntime(customError)) logOnce(*target, customError);
   target->prewarmQueue.clear();
  }
   preparePendingPack(net::minecraft::client::Minecraft::INSTANCE != nullptr
                          ? net::minecraft::client::Minecraft::INSTANCE->world
                          : nullptr);
   return true;
  }  discardStagedPack();
  stagedPack_ = clonePack(*pack, &merged);
  stagedIndex_ = activeIndex_;
  pipeline_.selectDimension(
      *stagedPack_,
      net::minecraft::client::Minecraft::INSTANCE != nullptr ? net::minecraft::client::Minecraft::INSTANCE->world
                                                             : nullptr,
      false);
  if(stagedPack_->programState == PackProgramState::Cold) {
   PackCompiler::buildPrewarmQueue(*stagedPack_);
   PackCompiler::prewarmStep(*stagedPack_, logFn());
  }
  if(packReady(*stagedPack_)) commitStagedPack();
  return true;
}
std::string PackManager::settingValue(const std::string& key) const {
 const PackInstance* pack = selectedPack();
 if(pack == nullptr) {
  return {};
 }
 const auto found = pack->settings.find(key);
 return found == pack->settings.end() ? std::string() : found->second;
}
void PackManager::cancelPendingPack() {
 if(!pendingIndex_.has_value() || *pendingIndex_ >= packs_.size()) {
  pendingIndex_.reset();
  return;
 }
 PackInstance* pack = packs_[*pendingIndex_].get();
 if(pack != nullptr) {
  pack->programState = PackProgramState::Cold;
 }
 pendingIndex_.reset();
}
void PackManager::discardStagedPack() {
 stagedPack_.reset();
 stagedIndex_ = kNoActivePack;
}
void PackManager::activatePack(std::size_t index) {
 discardStagedPack();
 const bool changed = activeIndex_ != index;
 PackInstance* previous = activePack();
 activeIndex_ = index;
 pendingIndex_.reset();
 refreshSummaries();
 if(!changed) return;
 pipeline_.reset();
 if(previous != nullptr && previous != activePack()) previous->clearGpuResources();
 const PackDefinition& definition = activeDefinition();
 render::block::BlockRenderManager::setVoxelizeLightBlocks(
     activeIndex_ < packs_.size() && definition.voxelizeLightBlocks);
 pipeline_.applyBlockIds(definition);
 reloadWorldMeshes();
}
void PackManager::preparePendingPack(net::minecraft::World* world) {
 if(!pendingIndex_.has_value() || *pendingIndex_ >= packs_.size()) return;
 PackInstance* pack = packs_[*pendingIndex_].get();
 if(pack == nullptr || !pack->summary.valid || pack->programs == nullptr) return;
 if(world == nullptr && !pack->rootDefinition.dimensionDefinitions.empty()) return;
 pipeline_.selectDimension(*pack, world, false);
 if(pack->programState == PackProgramState::Cold) {
  PackCompiler::buildPrewarmQueue(*pack);
  PackCompiler::prewarmStep(*pack, logFn());
 }
 if(packReady(*pack)) {
  activatePack(*pendingIndex_);
 }
}
bool PackManager::packReady(PackInstance& pack) {
 if(pack.programState == PackProgramState::Cold || pack.programs == nullptr) {
  return false;
 }
 if(!PackCompiler::validate(pack, [this](PackInstance& p, const std::string& message,
                                         ::net::minecraft::util::logging::LogLevel) {
     logOnce(p, message);
    })) return false;
 const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 if(minecraft == nullptr || !hasGlContext()) return false;
 return pipeline_.preparePackResources(pack, std::max(1, minecraft->displayWidth),
                                        std::max(1, minecraft->displayHeight));
}
std::unique_ptr<PackInstance> PackManager::clonePack(const PackInstance& source,
                                                     const std::unordered_map<std::string, std::string>* settings) {
   auto pack = std::make_unique<PackInstance>();
   pack->shaderCompiler = &compiler_;
   pack->summary = source.summary;
  pack->path = source.path;
  pack->directory = source.directory;
  pack->embedded = source.embedded;
  if(!source.directory && !source.embedded) {
   pack->zip = std::make_unique<resource::pack::ZippedTexturePack>(source.path);
   pack->zip->open();
  }
 pack->rootDefinition = source.rootDefinition;
 pack->definition = pack->rootDefinition;
 pack->sourceOptions = source.sourceOptions;
 pack->settings = settings != nullptr ? *settings : source.settings;
 std::vector<std::string> resources;
 if(source.embedded) {
  resources = VanillaPackEmbed::resources();
 } else if(source.directory) {
  resources = directoryResources(source.path);
 } else {
  resources = zipResources(*pack->zip);
 }
 PackDefinition reParsed;
 std::unordered_map<std::string, PackSourceOption> reOptions;
 std::string error;
 if(PackLoader::load(resources, [&pack](std::string_view resource) { return PackCompiler::cachedText(*pack, std::string(resource)); }, reParsed, reOptions, error, pack->settings)) {
  pack->definition = std::move(reParsed);
  pack->rootDefinition = pack->definition;
  pack->sourceOptions = std::move(reOptions);
 }
 initializePackRuntime(*pack);
 return pack;
}
void PackManager::prepareStagedPack(net::minecraft::World* world) {
 if(pendingIndex_.has_value()) {
  discardStagedPack();
  return;
 }
 PackInstance* current = activePack();
 if(current == nullptr) {
  discardStagedPack();
  return;
 }
 const std::string desired = pipeline_.dimensionKey(*current, world);
 if(stagedPack_ != nullptr) {
  const bool stale = stagedIndex_ != activeIndex_ ||
                     stagedPack_->rootDefinition.labPbr != current->rootDefinition.labPbr ||
                     stagedPack_->rootDefinition.labPbr13 != current->rootDefinition.labPbr13 ||
                     stagedPack_->dimensionKey != desired;
  if(stale) discardStagedPack();
 }
 if(stagedPack_ == nullptr) {
  if(desired == current->dimensionKey) return;
  discardStagedPack();
  stagedPack_ = clonePack(*current);
  stagedIndex_ = activeIndex_;
  pipeline_.selectDimension(*stagedPack_, world, false);
 }
 if(stagedPack_->programState == PackProgramState::Cold) {
  PackCompiler::buildPrewarmQueue(*stagedPack_);
  PackCompiler::prewarmStep(*stagedPack_, logFn());
 }
 if(packReady(*stagedPack_)) commitStagedPack();
}
void PackManager::commitStagedPack() {
 if(stagedPack_ == nullptr || stagedIndex_ != activeIndex_) {
  discardStagedPack();
  return;
 }
 if(activeIndex_ < packs_.size()) {
  packs_[activeIndex_] = std::move(stagedPack_);
 } else {
  basePack_ = std::move(stagedPack_);
 }
 stagedIndex_ = kNoActivePack;
 refreshSummaries();
 pipeline_.reset();
 const PackDefinition& definition = activeDefinition();
 render::block::BlockRenderManager::setVoxelizeLightBlocks(
     activeIndex_ < packs_.size() && definition.voxelizeLightBlocks);
 pipeline_.applyBlockIds(definition);
 reloadWorldMeshes();
}
PackInstance* PackManager::activePack() noexcept {
 if(activeIndex_ < packs_.size()) {
  return packs_[activeIndex_].get();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_.get() : nullptr;
}
const PackInstance* PackManager::activePack() const noexcept {
 if(activeIndex_ < packs_.size()) {
  return packs_[activeIndex_].get();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_.get() : nullptr;
}
PackInstance* PackManager::renderPack() noexcept {
 PackInstance* pack = pipeline_.interfaceProgramsActive() ? basePack_.get() : activePack();
 return pack != nullptr ? pack : basePack_.get();
}
PackInstance* PackManager::selectedPack() noexcept {
 if(pendingIndex_.has_value() && *pendingIndex_ < packs_.size()) {
  return packs_[*pendingIndex_].get();
 }
 if(stagedPack_ != nullptr && stagedIndex_ == activeIndex_) return stagedPack_.get();
 return activePack();
}
const PackInstance* PackManager::selectedPack() const noexcept {
 if(pendingIndex_.has_value() && *pendingIndex_ < packs_.size()) {
  return packs_[*pendingIndex_].get();
 }
 if(stagedPack_ != nullptr && stagedIndex_ == activeIndex_) return stagedPack_.get();
 return activePack();
}
bool PackManager::hasActivePack() const noexcept {
 const PackInstance* pack = activePack();
 return pack != nullptr && pack->summary.valid;
}
const PackDefinition& PackManager::activeDefinition() const noexcept {
 const PackInstance* pack = activePack();
 return pack != nullptr && pack->summary.valid ? pack->definition : vanillaPackDefinition();
}
const PackDefinition* PackManager::selectedDefinition() const noexcept {
 const PackInstance* pack = selectedPack();
 return pack != nullptr && pack->summary.valid ? &pack->definition : nullptr;
}
const PackDefinition& PackManager::meshDefinition() const noexcept {
 if(hasActivePack()) {
  return activeDefinition();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_->definition : vanillaPackDefinition();
}
bool PackManager::hasDeferredPasses() const {
 return pipeline_.hasDeferredPasses(activePack());
}
gl::ShaderProgram* PackManager::worldProgram(std::string_view key) {
 return pipeline_.worldProgram(key, renderPack());
}
void PackManager::prepareFrame(net::minecraft::World* world) {
 pipeline_.refreshResourcePackState(basePack_.get(), packs_);
 preparePendingPack(world);
 prepareStagedPack(world);
 pipeline_.prepareFrame(world, activePack(), basePack_.get());
}
void PackManager::setFrameUniforms(const PackUniformValues& frame) {
 pipeline_.setFrameUniforms(frame, activeDefinition(), activePack());
}
bool PackManager::renderBegin(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                              const int* shadowColorTextureIds, int shadowColorTextureCount,
                              shadowmap::ShadowTargets* shadowTargets,
                              const int* shadowColorAltTextureIds) {
 return pipeline_.renderBegin(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                              shadowColorTextureIds, shadowColorTextureCount, shadowTargets,
                              shadowColorAltTextureIds);
}
bool PackManager::renderShadowComposite(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                        const int* shadowColorTextureIds, int shadowColorTextureCount,
                                        shadowmap::ShadowTargets* shadowTargets,
                                        const int* shadowColorAltTextureIds) {
 return pipeline_.renderShadowComposite(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                        shadowColorTextureIds, shadowColorTextureCount, shadowTargets,
                                        shadowColorAltTextureIds);
}
bool PackManager::renderPreWorld(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                 const int* shadowColorTextureIds, int shadowColorTextureCount,
                                 shadowmap::ShadowTargets* shadowTargets,
                                 const int* shadowColorAltTextureIds) {
 return pipeline_.renderPreWorld(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                 shadowColorTextureIds, shadowColorTextureCount, shadowTargets,
                                 shadowColorAltTextureIds);
}
bool PackManager::renderDeferred(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                 const int* shadowColorTextureIds, int shadowColorTextureCount,
                                 const int* shadowColorAltTextureIds) {
 return pipeline_.renderDeferred(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                                 shadowColorTextureIds, shadowColorTextureCount, shadowColorAltTextureIds);
}
bool PackManager::renderPostProcess(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                    const int* shadowColorTextureIds, int shadowColorTextureCount,
                                    const int* shadowColorAltTextureIds) {
 return pipeline_.renderPostProcess(activePack(), basePack_.get(), shadowDepthTextureId,
                                    shadowOpaqueDepthTextureId, shadowColorTextureIds,
                                    shadowColorTextureCount, shadowColorAltTextureIds);
}
void PackManager::sampleCenterDepth() {
 pipeline_.sampleCenterDepth(activePack(), activeDefinition());
}
void PackManager::captureOpaqueDepth() {
 pipeline_.captureOpaqueDepth(activePack());
}
void PackManager::captureHandDepth() {
 pipeline_.captureHandDepth(activePack());
}
PackCompiler::LogFnLevel PackManager::logFn() {
 return [this](PackInstance& p, const std::string& message, ::net::minecraft::util::logging::LogLevel) {
  logOnce(p, message);
 };
}
void PackManager::logOnce(PackInstance& pack, const std::string& message) const {
 if(!pack.logged.insert(message).second) {
  return;
 }
 const std::string& label =
     !pack.summary.name.empty()      ? pack.summary.name
     : !pack.definition.name.empty() ? pack.definition.name
                                     : std::string("Shader pack");
 ClientLog::LOGGER.log(net::minecraft::util::logging::LogLevel::Warning,
                       "[shaderpack:" + label + "] " + message);
}
bool PackManager::ensureSceneTargets(int width, int height) {
 return pipeline_.ensureSceneTargets(activePack(), width, height);
}
void PackManager::bindScene() {
 pipeline_.bindScene(activePack());
}
void PackManager::endScene() {
 pipeline_.endScene(activePack());
}
int PackManager::sceneColorCount() const {
 return pipeline_.sceneColorCount(activePack());
}
void PackManager::clearScene(float fogR, float fogG, float fogB) {
 pipeline_.clearScene(activePack(), fogR, fogG, fogB);
}
} // namespace net::minecraft::client::render
