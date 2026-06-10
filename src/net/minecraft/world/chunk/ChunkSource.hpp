#pragma once

#include "net/minecraft/client/gui/screen/LoadingDisplay.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace net::minecraft {

class Chunk;

class ChunkSource {
public:
    virtual ~ChunkSource() = default;

    [[nodiscard]] virtual bool isChunkLoaded(int chunkX, int chunkZ) const = 0;
    [[nodiscard]] virtual Chunk& getChunk(int chunkX, int chunkZ) = 0;
    [[nodiscard]] virtual Chunk& loadChunk(int chunkX, int chunkZ) = 0;
    virtual void decorate(ChunkSource* source, int chunkX, int chunkZ) = 0;
    virtual bool save(bool saveEntities, client::gui::screen::LoadingDisplay* display) = 0;
    virtual bool tick() = 0;
    [[nodiscard]] virtual bool canSave() const = 0;
    [[nodiscard]] virtual std::string getDebugInfo() const = 0;

    // Create a fresh, independent generator suited for a worker thread.
    // Must produce terrain identical to the original for the same seed.
    // Default returns nullptr — worker cloning not supported.
    [[nodiscard]] virtual std::unique_ptr<ChunkSource> cloneForWorker(std::uint64_t /*seed*/) const
    {
        return nullptr;
    }
};

} // namespace net::minecraft
