#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include "net/minecraft/server/world/PlayerSaveHandler.hpp"
#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
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
 // One save entry point. The player travels inside `properties`; callers fold the live
 // player in with World::propertiesWithPlayer() before calling.
 virtual void save(const WorldProperties& properties) = 0;
 [[nodiscard]] virtual server::world::PlayerSaveHandler* getPlayerSaveHandler() = 0;
 virtual void forceSave() = 0;
 virtual void refreshSessionLock() = 0;
 [[nodiscard]] virtual fs::path getWorldPropertiesFile(const std::string& name) const = 0;
 [[nodiscard]] virtual fs::path worldDirectory() const = 0;
 [[nodiscard]] virtual std::string worldName() const = 0;
};
} // namespace net::minecraft
