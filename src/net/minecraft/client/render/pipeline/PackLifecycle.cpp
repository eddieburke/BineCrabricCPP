#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/render/PbrTextures.hpp"
#include "net/minecraft/client/render/shaders/WorldProgramBinder.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
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
std::filesystem::path settingsFilePath(const PackInstance& pack) {
 return pack.path.parent_path() / (pack.summary.key + ".txt");
}
std::unordered_map<std::string, std::string> readSettingsFile(const PackInstance& pack) {
 std::unordered_map<std::string, std::string> result;
 if(pack.embedded) return result;
 const std::filesystem::path path = settingsFilePath(pack);
 std::ifstream in(path);
 if(!in) return result;
 std::string line;
 while(std::getline(in, line)) {
  if(line.empty() || line[0] == '#') continue;
  const auto eq = line.find('=');
  if(eq == std::string::npos) continue;
  const std::string key = line.substr(0, eq);
  const std::string val = line.substr(eq + 1);
  bool valid = false;
  for(const PackSetting& setting : pack.definition.settings) {
   if(setting.key != key) continue;
   std::string normalized;
   if(normalizeSettingValue(setting, val, normalized)) {
    result[key] = std::move(normalized);
   }
   valid = true;
   break;
  }
  if(!valid) continue;
 }
 return result;
}
void writeSettingsFile(const PackInstance& pack) {
 if(pack.embedded) return;
 const std::filesystem::path path = settingsFilePath(pack);
 bool anyChanged = false;
 for(const PackSetting& setting : pack.definition.settings) {
  const auto it = pack.settings.find(setting.key);
  if(it != pack.settings.end() && it->second != defaultSettingValue(setting)) {
   anyChanged = true;
   break;
  }
 }
 if(!anyChanged) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
  return;
 }
 std::ofstream out(path, std::ios::trunc);
 if(!out) return;
 for(const PackSetting& setting : pack.definition.settings) {
  const auto it = pack.settings.find(setting.key);
  if(it == pack.settings.end()) continue;
  if(it->second == defaultSettingValue(setting)) continue;
  out << it->first << "=" << it->second << "\n";
 }
}
} // namespace
Pipeline::Pipeline(std::filesystem::path gameDirectory, option::GameOptions* options,
                   std::filesystem::path shaderCacheDirectory)
    : options_(options), gameDirectory_(std::move(gameDirectory)),
      shaderCacheDirectory_(std::move(shaderCacheDirectory)),
      shaderBinaryCache_(std::make_shared<gl::ShaderBinaryCache>(shaderCacheDirectory_)) {
 reload();
 startDirectoryWatcher();
}
WorldProgramBindContext Pipeline::makeWorldBindContext(WorldProgramId id) {
 WorldProgramBindContext ctx{};
 ctx.uniforms = &worldUniforms();
 ctx.lightmapTexture = lightmapTexture();
 ctx.overlayTexture = core::entityOverlayTexture();
 const bool interfaceProgram = interfaceProgramsActive();
 PackInstance* pack = renderPack();
 ctx.shadowDepthTexture = interfaceProgram ? -1 : shadowDepthTexture();
 ctx.shadowOpaqueDepthTexture = interfaceProgram ? -1 : shadowOpaqueDepthTexture();
 ctx.shadowColorTextures = interfaceProgram ? nullptr : shadowColorTextures();
 ctx.shadowColorTextureCount = interfaceProgram ? 0 : shadowColorTextureCount();
 const bool shadowPass = core::cameraFrame().shadowPass;
 if(!interfaceProgram && !shadowPass && pack != nullptr && pack->colorTargets.valid()) {
  ctx.sceneTargets = &pack->colorTargets;
  ctx.sceneDepthTexture = static_cast<int>(pack->colorTargets.depthTexture());
  ctx.opaqueDepthTexture = pack->opaqueDepthTexture(ctx.sceneDepthTexture);
  ctx.handDepthTexture = pack->handDepthTexture(ctx.sceneDepthTexture);
 }
 ctx.bindTextureAtlases = !interfaceProgram && !shadowPass && pack != nullptr &&
                          bindsTextureAtlases(id);
 ctx.normalTexture = normalFallbackTexture();
 ctx.specularTexture = specularFallbackTexture();
 if(ctx.bindTextureAtlases && net::minecraft::client::Minecraft::INSTANCE != nullptr) {
  auto& textureManager = net::minecraft::client::Minecraft::INSTANCE->textureManager;
  core::activeTexture(gl::tex::Texture0);
  const int diffuseTexture = core::boundTexture();
  const render::PbrTextures::Holder holder =
      render::PbrTextures::getOrLoad(diffuseTexture, textureManager, pack->definition.labPbr);
  if(holder.normal > 0) ctx.normalTexture = static_cast<unsigned int>(holder.normal);
  if(holder.specular > 0) ctx.specularTexture = static_cast<unsigned int>(holder.specular);
  if(!textureManager.getTextureDimensionsForId(diffuseTexture, ctx.atlasWidth, ctx.atlasHeight)) {
   ctx.atlasWidth = 0;
   ctx.atlasHeight = 0;
  }
 }
 ctx.clearShadowBindsWhenNoPack = interfaceProgram || pack == nullptr;
 ctx.pack = interfaceProgram ? nullptr : pack;
 return ctx;
}
void Pipeline::reloadWorldMeshes() {
 if(net::minecraft::client::Minecraft::INSTANCE == nullptr ||
    net::minecraft::client::Minecraft::INSTANCE->worldRenderer == nullptr) {
  return;
 }
 net::minecraft::client::Minecraft::INSTANCE->worldRenderer->reload();
}
void Pipeline::reload() {
 packs_.clear();
 summaries_.clear();
 activeIndex_ = kNoActivePack;
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
  auto pack = std::make_unique<PackInstance>();
  pack->path = path;
  pack->directory = true;
  pack->summary.key = path.filename().string();
  pack->summary.name = pack->summary.key;
  pack->summary.valid = true;
  packs_.push_back(std::move(pack));
 }
 for(const auto& path : archives) {
  auto pack = std::make_unique<PackInstance>();
  pack->path = path;
  pack->summary.key = path.filename().string();
  pack->summary.name = pack->summary.key;
  pack->summary.valid = true;
  packs_.push_back(std::move(pack));
 }
  const std::string selected = options_ != nullptr ? options_->shaderPack : std::string{};
  std::size_t foundIndex = kNoActivePack;
  if(!selected.empty() && selected != "OFF") {
   for(std::size_t i = 0; i < packs_.size(); ++i) {
    if(packs_[i]->summary.key == selected || packs_[i]->summary.name == selected) {
     foundIndex = i;
     break;
    }
   }
  }
  refreshSummaries();
  render::block::BlockRenderManager::setVoxelizeLightBlocks(false);
  applyBlockIds(activeDefinition());
  reloadWorldMeshes();
#ifndef _WIN32
  watchedStamp_.store(packDirectoryStamp(directory), std::memory_order_relaxed);
#endif
  directoryChanged_.store(false, std::memory_order_relaxed);
  reset();
  refreshResourcePackState(basePack_.get(), packs_);
  activatePack(foundIndex);
}
void Pipeline::startDirectoryWatcher() {
 stopDirectoryWatcher();
 directoryWatcher_ = std::jthread([this](const std::stop_token& stop) { directoryWatchLoop(stop); });
}
void Pipeline::stopDirectoryWatcher() {
 if(directoryWatcher_.joinable()) {
  directoryWatcher_.request_stop();
  directoryWatcher_.join();
 }
}
void Pipeline::directoryWatchLoop(const std::stop_token& stop) {
#ifdef _WIN32
 SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
 const std::wstring path = (gameDirectory_ / "shaders").wstring();
 const HANDLE directory = CreateFileW(path.c_str(), FILE_LIST_DIRECTORY,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
 if(directory != INVALID_HANDLE_VALUE) {
  const HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if(event != nullptr) {
   alignas(DWORD) std::array<unsigned char, 65536> buffer{};
   OVERLAPPED overlapped{};
   overlapped.hEvent = event;
   bool failed = false;
   while(!stop.stop_requested()) {
    ResetEvent(event);
    DWORD ignored = 0;
    if(!ReadDirectoryChangesW(directory, buffer.data(), static_cast<DWORD>(buffer.size()), TRUE,
                              FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                  FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
                              &ignored, &overlapped, nullptr)) {
     failed = true;
     break;
    }
    bool completed = false;
    while(!stop.stop_requested()) {
     const DWORD result = WaitForSingleObject(event, 100);
     if(result == WAIT_TIMEOUT) {
      continue;
     }
     completed = true;
     if(result != WAIT_OBJECT_0) {
      failed = true;
     }
     break;
    }
    if(!completed) {
     CancelIoEx(directory, &overlapped);
     WaitForSingleObject(event, INFINITE);
     break;
    }
    if(failed) {
     CancelIoEx(directory, &overlapped);
     WaitForSingleObject(event, INFINITE);
     break;
    }
    DWORD bytes = 0;
    if(!GetOverlappedResult(directory, &overlapped, &bytes, FALSE)) {
     failed = true;
     break;
    }
    bool packContentChanged = bytes == 0;
    for(DWORD offset = 0; !packContentChanged && offset < bytes;) {
     const auto* record = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer.data() + offset);
     const std::wstring_view name(record->FileName, record->FileNameLength / sizeof(WCHAR));
     if(name.find(L'\\') != std::wstring_view::npos || !name.ends_with(L".txt")) {
      packContentChanged = true;
     }
     if(record->NextEntryOffset == 0) {
      break;
     }
     offset += record->NextEntryOffset;
    }
    if(packContentChanged) {
     directoryChanged_.store(true, std::memory_order_release);
    }
   }
   CloseHandle(event);
   CloseHandle(directory);
   if(!failed || stop.stop_requested()) {
    return;
   }
  } else {
   CloseHandle(directory);
  }
 }
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
void Pipeline::poll() {
 if(directoryChanged_.exchange(false, std::memory_order_acq_rel)) {
  reload();
 }
 net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 if(minecraft == nullptr || minecraft->world != nullptr) return;
 if(basePack_ != nullptr && basePack_->summary.valid && !basePack_->executionPlanReady) {
  compileExecutionPlan(*basePack_, 1);
 }
 PackInstance* pack = activePack();
 if(pack == nullptr || pack == basePack_.get() || !pack->summary.valid || pack->executionPlanReady) return;
 compileExecutionPlan(*pack, 1);
}
std::unique_ptr<PackInstance> Pipeline::loadEmbeddedVanillaPack() {
 auto pack = std::make_unique<PackInstance>();
 pack->shaderBinaryCache = shaderBinaryCache_;
 pack->embedded = true;
 pack->summary.key = "vanilla";
 pack->summary.name = "vanilla";
 pack->resources = VanillaPackEmbed::resources();
 if(PackLoader::load(
        pack->resources,
        [&pack](std::string_view resource) { return PackCompiler::cachedText(*pack, std::string(resource)); },
        pack->definition, pack->sourceOptions, pack->summary.error)) {
  pack->summary.valid = true;
  pack->rootDefinition = pack->definition;
  initializePackRuntime(*pack);
 }
 return pack;
}
std::unique_ptr<PackInstance> Pipeline::loadPack(const std::filesystem::path& path, bool directory) {
 auto pack = std::make_unique<PackInstance>();
 pack->shaderBinaryCache = shaderBinaryCache_;
 pack->path = path;
 pack->directory = directory;
 pack->summary.key = path.filename().string();
 if(directory) {
  pack->resources = directoryResources(path);
 } else {
  pack->zip = std::make_unique<resource::pack::ZippedTexturePack>(path);
  pack->zip->open();
  pack->resources = zipResources(*pack->zip);
 }
 if(PackLoader::load(
        pack->resources,
        [&pack](std::string_view resource) { return PackCompiler::cachedText(*pack, std::string(resource)); },
        pack->definition, pack->sourceOptions, pack->summary.error)) {
  pack->summary.valid = true;
  pack->rootDefinition = pack->definition;
  pack->summary.name = path.filename().string();
  initializePackRuntime(*pack);
  const auto savedSettings = readSettingsFile(*pack);
  if(!savedSettings.empty()) {
   for(const auto& [key, value] : savedSettings) {
    pack->settings[key] = value;
   }
   if(PackLoader::load(
          pack->resources,
          [&pack](std::string_view resource) { return PackCompiler::cachedText(*pack, std::string(resource)); },
          pack->definition, pack->sourceOptions, pack->summary.error, savedSettings)) {
    pack->rootDefinition = pack->definition;
   }
  }
 }
 if(pack->summary.name.empty()) {
  pack->summary.name = path.filename().string();
 }
 return pack;
}
void Pipeline::initializePackRuntime(PackInstance& pack) {
 for(const PackSetting& setting : pack.definition.settings) {
  pack.settings.try_emplace(setting.key, defaultSettingValue(setting));
 }
 std::string customError;
 if(!pack.rebuildRuntime(customError)) logOnce(pack, customError);
}
void Pipeline::refreshSummaries() {
 summaries_.clear();
 summaries_.reserve(packs_.size());
 for(std::size_t i = 0; i < packs_.size(); ++i) {
  PackSummary summary = packs_[i]->summary;
  summary.selected = i == activeIndex_;
  summaries_.push_back(std::move(summary));
 }
}
bool Pipeline::select(const std::string& key) {
 if(key.empty() || lower(key) == "off" || lower(key) == "none") {
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
  activatePack(i);
  if(activeIndex_ != i) {
   return false;
  }
  if(options_ != nullptr) {
   options_->shaderPack = packs_[i]->summary.key;
   options_->save();
  }
  return true;
 }
 return false;
}
bool Pipeline::setSetting(const std::string& key, std::string value) {
 return setSettings({{key, std::move(value)}});
}
bool Pipeline::setSettings(const std::vector<std::pair<std::string, std::string>>& values) {
 if(values.empty()) {
  return false;
 }
 PackInstance* pack = selectedPack();
 if(pack == nullptr || pack->definition.settings.empty()) {
  return false;
 }
 // see third_party/iris/common/src/main/java/net/irisshaders/iris/Iris.java:520
 std::unordered_map<std::string, std::string> merged = pack->settings;
 bool changed = false;
 for(const auto& [key, value] : values) {
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
 if(!changed) {
  return false;
 }
 PackDefinition definition;
 std::unordered_map<std::string, PackSourceOption> sourceOptions;
 std::string error;
 if(!PackLoader::load(
        pack->resources,
        [pack](std::string_view resource) { return PackCompiler::cachedText(*pack, std::string(resource)); },
        definition,
        sourceOptions,
        error,
        merged)) {
  logOnce(*pack, error, LogLevel::Severe);
  return false;
 }
 pack->settings = std::move(merged);
 writeSettingsFile(*pack);
 pack->sourceOptions = std::move(sourceOptions);
 pack->rootDefinition = std::move(definition);
 pack->definition = pack->rootDefinition;
 pack->dimensionKey.clear();
 initializePackRuntime(*pack);
 refreshResourcePackState(basePack_.get(), packs_);
 activatePack(activeIndex_);
 return true;
}
std::string Pipeline::settingValue(const std::string& key) const {
 const PackInstance* pack = selectedPack();
 if(pack == nullptr) {
  return {};
 }
 const auto found = pack->settings.find(key);
 return found == pack->settings.end() ? std::string() : found->second;
}
void Pipeline::compileExecutionPlan(PackInstance& pack, std::size_t programBudget) {
 if(pack.executionPlanReady) return;
 gui::screen::LoadingDisplay* progress = net::minecraft::client::Minecraft::INSTANCE != nullptr
                                            ? &net::minecraft::client::Minecraft::INSTANCE->progressRenderer
                                            : nullptr;
 const bool showProgress = programBudget == static_cast<std::size_t>(-1);
 if(progress != nullptr && showProgress) progress->progressStart("Compiling shaders");
 const std::size_t total = pack.definition.programs.size();
 std::size_t count = 0;
 std::size_t compiledNow = 0;
 for(const auto& [name, spec] : pack.definition.programs) {
  (void)spec;
  if(progress != nullptr && showProgress && total > 0) {
   progress->progressStagePercentage(static_cast<int>(count * 100 / total));
  }
  if(isProgramEnabledCached(pack.definition, pack.settings, name, pack.programEnabledCache)) {
   if(!pack.compiledPrograms.contains(name)) {
    if(compiledNow >= programBudget) return;
    PackCompiler::compile(pack, name, logFn());
    ++compiledNow;
   }
  }
  ++count;
 }
 std::string error;
 if(!pack.buildExecutionPlan(error)) logOnce(pack, error);
}
void Pipeline::activatePack(std::size_t index) {
 PackInstance* previous = activePack();
 if(index < packs_.size()) {
  const std::size_t candidateIndex = index;
  PackInstance& candidate = *packs_[candidateIndex];
  if(candidate.summary.valid && candidate.resources.empty() && candidate.zip == nullptr) {
   packs_[candidateIndex] = loadPack(candidate.path, candidate.directory);
   if(!packs_[candidateIndex]->summary.valid) {
    index = kNoActivePack;
   }
  }
 }
 activeIndex_ = index;
 refreshSummaries();
 reset();
 if(previous != nullptr && previous != activePack()) previous->clearGpuResources();
 PackInstance* pack = activePack();
 if(pack != nullptr && pack->summary.valid) {
  net::minecraft::World* world = net::minecraft::client::Minecraft::INSTANCE != nullptr ? net::minecraft::client::Minecraft::INSTANCE->world : nullptr;
  selectDimension(*pack, world, false);
  if(world != nullptr) {
   compileExecutionPlan(*pack, static_cast<std::size_t>(-1));
   const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
   preparePackResources(*pack, minecraft != nullptr ? std::max(1, minecraft->displayWidth) : 1,
                               minecraft != nullptr ? std::max(1, minecraft->displayHeight) : 1);
  }
 }
 const PackDefinition& definition = activeDefinition();
 render::block::BlockRenderManager::setVoxelizeLightBlocks(
     activeIndex_ < packs_.size() && definition.voxelizeLightBlocks);
 applyBlockIds(definition);
 reloadWorldMeshes();
}
PackInstance* Pipeline::activePack() noexcept {
 if(activeIndex_ < packs_.size()) {
  return packs_[activeIndex_].get();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_.get() : nullptr;
}
const PackInstance* Pipeline::activePack() const noexcept {
 if(activeIndex_ < packs_.size()) {
  return packs_[activeIndex_].get();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_.get() : nullptr;
}
PackInstance* Pipeline::renderPack() noexcept {
 PackInstance* pack = interfaceProgramsActive() ? basePack_.get() : activePack();
 return pack != nullptr ? pack : basePack_.get();
}
PackInstance* Pipeline::selectedPack() noexcept {
 return activePack();
}
const PackInstance* Pipeline::selectedPack() const noexcept {
 return activePack();
}
bool Pipeline::hasActivePack() const noexcept {
 const PackInstance* pack = activePack();
 return pack != nullptr && pack->summary.valid;
}
const PackDefinition& Pipeline::activeDefinition() const noexcept {
 const PackInstance* pack = activePack();
 return pack != nullptr && pack->summary.valid ? pack->definition : vanillaPackDefinition();
}
const PackDefinition* Pipeline::selectedDefinition() const noexcept {
 const PackInstance* pack = selectedPack();
 return pack != nullptr && pack->summary.valid ? &pack->definition : nullptr;
}
const PackDefinition& Pipeline::meshDefinition() const noexcept {
 if(hasActivePack()) {
  return activeDefinition();
 }
 return basePack_ != nullptr && basePack_->summary.valid ? basePack_->definition : vanillaPackDefinition();
}
bool Pipeline::hasDeferredPasses() const {
 return hasDeferredPasses(activePack());
}
gl::ShaderProgram* Pipeline::worldProgram(WorldProgramId id) {
 return worldProgram(id, renderPack());
}
void Pipeline::applyWorldPassDirectives(WorldProgramId id, gl::ShaderProgram& program) {
 PackInstance* pack = renderPack();
 if(pack == nullptr) return;
 const bool shadowPass = core::cameraFrame().shadowPass;
 const PackInstance::WorldProgramRuntime& runtime =
     pack->worldPrograms[static_cast<std::size_t>(id)][shadowPass ? 1u : 0u];
 const std::string& programKey = runtime.resolvedKey;
 if(programKey.empty()) return;
 const int colorCount = shadowPass ? pack->definition.shadowColorBuffers : pack->colorTargets.colorCount();
 if(!shadowPass || colorCount > 0) program.applyDrawBuffers(std::max(1, colorCount));
 applyBufferBlends(pack->definition, programKey, program.drawBufferColortexIndices());
 applyAlphaTest(pack->definition, programKey);
}
void Pipeline::bindWorldProgramState(gl::ShaderProgram& program, WorldProgramId id, bool withUniforms) {
 WorldProgramBindContext context = makeWorldBindContext(id);
 // Uniform values are per-program GL state and survive rebinding, so a program
 // that already holds this generation's snapshot only needs its samplers pointed
 // at the current texture units again. Null uniforms is bindWorldProgram's own
 // "samplers only" switch.
 if(!withUniforms) {
  context.uniforms = nullptr;
 }
 bindWorldProgram(program, context);
}
void Pipeline::bindWorldProgramMaterial(gl::ShaderProgram& program, WorldProgramId id) {
 bindProgramMaterialTextures(program, makeWorldBindContext(id));
}
int Pipeline::resolveObjectId(const std::string& kind, const std::string& name, int fallback) const {
 const PackDefinition& definition = activeDefinition();
 const auto& ids = kind == "entity" ? definition.entityIds
                                    : kind == "item" ? definition.itemIds : definition.blockIds;
 const std::string key = lower(name);
 if(const auto found = ids.find(key); found != ids.end()) return found->second;
 if(const auto found = ids.find("minecraft:" + key); found != ids.end()) return found->second;
 return fallback;
}
void Pipeline::prepareFrame(net::minecraft::World* world) {
 refreshResourcePackState(basePack_.get(), packs_);
 PackInstance* active = activePack();
 if(active != nullptr && active->summary.valid) {
  selectDimension(*active, world, true);
  if(!active->executionPlanReady) {
   compileExecutionPlan(*active, static_cast<std::size_t>(-1));
   const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
   preparePackResources(*active, minecraft != nullptr ? std::max(1, minecraft->displayWidth) : 1,
                               minecraft != nullptr ? std::max(1, minecraft->displayHeight) : 1);
  }
 }
 if(basePack_ != nullptr && basePack_.get() != active && basePack_->summary.valid &&
    !basePack_->executionPlanReady) {
  compileExecutionPlan(*basePack_, static_cast<std::size_t>(-1));
 }
 prepareFrame(world, activePack(), basePack_.get());
}
void Pipeline::setFrameUniforms(const PackUniformValues& frame) {
 setFrameUniforms(frame, activeDefinition(), activePack());
}
bool Pipeline::renderBegin(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                              const int* shadowColorTextureIds, int shadowColorTextureCount,
                              shadowmap::ShadowTargets* shadowTargets,
                              const int* shadowColorAltTextureIds) {
 return renderBegin(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                    shadowColorTextureIds, shadowColorTextureCount, shadowTargets,
                    shadowColorAltTextureIds);
}
bool Pipeline::renderShadowComposite(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                        const int* shadowColorTextureIds, int shadowColorTextureCount,
                                        shadowmap::ShadowTargets* shadowTargets,
                                        const int* shadowColorAltTextureIds) {
 return renderShadowComposite(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                              shadowColorTextureIds, shadowColorTextureCount, shadowTargets,
                              shadowColorAltTextureIds);
}
bool Pipeline::renderPreWorld(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                 const int* shadowColorTextureIds, int shadowColorTextureCount,
                                 shadowmap::ShadowTargets* shadowTargets,
                                 const int* shadowColorAltTextureIds) {
 return renderPreWorld(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                       shadowColorTextureIds, shadowColorTextureCount, shadowTargets,
                       shadowColorAltTextureIds);
}
bool Pipeline::renderDeferred(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                 const int* shadowColorTextureIds, int shadowColorTextureCount,
                                 const int* shadowColorAltTextureIds) {
 return renderDeferred(activePack(), shadowDepthTextureId, shadowOpaqueDepthTextureId,
                       shadowColorTextureIds, shadowColorTextureCount, shadowColorAltTextureIds);
}
bool Pipeline::renderPostProcess(int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
                                    const int* shadowColorTextureIds, int shadowColorTextureCount,
                                    const int* shadowColorAltTextureIds) {
 return renderPostProcess(activePack(), basePack_.get(), shadowDepthTextureId,
                          shadowOpaqueDepthTextureId, shadowColorTextureIds,
                          shadowColorTextureCount, shadowColorAltTextureIds);
}
void Pipeline::sampleCenterDepth() {
 sampleCenterDepth(activePack(), activeDefinition());
}
void Pipeline::captureOpaqueDepth() {
 captureOpaqueDepth(activePack());
}
void Pipeline::captureHandDepth() {
 captureHandDepth(activePack());
}
PackCompiler::LogFnLevel Pipeline::logFn() {
 return [this](PackInstance& p, const std::string& message, ::net::minecraft::util::logging::LogLevel level) {
  logOnce(p, message, level);
 };
}
bool Pipeline::ensureSceneTargets(int width, int height) {
 return ensureSceneTargets(activePack(), width, height);
}
void Pipeline::bindScene() {
 bindScene(activePack());
}
void Pipeline::endScene() {
 endScene(activePack());
}
int Pipeline::sceneColorCount() const {
 return sceneColorCount(activePack());
}
void Pipeline::clearScene(float fogR, float fogG, float fogB) {
 clearScene(activePack(), fogR, fogG, fogB);
}
} // namespace net::minecraft::client::render
