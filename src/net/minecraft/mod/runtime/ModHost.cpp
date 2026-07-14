#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaRuntimePrelude.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/runtime/LuaBlockBindings.hpp"
#include "net/minecraft/mod/runtime/LuaItemBindings.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/mod/runtime/LuaCameraBindings.hpp"
#include "net/minecraft/mod/runtime/LuaFboBindings.hpp"
#include "net/minecraft/mod/runtime/LuaInventoryBindings.hpp"
#include "net/minecraft/mod/runtime/LuaModelBindings.hpp"
#include "net/minecraft/mod/runtime/LuaRaycastBindings.hpp"
#include "net/minecraft/mod/runtime/LuaRenderBindings.hpp"
#include "net/minecraft/mod/runtime/LuaScreenBindings.hpp"
#include "net/minecraft/mod/runtime/LuaSoundBindings.hpp"
#include "net/minecraft/mod/runtime/LuaTextureBindings.hpp"
#include "net/minecraft/mod/runtime/LuaShaderBindings.hpp"
#endif
#include "net/minecraft/mod/runtime/LuaBlockEntityBindings.hpp"
#include "net/minecraft/mod/runtime/LuaCoreBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEntityBindings.hpp"
#include "net/minecraft/mod/runtime/LuaEventSubscribers.hpp"
#include "net/minecraft/mod/runtime/LuaRecipeBindings.hpp"
#include "net/minecraft/mod/runtime/LuaWorldBindings.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/mod/runtime/ModPackageIo.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/lua/LuaModApi.hpp"
#endif
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
namespace net::minecraft::mod::runtime {
namespace {
using namespace net::minecraft::mod::lua;
void installMinecraftTable(lua_State* state, ModHost::LoadedLuaMod& mod) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 12);
  setField(state, "mod_id", mod.modId);
  installCoreApi(state, mod);
  installWorldApi(state, mod);
  installEntityApi(state, mod);
  installBlockApi(state, mod);
  installItemApi(state, mod);
#ifdef MINECRAFT_NATIVE_EXPORTS
  installCameraApi(state);
  installFboApi(state);
  installShaderApi(state);
  installTextureApi(state);
  installModelApi(state, mod);
  installSoundApi(state, mod);
  installInventoryApi(state);
  installScreenApi(state, mod);
  installRenderApi(state);
  installRaycastApi(state);
#endif
  installTileEntityApi(state);
  installRecipeApi(state, mod);
#ifdef MINECRAFT_NATIVE_EXPORTS
  net::minecraft::mod::lua::installGenericModApi(state, mod);
#endif
  api.setglobal(state, "minecraft");
}
bool installLuaPrelude(lua_State* state, std::string& error) {
  LuaApi& api = luaApi();
  const std::string_view source = net::minecraft::mod::lua::kRuntimePrelude;
  int status = api.loadbufferx(state, source.data(), source.size(), "@minecraft/runtime.lua", "t");
  if(status == kLuaOk) {
    status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  }
  if(status == kLuaOk) {
    return true;
  }
  const char* message = api.tolstring(state, -1, nullptr);
  error = message != nullptr ? message : "failed to initialize the Lua runtime prelude";
  api.settop(state, 0);
  return false;
}
bool loadLuaMod(ModPackage& info, std::vector<std::shared_ptr<ModHost::LoadedLuaMod>>& loadedMods) {
  if(info.entry.empty()) {
    info.active = info.resourceOverlay;
    return true;
  }
  if(!info.runtimeScript) {
    info.error = "Only Lua script entries are supported";
    info.active = info.resourceOverlay;
    runtimeLog(info.id, "error", info.error);
    return false;
  }
  if(!isSafeRelativePath(info.entry)) {
    info.error = "Unsafe script path";
    info.active = info.resourceOverlay;
    runtimeLog(info.id, "error", info.error);
    return false;
  }
  LuaApi& api = luaApi();
  if(!api.ready()) {
    info.error = "Lua runtime unavailable";
    info.active = info.resourceOverlay;
    runtimeLog(info.id, "error", info.error);
    return false;
  }
  const std::filesystem::path scriptPath = info.rootPath / std::filesystem::path(info.entry);
  if(!std::filesystem::is_regular_file(scriptPath)) {
    info.error = "Missing Lua script: " + scriptPath.string();
    info.active = info.resourceOverlay;
    runtimeLog(info.id, "error", info.error);
    return false;
  }
  lua_State* state = api.newstate();
  if(state == nullptr) {
    info.error = "Failed to create Lua state";
    info.active = info.resourceOverlay;
    runtimeLog(info.id, "error", info.error);
    return false;
  }
  api.openlibs(state);
  auto mod = std::make_shared<ModHost::LoadedLuaMod>();
  mod->modId = info.id;
  mod->state = state;
  installMinecraftTable(state, *mod);
  if(!installLuaPrelude(state, info.error)) {
    api.close(state);
    info.active = info.resourceOverlay;
    runtimeLog(info.id, "error", info.error);
    return false;
  }
  // Mark active before running the script so the mod's own resource lookups
  // (e.g. minecraft.model.json during top-level init) can find its files via
  // findResourceFile, which only searches active mods.
  info.active = true;
  const std::string script = scriptPath.string();
  int status = api.loadfilex(state, script.c_str(), "t");
  if(status == kLuaOk) {
    status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  }
  if(status != kLuaOk) {
    const char* error = api.tolstring(state, -1, nullptr);
    info.error = error != nullptr ? error : "Lua script failed to load";
    api.close(state);
    info.active = info.resourceOverlay;
    runtimeLog(info.id, "error", info.error);
    return false;
  }
  mod->active = true;
  for(const auto& callback : mod->callbacks) {
    subscribeLuaCallback(mod, callback);
  }
  loadedMods.push_back(std::move(mod));
  info.error.clear();
  runtimeLog(info.id, "info", "loaded " + info.name + " " + info.version);
  return true;
}
} // namespace
ModHost& host() {
  static ModHost value;
  return value;
}
const std::vector<std::shared_ptr<ModHost::LoadedLuaMod>>& loadedLuaMods() {
  return host().loadedMods();
}
std::filesystem::path ModHost::defaultRunDirectory() {
  const char* appData = std::getenv("APPDATA");
  if(appData != nullptr && *appData != '\0') {
    return std::filesystem::path(appData) / ".minecraft";
  }
  const char* home = std::getenv("USERPROFILE");
  if(home != nullptr && *home != '\0') {
    return std::filesystem::path(home) / ".minecraft";
  }
  return std::filesystem::current_path() / ".minecraft";
}
void ModHost::initialize(const std::filesystem::path& runDirectory) {
  if(initialized_) {
    return;
  }
  runDirectory_ = runDirectory;
  modsDirectory_ = runDirectory_ / "mods";
  cacheDirectory_ = modsDirectory_ / ".cache";
  std::filesystem::create_directories(modsDirectory_);
  std::filesystem::create_directories(cacheDirectory_);
  initialized_ = true;
  loadStateFile();
  rescan();
}
void ModHost::shutdown() {
  LuaApi* api = loadedLuaMods_.empty() ? nullptr : &luaApi();
  for(const std::shared_ptr<LoadedLuaMod>& mod : loadedLuaMods_) {
    if(mod == nullptr) {
      continue;
    }
    hooks().unsubscribeOwner(mod.get());
    const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
    mod->active = false;
    if(api != nullptr && api->ready() && mod->state != nullptr) {
      auto* state = static_cast<lua_State*>(mod->state);
      for(const LoadedLuaMod::Callback& callback : mod->callbacks) {
        if(callback.functionRef != kLuaNoRef) {
          api->unref(state, kLuaRegistryIndex, callback.functionRef);
        }
      }
      for(const int ref : mod->buttonCallbackRefs) {
        if(ref != kLuaNoRef) {
          api->unref(state, kLuaRegistryIndex, ref);
        }
      }
      mod->buttonCallbackRefs.clear();
      for(const int ref : mod->blockModelCallbackRefs) {
        if(ref != kLuaNoRef) {
          api->unref(state, kLuaRegistryIndex, ref);
        }
      }
      mod->blockModelCallbackRefs.clear();
      for(const int ref : mod->itemModelCallbackRefs) {
        if(ref != kLuaNoRef) {
          api->unref(state, kLuaRegistryIndex, ref);
        }
      }
      mod->itemModelCallbackRefs.clear();
#ifdef MINECRAFT_NATIVE_EXPORTS
      if(client::Minecraft::INSTANCE != nullptr) {
        for(const int textureId : mod->ownedTextureIds) {
          client::Minecraft::INSTANCE->textureManager.deleteTexture(textureId);
        }
      }
#endif
      mod->ownedTextureIds.clear();
      api->close(state);
      mod->state = nullptr;
    }
  }
  loadedLuaMods_.clear();
  packageMods_.clear();
  packageModsLoaded_ = false;
  initialized_ = false;
}
void ModHost::loadStateFile() {
  savedEnabledStates_.clear();
  const std::filesystem::path statePath = modsDirectory_ / "mod-state.cfg";
  std::ifstream input(statePath);
  if(!input.is_open()) {
    return;
  }
  std::string line;
  while(std::getline(input, line)) {
    line = trimCopy(line);
    if(line.empty() || line.starts_with('#') || line.starts_with(';')) {
      continue;
    }
    const std::size_t equals = line.find('=');
    if(equals == std::string::npos) {
      continue;
    }
    const std::string id = trimCopy(line.substr(0, equals));
    const std::string value = toLowerCopy(trimCopy(line.substr(equals + 1)));
    if(!id.empty()) {
      savedEnabledStates_[id] = value == "1" || value == "true" || value == "yes" || value == "on";
    }
  }
}
void ModHost::saveStateFile() const {
  std::vector<std::string> ids;
  ids.reserve(savedEnabledStates_.size());
  for(const auto& [id, _] : savedEnabledStates_) {
    ids.push_back(id);
  }
  std::sort(ids.begin(), ids.end());
  std::ostringstream output;
  output << "# Minecraft Native Lua mod enable state\n";
  for(const std::string& id : ids) {
    const auto it = savedEnabledStates_.find(id);
    if(it != savedEnabledStates_.end()) {
      output << id << '=' << (it->second ? "1" : "0") << '\n';
    }
  }
  (void)writeFileText(modsDirectory_ / "mod-state.cfg", output.str());
}
ModPackage* ModHost::findPackageMod(const std::string& modId) {
  for(ModPackage& mod : packageMods_) {
    if(mod.id == modId) {
      return &mod;
    }
  }
  return nullptr;
}
const ModPackage* ModHost::findPackageMod(const std::string& modId) const {
  for(const ModPackage& mod : packageMods_) {
    if(mod.id == modId) {
      return &mod;
    }
  }
  return nullptr;
}
bool ModHost::isEnabled(const std::string& modId, bool enabledByDefault) const {
  const auto it = savedEnabledStates_.find(modId);
  if(it != savedEnabledStates_.end()) {
    return it->second;
  }
  if(const ModPackage* pkg = findPackageMod(modId)) {
    return pkg->configuredEnabled;
  }
  return enabledByDefault;
}
bool ModHost::setEnabled(const std::string& modId, bool enabled) {
  savedEnabledStates_[modId] = enabled;
  if(ModPackage* mod = findPackageMod(modId)) {
    mod->configuredEnabled = enabled;
  }
  saveStateFile();
  return true;
}
void ModHost::rescan() {
  if(!initialized_) {
    return;
  }
  loadStateFile();
  std::unordered_map<std::string, ModPackage> previous;
  for(const ModPackage& mod : packageMods_) {
    previous.emplace(mod.id, mod);
  }
  packageMods_.clear();
  std::set<std::string> seenIds;
  if(std::filesystem::exists(modsDirectory_) && std::filesystem::is_directory(modsDirectory_)) {
    for(auto it = std::filesystem::recursive_directory_iterator(modsDirectory_);
        it != std::filesystem::recursive_directory_iterator();
        ++it) {
      const auto& entry = *it;
      const std::filesystem::path path = entry.path();
      const std::string filename = path.filename().string();
      if(entry.is_directory()) {
        if(filename == ".cache") {
          it.disable_recursion_pending();
          continue;
        }
        const std::filesystem::path manifestPath = path / "mod.json";
        if(!std::filesystem::is_regular_file(manifestPath)) {
          continue;
        }
        ModPackage modInfo;
        const bool discovered = parseManifestJson(
            readFileText(manifestPath), modInfo, path, ModPackageSource::Directory, "mod.json: ");
        if(discovered) {
          modInfo.rootPath = path;
          modInfo.resourceOverlay = std::filesystem::is_directory(modInfo.rootPath / "resources");
        }
        modInfo.configuredEnabled = isEnabled(modInfo.id, modInfo.enabledByDefault);
        if(const auto previousIt = previous.find(modInfo.id); previousIt != previous.end()) {
          modInfo.active = previousIt->second.active;
          if(modInfo.error.empty()) {
            modInfo.error = previousIt->second.error;
          }
        }
        if(!seenIds.insert(modInfo.id).second) {
          modInfo.id += "__duplicate__" + std::to_string(packageMods_.size());
          modInfo.error = "Duplicate mod id detected";
          modInfo.configuredEnabled = false;
        }
        packageMods_.push_back(std::move(modInfo));
        it.disable_recursion_pending();
        continue;
      }
      if(!entry.is_regular_file() || filename == "mod-state.cfg" ||
         toLowerCopy(path.extension().string()) != ".zip") {
        continue;
      }
      ModPackage modInfo;
      bool discovered = false;
      std::vector<ZipEntry> zipEntries;
      std::error_code archiveSizeError;
      const std::uintmax_t archiveSize = std::filesystem::file_size(path, archiveSizeError);
      std::vector<std::uint8_t> archive;
      if(archiveSizeError || archiveSize > kMaxModArchiveBytes) {
        modInfo = makeBrokenPackage(
            ModPackageSource::Zip, path, sanitizeName(path.stem().string()), "Mod archive is too large");
      } else {
        archive = readFileBytes(path);
      }
      if(modInfo.id.empty() && !buildZipIndex(archive, zipEntries)) {
        modInfo = makeBrokenPackage(
            ModPackageSource::Zip, path, sanitizeName(path.stem().string()), "Unable to read zip");
      } else if(modInfo.id.empty()) {
        const ZipEntry* manifestEntry = findZipEntry(zipEntries, "mod.json");
        if(manifestEntry == nullptr) {
          modInfo = makeBrokenPackage(ModPackageSource::Zip,
                                      path,
                                      sanitizeName(path.stem().string()),
                                      "Zip package is missing mod.json");
        } else {
          const std::vector<std::uint8_t> manifestBytes = readZipEntryData(archive, *manifestEntry);
          const std::string manifestText(manifestBytes.begin(), manifestBytes.end());
          discovered = parseManifestJson(manifestText, modInfo, path, ModPackageSource::Zip, "mod.json: ");
          if(discovered) {
            std::error_code ec;
            const auto stamp = std::filesystem::last_write_time(path, ec).time_since_epoch().count();
            const std::string cacheName = sanitizeName(path.stem().string()) + "_" +
                                          std::to_string(archiveSize) + "_" + std::to_string(stamp);
            modInfo.rootPath = cacheDirectory_ / cacheName;
            std::filesystem::remove_all(modInfo.rootPath, ec);
            std::filesystem::create_directories(modInfo.rootPath);
            for(const ZipEntry& zipEntry : zipEntries) {
              const std::string relativePath = normalizeRelativePath(zipEntry.name);
              if(relativePath.empty() || isDirectoryZipPath(relativePath) ||
                 !isSafeRelativePath(relativePath)) {
                continue;
              }
              const std::filesystem::path outPath =
                  modInfo.rootPath / std::filesystem::path(relativePath);
              (void)writeFileBytes(outPath, readZipEntryData(archive, zipEntry));
            }
            modInfo.resourceOverlay = std::filesystem::is_directory(modInfo.rootPath / "resources");
          }
        }
      }
      if(modInfo.id.empty()) {
        continue;
      }
      modInfo.configuredEnabled = isEnabled(modInfo.id, modInfo.enabledByDefault);
      if(const auto previousIt = previous.find(modInfo.id); previousIt != previous.end()) {
        modInfo.active = previousIt->second.active;
        if(modInfo.error.empty()) {
          modInfo.error = previousIt->second.error;
        }
      }
      if(!seenIds.insert(modInfo.id).second) {
        modInfo.id += "__duplicate__" + std::to_string(packageMods_.size());
        modInfo.error = "Duplicate mod id detected";
        modInfo.configuredEnabled = false;
      }
      packageMods_.push_back(std::move(modInfo));
    }
  }
  sortMods(packageMods_);
  if(packageModsLoaded_) {
    for(ModPackage& mod : packageMods_) {
      if(!mod.configuredEnabled || mod.active) {
        continue;
      }
      (void)loadLuaMod(mod, loadedLuaMods_);
    }
  }
}
void ModHost::loadEnabledPackageMods() {
  if(packageModsLoaded_) {
    return;
  }
  packageModsLoaded_ = true;
  for(ModPackage& mod : packageMods_) {
    mod.active = false;
    if(!mod.configuredEnabled) {
      continue;
    }
    (void)loadLuaMod(mod, loadedLuaMods_);
  }
}
std::vector<ModPackage> ModHost::packageMods() const {
  return packageMods_;
}
std::optional<std::filesystem::path> ModHost::findResourceFile(std::string_view path) const {
  const std::string normalized = normalizeRelativePath(path);
  if(normalized.empty() || !isSafeRelativePath(normalized)) {
    return std::nullopt;
  }
  for(auto it = packageMods_.rbegin(); it != packageMods_.rend(); ++it) {
    if(!it->active || it->rootPath.empty()) {
      continue;
    }
    if(it->resourceOverlay) {
      const std::filesystem::path overlayCandidate =
          it->rootPath / "resources" / std::filesystem::path(normalized);
      if(std::filesystem::is_regular_file(overlayCandidate)) {
        return overlayCandidate;
      }
    }
    const std::filesystem::path assetCandidate = it->rootPath / std::filesystem::path(normalized);
    if(std::filesystem::is_regular_file(assetCandidate)) {
      return assetCandidate;
    }
    const std::filesystem::path assetsCandidate = it->rootPath / "assets" / std::filesystem::path(normalized);
    if(std::filesystem::is_regular_file(assetsCandidate)) {
      return assetsCandidate;
    }
  }
  return std::nullopt;
}
std::vector<std::uint8_t> ModHost::readResource(std::string_view path) const {
  if(const std::optional<std::filesystem::path> filePath = findResourceFile(path); filePath.has_value()) {
    return readFileBytes(*filePath);
  }
  return {};
}
std::optional<std::filesystem::path> ModHost::resolveResourcePath(std::string_view path) const {
  return findResourceFile(path);
}
std::filesystem::path ModHost::modsDirectory() const {
  return modsDirectory_;
}
std::filesystem::path ModHost::runDirectory() const {
  return runDirectory_;
}
std::filesystem::path ModHost::assetPath(std::string_view modId, std::string_view relativePath) const {
  if(!isSafeRelativePath(relativePath)) {
    return {};
  }
  const ModPackage* mod = findPackageMod(std::string(modId));
  if(mod == nullptr || mod->rootPath.empty()) {
    return {};
  }
  const std::filesystem::path candidate = mod->rootPath / std::filesystem::path(normalizeRelativePath(relativePath));
  if(!std::filesystem::exists(candidate)) {
    return {};
  }
  return candidate;
}
} // namespace net::minecraft::mod::runtime
