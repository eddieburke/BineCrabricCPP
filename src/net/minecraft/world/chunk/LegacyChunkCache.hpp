#pragma once

#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/ClientChunkStream.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace net::minecraft {

class World;

// Thin ChunkSource adapter over ClientChunkStream (sparse residency + async load).
class LegacyChunkCache : public ChunkSource {
public:
    LegacyChunkCache(World* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator)
        : storage_(std::move(storage)),
          stream_(world, storage_.get(), generator)
    {
    }

    void setSpawnPoint(int chunkX, int chunkZ) { stream_.setCenter(chunkX, chunkZ); }

    void setActiveRadius(int radius) { stream_.setResidentRadius(radius); }

    void initAsync(std::uint64_t seed, std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory)
    {
        stream_.initAsync(seed, std::move(genFactory));
    }

    void prefetch(int centerChunkX, int centerChunkZ)
    {
        stream_.setCenter(centerChunkX, centerChunkZ);
        stream_.prefetch();
    }

    void populateReadyChunks() { stream_.populateReadyChunks(); }

    [[nodiscard]] bool isSpawnChunk(int chunkX, int chunkZ) const
    {
        return stream_.isWithinResidentRadius(chunkX, chunkZ);
    }

    [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override
    {
        return stream_.isChunkLoaded(chunkX, chunkZ);
    }

    Chunk& loadChunk(int chunkX, int chunkZ) override { return getChunk(chunkX, chunkZ); }

    Chunk& getChunk(int chunkX, int chunkZ) override { return stream_.getChunk(chunkX, chunkZ); }

    void decorate(ChunkSource* source, int chunkX, int chunkZ) override
    {
        stream_.decorate(source, chunkX, chunkZ);
    }

    bool save(bool saveEntities, client::gui::screen::LoadingDisplay* display) override
    {
        return stream_.save(saveEntities, display);
    }

    bool tick() override { return stream_.tick(); }

    [[nodiscard]] bool canSave() const override { return true; }

    [[nodiscard]] std::string getDebugInfo() const override { return stream_.debugInfo(); }

private:
    std::unique_ptr<ChunkStorage> storage_ {};
    ClientChunkStream stream_;
};

} // namespace net::minecraft
