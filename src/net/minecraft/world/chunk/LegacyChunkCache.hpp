#pragma once

#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/AsyncChunkLoader.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace net::minecraft {

class World;

// Faithful port of net.minecraft.world.chunk.LegacyChunkCache (beta 1.7.3 client).
class LegacyChunkCache : public ChunkSource {
public:
    LegacyChunkCache(World* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator)
        : empty_(world, 0, 0),
          generator_(generator),
          storage_(std::move(storage)),
          world_(world)
    {
        chunks_.fill(nullptr);
    }

    void setSpawnPoint(int chunkX, int chunkZ)
    {
        spawnChunkX_ = chunkX;
        spawnChunkZ_ = chunkZ;
    }

    // Wire async chunk loading with the given seed and generator factory.
    // Must be called once after construction before tick().
    void initAsync(std::uint64_t seed, std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory)
    {
        asyncLoader_ = std::make_unique<AsyncChunkLoader>(world_, seed, storage_.get(), std::move(genFactory));
    }

    // Ask the async loader to background-load chunks in the active window,
    // nearest first. Called each tick after setSpawnPoint.
    void prefetch(int centerChunkX, int centerChunkZ)
    {
        if (asyncLoader_ == nullptr) {
            return;
        }
        constexpr int window = 15;
        // Spiral outward from center so near chunks load first.
        for (int r = 0; r <= window && asyncLoader_->inFlightCount() < 8; ++r) {
            for (int dx = -r; dx <= r; ++dx) {
                for (int dz = -r; dz <= r; ++dz) {
                    if (r > 0 && std::abs(dx) != r && std::abs(dz) != r) {
                        continue;  // only the outermost ring at radius r
                    }
                    const int cx = centerChunkX + dx;
                    const int cz = centerChunkZ + dz;
                    if (!isSpawnChunk(cx, cz)) {
                        continue;
                    }
                    if (isChunkLoaded(cx, cz)) {
                        continue;
                    }
                    asyncLoader_->request(cx, cz);
                }
            }
        }
    }

    // Publish async-loaded chunks into the cache and trigger decoration.
    void pumpAsyncChunks()
    {
        if (asyncLoader_ == nullptr) {
            return;
        }
        auto completed = asyncLoader_->pumpCompleted();
        for (auto& job : completed) {
            if (job->failed || job->result == nullptr) {
                continue;
            }
            const int cx = job->chunkX;
            const int cz = job->chunkZ;
            const int index = (cx & 31) + (cz & 31) * 32;

            // Evict old occupant if any (same as getChunk).
            if (chunks_[static_cast<std::size_t>(index)] != nullptr) {
                chunks_[static_cast<std::size_t>(index)]->unload();
                saveChunk(*chunks_[static_cast<std::size_t>(index)]);
                saveEntitiesForChunk(*chunks_[static_cast<std::size_t>(index)]);
            }

            Chunk* chunk = job->result.get();
            chunk->world = world_;
            chunk->populateBlockLight();
            chunk->load();
            chunks_[static_cast<std::size_t>(index)] = chunk;
            ownedChunks_.push_back(std::move(job->result));

            // Decoration — same 4-way neighbor checks as getChunk.
            if (!chunk->terrainPopulated && isChunkLoaded(cx + 1, cz + 1) && isChunkLoaded(cx, cz + 1)
                && isChunkLoaded(cx + 1, cz)) {
                decorate(this, cx, cz);
            }
            if (isChunkLoaded(cx - 1, cz) && !getChunk(cx - 1, cz).terrainPopulated
                && isChunkLoaded(cx - 1, cz + 1) && isChunkLoaded(cx, cz + 1) && isChunkLoaded(cx - 1, cz)) {
                decorate(this, cx - 1, cz);
            }
            if (isChunkLoaded(cx, cz - 1) && !getChunk(cx, cz - 1).terrainPopulated
                && isChunkLoaded(cx + 1, cz - 1) && isChunkLoaded(cx, cz - 1) && isChunkLoaded(cx + 1, cz)) {
                decorate(this, cx, cz - 1);
            }
            if (isChunkLoaded(cx - 1, cz - 1) && !getChunk(cx - 1, cz - 1).terrainPopulated
                && isChunkLoaded(cx - 1, cz - 1) && isChunkLoaded(cx, cz - 1) && isChunkLoaded(cx - 1, cz)) {
                decorate(this, cx - 1, cz - 1);
            }
        }
    }

    void populateReadyChunks()
    {
        for (Chunk* chunkPtr : chunks_) {
            if (chunkPtr == nullptr || chunkPtr == &empty_ || chunkPtr->terrainPopulated) {
                continue;
            }
            const int chunkX = chunkPtr->x;
            const int chunkZ = chunkPtr->z;
            if (isChunkLoaded(chunkX + 1, chunkZ + 1) && isChunkLoaded(chunkX, chunkZ + 1) && isChunkLoaded(chunkX + 1, chunkZ)) {
                decorate(this, chunkX, chunkZ);
            }
        }
    }

    [[nodiscard]] bool isSpawnChunk(int chunkX, int chunkZ) const
    {
        constexpr int radius = 15;
        return chunkX >= spawnChunkX_ - radius
            && chunkZ >= spawnChunkZ_ - radius
            && chunkX <= spawnChunkX_ + radius
            && chunkZ <= spawnChunkZ_ + radius;
    }

    [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override
    {
        if (!isSpawnChunk(chunkX, chunkZ)) {
            return false;
        }
        if (cachedChunk_ != nullptr && cachedChunkX == chunkX && cachedChunkZ == chunkZ) {
            return true;
        }
        const int index = (chunkX & 31) + (chunkZ & 31) * 32;
        return chunks_[static_cast<std::size_t>(index)] != nullptr
            && (chunks_[static_cast<std::size_t>(index)] == &empty_
                || chunks_[static_cast<std::size_t>(index)]->chunkPosEquals(chunkX, chunkZ));
    }

    Chunk& loadChunk(int chunkX, int chunkZ) override
    {
        return getChunk(chunkX, chunkZ);
    }

    Chunk& getChunk(int chunkX, int chunkZ) override
    {
        if (cachedChunk_ != nullptr && cachedChunkX == chunkX && cachedChunkZ == chunkZ) {
            return *cachedChunk_;
        }
        if (world_ != nullptr && !world_->eventProcessingEnabled && !isSpawnChunk(chunkX, chunkZ)) {
            return empty_;
        }

        const int index = (chunkX & 31) + (chunkZ & 31) * 32;
        if (!isChunkLoaded(chunkX, chunkZ)) {
            // If async loader has this chunk in flight, wait for it.
            if (asyncLoader_ != nullptr && asyncLoader_->isInFlight(chunkX, chunkZ)) {
                if (auto asyncChunk = asyncLoader_->waitForChunk(chunkX, chunkZ)) {
                    if (chunks_[static_cast<std::size_t>(index)] != nullptr) {
                        chunks_[static_cast<std::size_t>(index)]->unload();
                        saveChunk(*chunks_[static_cast<std::size_t>(index)]);
                        saveEntitiesForChunk(*chunks_[static_cast<std::size_t>(index)]);
                    }
                    Chunk* chunk = asyncChunk.get();
                    chunk->world = world_;
                    chunk->populateBlockLight();
                    chunk->load();
                    chunks_[static_cast<std::size_t>(index)] = chunk;
                    ownedChunks_.push_back(std::move(asyncChunk));
                    // Trigger neighbor decoration (same as below).
                    triggerNeighborDecoration(chunkX, chunkZ);
                    cachedChunkX = chunkX;
                    cachedChunkZ = chunkZ;
                    cachedChunk_ = chunks_[static_cast<std::size_t>(index)];
                    return *cachedChunk_;
                }
            }

            if (chunks_[static_cast<std::size_t>(index)] != nullptr) {
                chunks_[static_cast<std::size_t>(index)]->unload();
                saveChunk(*chunks_[static_cast<std::size_t>(index)]);
                saveEntitiesForChunk(*chunks_[static_cast<std::size_t>(index)]);
            }

            Chunk* chunk = loadChunkFromStorage(chunkX, chunkZ);
            if (chunk == nullptr) {
                if (generator_ == nullptr) {
                    chunk = &empty_;
                } else {
                    chunk = loadChunkFromGenerator(chunkX, chunkZ);
                    chunk->fill();
                }
            }

            chunks_[static_cast<std::size_t>(index)] = chunk;
            if (chunk != nullptr) {
                chunk->populateBlockLight();
                chunk->load();
            }

            triggerNeighborDecoration(chunkX, chunkZ);
        }

        cachedChunkX = chunkX;
        cachedChunkZ = chunkZ;
        cachedChunk_ = chunks_[static_cast<std::size_t>(index)];
        return *cachedChunk_;
    }

    void decorate(ChunkSource* source, int chunkX, int chunkZ) override
    {
        Chunk& chunk = getChunk(chunkX, chunkZ);
        if (chunk.terrainPopulated) {
            return;
        }
        chunk.terrainPopulated = true;
        if (generator_ != nullptr) {
            generator_->decorate(source, chunkX, chunkZ);
            chunk.markDirty();
        }
    }

    bool save(bool saveEntities, client::gui::screen::LoadingDisplay* display) override
    {
        int total = 0;
        if (display != nullptr) {
            for (Chunk* chunk : chunks_) {
                if (chunk != nullptr && chunk->shouldSave(saveEntities)) {
                    ++total;
                }
            }
        }

        int saved = 0;
        int index = 0;
        for (Chunk* chunk : chunks_) {
            if (chunk == nullptr) {
                continue;
            }
            if (saveEntities && !chunk->empty) {
                saveEntitiesForChunk(*chunk);
            }
            if (!chunk->shouldSave(saveEntities)) {
                continue;
            }
            saveChunk(*chunk);
            chunk->dirty = false;
            if (!saveEntities && ++saved == 2) {
                return false;
            }
            if (display != nullptr && total > 0 && ++index % 10 == 0) {
                display->progressStagePercentage(index * 100 / total);
            }
        }
        if (saveEntities && storage_ != nullptr) {
            storage_->flush();
        }
        return true;
    }

    bool tick() override
    {
        pumpAsyncChunks();
        if (storage_ != nullptr) {
            storage_->tick();
        }
        return generator_ != nullptr && generator_->tick();
    }

    [[nodiscard]] bool canSave() const override
    {
        return true;
    }

    [[nodiscard]] std::string getDebugInfo() const override
    {
        return "ChunkCache: " + std::to_string(chunks_.size());
    }

    int cachedChunkX = 0;
    int cachedChunkZ = 0;

private:
    Chunk* loadChunkFromStorage(int chunkX, int chunkZ)
    {
        if (storage_ == nullptr) {
            return &empty_;
        }
        try {
            std::unique_ptr<Chunk> loaded = std::make_unique<Chunk>(std::move(storage_->loadChunk(world_, chunkX, chunkZ)));
            if (loaded->empty || !loaded->chunkPosEquals(chunkX, chunkZ)) {
                return nullptr;
            }
            loaded->world = world_;
            Chunk* stored = loaded.get();
            ownedChunks_.push_back(std::move(loaded));
            return stored;
        } catch (...) {
            return &empty_;
        }
    }

    Chunk* loadChunkFromGenerator(int chunkX, int chunkZ)
    {
        std::unique_ptr<Chunk> generated = std::make_unique<Chunk>(std::move(generator_->getChunk(chunkX, chunkZ)));
        generated->world = world_;
        Chunk* stored = generated.get();
        ownedChunks_.push_back(std::move(generated));
        return stored;
    }

    void saveEntitiesForChunk(Chunk& chunk)
    {
        if (storage_ == nullptr) {
            return;
        }
        try {
            storage_->saveEntities(world_, chunk);
        } catch (...) {
        }
    }

    void saveChunk(Chunk& chunk)
    {
        if (storage_ == nullptr) {
            return;
        }
        try {
            storage_->saveChunk(world_, chunk);
        } catch (...) {
        }
    }

    void triggerNeighborDecoration(int chunkX, int chunkZ)
    {
        if (isChunkLoaded(chunkX + 1, chunkZ + 1) && isChunkLoaded(chunkX, chunkZ + 1)
            && isChunkLoaded(chunkX + 1, chunkZ)) {
            decorate(this, chunkX, chunkZ);
        }
        if (isChunkLoaded(chunkX - 1, chunkZ) && !getChunk(chunkX - 1, chunkZ).terrainPopulated
            && isChunkLoaded(chunkX - 1, chunkZ + 1) && isChunkLoaded(chunkX, chunkZ + 1)
            && isChunkLoaded(chunkX - 1, chunkZ)) {
            decorate(this, chunkX - 1, chunkZ);
        }
        if (isChunkLoaded(chunkX, chunkZ - 1) && !getChunk(chunkX, chunkZ - 1).terrainPopulated
            && isChunkLoaded(chunkX + 1, chunkZ - 1) && isChunkLoaded(chunkX, chunkZ - 1)
            && isChunkLoaded(chunkX + 1, chunkZ)) {
            decorate(this, chunkX, chunkZ - 1);
        }
        if (isChunkLoaded(chunkX - 1, chunkZ - 1) && !getChunk(chunkX - 1, chunkZ - 1).terrainPopulated
            && isChunkLoaded(chunkX - 1, chunkZ - 1) && isChunkLoaded(chunkX, chunkZ - 1)
            && isChunkLoaded(chunkX - 1, chunkZ)) {
            decorate(this, chunkX - 1, chunkZ - 1);
        }
    }

    EmptyChunk empty_;
    ChunkSource* generator_ = nullptr;
    std::unique_ptr<ChunkStorage> storage_ {};
    std::unique_ptr<AsyncChunkLoader> asyncLoader_ {};
    std::array<Chunk*, 32 * 32> chunks_ {};
    World* world_ = nullptr;
    Chunk* cachedChunk_ = nullptr;
    int spawnChunkX_ = 0;
    int spawnChunkZ_ = 0;
    std::vector<std::unique_ptr<Chunk>> ownedChunks_ {};
};

} // namespace net::minecraft
