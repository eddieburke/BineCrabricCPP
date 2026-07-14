#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtFileIo.hpp"
#include "net/minecraft/server/world/PlayerSaveHandler.hpp"
#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
namespace net::minecraft {
namespace fs = std::filesystem;
namespace entity::player {
class PlayerEntity;
}
class AlphaWorldStorage : public WorldStorage, public server::world::PlayerSaveHandler {
public:
  AlphaWorldStorage(fs::path savesDir, std::string name, bool createPlayerDataDir = false);
  [[nodiscard]] fs::path getDirectory() const {
    return dir_;
  }
  [[nodiscard]] fs::path worldDirectory() const override {
    return dir_;
  }
  [[nodiscard]] std::string worldName() const override {
    return worldName_;
  }
  void checkSessionLock() override;
  [[nodiscard]] std::unique_ptr<ChunkStorage> getChunkStorage(const Dimension* dimension) override;
  [[nodiscard]] std::optional<WorldProperties> loadProperties() override;
  void save(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players) override;
  void save(const WorldProperties& properties) override;
  void saveUnload(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players);
  void savePlayerData(entity::player::PlayerEntity& player) override;
  void loadPlayerData(entity::player::PlayerEntity& player) override;
  void refreshSessionLock() override;
  [[nodiscard]] Nbt loadPlayerData(const std::string& playerName);
  [[nodiscard]] server::world::PlayerSaveHandler* getPlayerSaveHandler() override {
    return this;
  }
  void forceSave() override {
  }
  [[nodiscard]] fs::path getWorldPropertiesFile(const std::string& name) const override;

protected:
  void writeSessionLock();
  void writeLevelDat(const WorldProperties& properties,
                     const std::vector<entity::player::PlayerEntity*>& players,
                     AtomicWriteOptions options = {});
  [[nodiscard]] static std::optional<WorldProperties> loadPropertiesFrom(const fs::path& file);
  fs::path dir_;
  fs::path playerDataDir_;
  fs::path dataDir_;
  std::string worldName_;
  std::uint64_t startTime_ = 0;
};
} // namespace net::minecraft
