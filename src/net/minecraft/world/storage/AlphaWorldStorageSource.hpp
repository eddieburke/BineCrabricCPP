#pragma once
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include "net/minecraft/world/storage/WorldSaveInfo.hpp"
#include "net/minecraft/world/storage/WorldStorageSource.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
namespace net::minecraft {
namespace fs = std::filesystem;
class AlphaWorldStorageSource : public WorldStorageSource {
public:
  explicit AlphaWorldStorageSource(fs::path dir) : dir_(std::move(dir)) {
    if(!fs::exists(dir_)) {
      fs::create_directories(dir_);
    }
  }
  [[nodiscard]] std::string getName() const override {
    return "Old Format";
  }
  [[nodiscard]] std::vector<WorldSaveInfo> getAll() override {
    std::vector<WorldSaveInfo> saves;
    for(int i = 0; i < 5; ++i) {
      const std::string saveName = "World" + std::to_string(i + 1);
      const std::optional<WorldProperties> props = getWorldProperties(saveName);
      if(!props.has_value()) {
        continue;
      }
      WorldProperties propsCopy = *props;
      saves.emplace_back(saveName, "", propsCopy.setLastPlayed(), propsCopy.getSizeOnDisk(), false);
    }
    return saves;
  }
  void flush() override {}
  [[nodiscard]] std::optional<WorldProperties> getWorldProperties(const std::string& string) override {
    const fs::path saveDir = dir_ / string;
    if(!fs::exists(saveDir)) {
      return std::nullopt;
    }
    if(const std::optional<WorldProperties> loaded = loadLevelDat(saveDir / "level.dat"); loaded.has_value()) {
      return loaded;
    }
    return loadLevelDat(saveDir / "level.dat_old");
  }
  void rename(const std::string& saveName, const std::string& newName) override {
    const fs::path saveDir = dir_ / saveName;
    if(!fs::exists(saveDir)) {
      return;
    }
    const fs::path levelDat = saveDir / "level.dat";
    if(!fs::exists(levelDat)) {
      return;
    }
    try {
      std::ifstream input(levelDat, std::ios::binary);
      if(!input) {
        return;
      }
      NbtCompound root = NbtIo::readCompressed(input);
      NbtCompound data = root.getCompound("Data");
      data.putString("LevelName", newName);
      root.put("Data", data);
      std::ofstream output(levelDat, std::ios::binary | std::ios::trunc);
      if(output) {
        NbtIo::writeCompressed(root, output);
      }
    } catch(...) {
    }
  }
  void deleteSave(const std::string& saveName) override {
    const fs::path saveDir = dir_ / saveName;
    if(!fs::exists(saveDir)) {
      return;
    }
    std::vector<fs::path> children;
    for(const fs::directory_entry& entry : fs::directory_iterator(saveDir)) {
      children.push_back(entry.path());
    }
    deleteFilesAndDirs(children);
    fs::remove(saveDir);
  }
  [[nodiscard]] std::unique_ptr<WorldStorage> getSaveLoader(const std::string& string, bool bl) override {
    return std::make_unique<AlphaWorldStorage>(dir_, string, bl);
  }
  [[nodiscard]] bool needsConversion(const std::string&) const override {
    return false;
  }
  bool convert(const std::string&, client::gui::screen::LoadingDisplay*) override {
    return false;
  }

protected:
  [[nodiscard]] const fs::path& savesDirectory() const noexcept {
    return dir_;
  }
  static void deleteFilesAndDirs(const std::vector<fs::path>& files) {
    for(const fs::path& path : files) {
      if(fs::is_directory(path)) {
        std::vector<fs::path> children;
        for(const fs::directory_entry& entry : fs::directory_iterator(path)) {
          children.push_back(entry.path());
        }
        deleteFilesAndDirs(children);
      }
      fs::remove(path);
    }
  }

private:
  [[nodiscard]] static std::optional<WorldProperties> loadLevelDat(const fs::path& file) {
    if(!fs::exists(file)) {
      return std::nullopt;
    }
    std::ifstream input(file, std::ios::binary);
    if(!input) {
      return std::nullopt;
    }
    try {
      const NbtCompound root = NbtIo::readCompressed(input);
      if(!root.contains("Data")) {
        return std::nullopt;
      }
      return WorldProperties(root.getCompound("Data"));
    } catch(...) {
      return std::nullopt;
    }
  }
  fs::path dir_;
};
} // namespace net::minecraft
