#pragma once

#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include "net/minecraft/server/world/PlayerSaveHandler.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace net::minecraft {

namespace fs = std::filesystem;

class Dimension;

namespace entity::player {
class PlayerEntity;
}

class WorldStorage {
public:
    virtual ~WorldStorage() = default;

    [[nodiscard]] virtual std::optional<WorldProperties> loadProperties() = 0;
    virtual void checkSessionLock() = 0;
    [[nodiscard]] virtual std::unique_ptr<ChunkStorage> getChunkStorage(const Dimension* dimension) = 0;
    virtual void save(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players) = 0;
    virtual void save(const WorldProperties& properties) = 0;
    [[nodiscard]] virtual PlayerSaveHandler* getPlayerSaveHandler() = 0;
    virtual void forceSave() = 0;
    [[nodiscard]] virtual fs::path getWorldPropertiesFile(const std::string& name) const = 0;
};

} // namespace net::minecraft
