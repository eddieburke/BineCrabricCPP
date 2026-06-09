#pragma once

#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/server/world/PlayerSaveHandler.hpp"
#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace net::minecraft {

namespace fs = std::filesystem;

namespace entity::player {
class PlayerEntity;
}

class AlphaWorldStorage : public WorldStorage, public PlayerSaveHandler {
public:
    AlphaWorldStorage(fs::path savesDir, std::string name, bool createPlayerDataDir = false);

    [[nodiscard]] fs::path getDirectory() const { return dir_; }

    void checkSessionLock() override;
    [[nodiscard]] std::unique_ptr<ChunkStorage> getChunkStorage(const Dimension* dimension) override;
    [[nodiscard]] std::optional<WorldProperties> loadProperties() override;
    void save(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players) override;
    void save(const WorldProperties& properties) override;

    void savePlayerData(entity::player::PlayerEntity& player) override;
    void loadPlayerData(entity::player::PlayerEntity& player) override;
    [[nodiscard]] Nbt loadPlayerData(const std::string& playerName);

    [[nodiscard]] PlayerSaveHandler* getPlayerSaveHandler() override { return this; }
    void forceSave() override {}
    [[nodiscard]] fs::path getWorldPropertiesFile(const std::string& name) const override;

protected:
    void writeLevelDat(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players);
    [[nodiscard]] static std::optional<WorldProperties> loadPropertiesFrom(const fs::path& file);

    fs::path dir_;
    fs::path playerDataDir_;
    fs::path dataDir_;
    std::uint64_t startTime_ = 0;
};

} // namespace net::minecraft
