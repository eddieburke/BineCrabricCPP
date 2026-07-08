#pragma once
#include <filesystem>
#include <memory>
#include <optional>

#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"

namespace net::minecraft {
namespace fs = std::filesystem;

class NullChunkStorage : public ChunkStorage {
   public:
    Chunk loadChunk(World* world, int chunkX, int chunkZ) override {
        return EmptyChunk(world, chunkX, chunkZ);
    }

    void saveChunk(World* world, Chunk& chunk) override {
        (void) world;
        (void) chunk;
    }

    void saveEntities(World* world, Chunk& chunk) override {
        (void) world;
        (void) chunk;
    }

    void tick() override {
    }

    void flush() override {
    }
};

class EmptyWorldStorage : public WorldStorage {
   public:
    [[nodiscard]] std::optional<WorldProperties> loadProperties() override {
        return std::nullopt;
    }

    void checkSessionLock() override {
    }

    [[nodiscard]] std::unique_ptr<ChunkStorage> getChunkStorage(const Dimension* dimension) override {
        (void) dimension;
        return std::make_unique<NullChunkStorage>();
    }

    void save(const WorldProperties& properties, const std::vector<entity::player::PlayerEntity*>& players) override {
        (void) properties;
        (void) players;
    }

    void save(const WorldProperties& properties) override {
        (void) properties;
    }

    [[nodiscard]] server::world::PlayerSaveHandler* getPlayerSaveHandler() override {
        return nullptr;
    }

    void forceSave() override {
    }

    void refreshSessionLock() override {
    }

    [[nodiscard]] fs::path getWorldPropertiesFile(const std::string& name) const override {
        (void) name;
        return {};
    }

    [[nodiscard]] fs::path worldDirectory() const override {
        return {};
    }

    [[nodiscard]] std::string worldName() const override {
        return {};
    }
};
}  // namespace net::minecraft
