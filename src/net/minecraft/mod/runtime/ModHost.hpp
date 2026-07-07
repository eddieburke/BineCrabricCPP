#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace net::minecraft::mod::runtime {
enum class ModPackageSource {
  Directory,
  Zip
};
struct ModPackage {
  std::string id;
  std::string name;
  std::string version;
  std::string description;
  std::string entry;
  ModPackageSource source = ModPackageSource::Directory;
  bool enabledByDefault = true;
  bool configuredEnabled = true;
  bool active = false;
  bool resourceOverlay = false;
  bool runtimeScript = false;
  std::string error;
  std::filesystem::path sourcePath;
  std::filesystem::path rootPath;
};
class ModHost {
public:
  struct LoadedLuaMod {
    struct Callback {
      std::string event;
      int functionRef = 0;
      int priority = 0;
    };
    std::string modId;
    void* state = nullptr;
    bool active = false;
    std::recursive_mutex stateMutex;
    std::vector<Callback> callbacks;
    std::vector<int> buttonCallbackRefs;
    std::vector<int> blockModelCallbackRefs;
    std::vector<int> ownedTextureIds;
  };
  void initialize(const std::filesystem::path& runDirectory);
  void shutdown();
  void rescan();
  [[nodiscard]] bool initialized() const noexcept {
    return initialized_;
  }
  [[nodiscard]] bool isEnabled(const std::string& modId, bool enabledByDefault = true) const;
  bool setEnabled(const std::string& modId, bool enabled);
  void loadEnabledPackageMods();
  [[nodiscard]] std::vector<ModPackage> packageMods() const;
  [[nodiscard]] std::vector<std::uint8_t> readResource(std::string_view path) const;
  [[nodiscard]] std::filesystem::path modsDirectory() const;
  [[nodiscard]] std::filesystem::path runDirectory() const;
  [[nodiscard]] std::filesystem::path assetPath(std::string_view modId, std::string_view relativePath) const;
  [[nodiscard]] const ModPackage* findPackageMod(const std::string& modId) const;
  [[nodiscard]] const std::vector<std::shared_ptr<LoadedLuaMod>>& loadedMods() const {
    return loadedLuaMods_;
  }
  static std::filesystem::path defaultRunDirectory();

private:
  void loadStateFile();
  void saveStateFile() const;
  ModPackage* findPackageMod(const std::string& modId);
  std::filesystem::path runDirectory_;
  std::filesystem::path modsDirectory_;
  std::filesystem::path cacheDirectory_;
  bool initialized_ = false;
  bool packageModsLoaded_ = false;
  std::unordered_map<std::string, bool> savedEnabledStates_;
  std::vector<ModPackage> packageMods_;
  std::vector<std::shared_ptr<LoadedLuaMod>> loadedLuaMods_;
};
ModHost& host();
const std::vector<std::shared_ptr<ModHost::LoadedLuaMod>>& loadedLuaMods();
} // namespace net::minecraft::mod::runtime
