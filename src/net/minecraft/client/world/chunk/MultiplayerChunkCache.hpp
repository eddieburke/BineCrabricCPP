#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::world::chunk {

class MultiplayerChunkCache : public ChunkSource {
public:
    explicit MultiplayerChunkCache(World* world)
        : empty_(world, 0, 0),
          world_(world)
    {
    }

    [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override
    {
        return chunksByPos_.contains(ChunkPos{chunkX, chunkZ});
    }

    void unloadChunk(int chunkX, int chunkZ)
    {
        const ChunkPos pos{chunkX, chunkZ};
        auto it = chunksByPos_.find(pos);
        if (it == chunksByPos_.end()) {
            return;
        }
        Chunk* chunk = it->second.get();
        if (chunk != nullptr && !chunk->isEmpty()) {
            chunk->unload();
        }
        chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
        chunksByPos_.erase(it);
    }

    Chunk& loadChunk(int chunkX, int chunkZ) override
    {
        auto chunk = std::make_unique<Chunk>(world_, chunkX, chunkZ);
        std::fill(chunk->skyLight.bytes.begin(), chunk->skyLight.bytes.end(), static_cast<std::uint8_t>(0xFF));
        chunk->loaded = true;
        Chunk* raw = chunk.get();
        chunksByPos_[ChunkPos{chunkX, chunkZ}] = std::move(chunk);
        chunks_.push_back(raw);
        return *raw;
    }

    [[nodiscard]] Chunk& getChunk(int chunkX, int chunkZ) override
    {
        auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
        if (it == chunksByPos_.end() || it->second == nullptr) {
            return empty_;
        }
        return *it->second;
    }

    bool save(bool /*saveEntities*/, client::gui::screen::LoadingDisplay* /*display*/) override { return true; }

    bool tick() override { return false; }

    [[nodiscard]] bool canSave() const override { return false; }

    void decorate(ChunkSource* /*source*/, int /*chunkX*/, int /*chunkZ*/) override {}

    [[nodiscard]] std::string getDebugInfo() const override
    {
        std::ostringstream out;
        out << "MultiplayerChunkCache: " << chunksByPos_.size();
        return out.str();
    }

private:
    EmptyChunk empty_;
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> chunksByPos_;
    std::vector<Chunk*> chunks_;
    World* world_ = nullptr;
};

} // namespace net::minecraft::client::world::chunk
