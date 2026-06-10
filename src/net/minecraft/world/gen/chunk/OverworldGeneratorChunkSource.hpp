#pragma once

#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"

#include <optional>
#include <string>

namespace net::minecraft {

class World;

// Wraps OverworldChunkGenerator as a ChunkSource (Java OverworldChunkGenerator implements ChunkSource).
class OverworldGeneratorChunkSource : public ChunkSource {
public:
    OverworldGeneratorChunkSource(World* world, std::uint64_t seed)
        : world_(world), generator_(world, seed)
    {
    }

    [[nodiscard]] bool isChunkLoaded(int /*chunkX*/, int /*chunkZ*/) const override
    {
        return true;
    }

    Chunk& getChunk(int chunkX, int chunkZ) override
    {
        return loadChunk(chunkX, chunkZ);
    }

    Chunk& loadChunk(int chunkX, int chunkZ) override
    {
        scratchChunk_.emplace(generator_.loadChunk(this, chunkX, chunkZ));
        scratchChunk_->world = world_;
        return *scratchChunk_;
    }

    void decorate(ChunkSource* source, int chunkX, int chunkZ) override
    {
        generator_.decorate(source, chunkX, chunkZ);
    }

    bool save(bool /*saveEntities*/, client::gui::screen::LoadingDisplay* /*display*/) override
    {
        return true;
    }

    bool tick() override
    {
        return false;
    }

    [[nodiscard]] bool canSave() const override
    {
        return true;
    }

    // Create a fresh generator instance for a worker thread. Same seed →
    // identical terrain; null world since workers never call decorate/tick.
    [[nodiscard]] std::unique_ptr<ChunkSource> cloneForWorker(std::uint64_t seed) const override
    {
        return std::make_unique<OverworldGeneratorChunkSource>(nullptr, seed);
    }

    [[nodiscard]] std::string getDebugInfo() const override
    {
        return "RandomLevelSource";
    }

private:
    World* world_ = nullptr;
    OverworldChunkGenerator generator_;
    std::optional<Chunk> scratchChunk_;
};

} // namespace net::minecraft
