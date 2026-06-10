#pragma once

#include "net/minecraft/util/concurrent/WorkerPool.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace net::minecraft {

class World;
class ChunkSource;

namespace world::gen::chunk {
class OverworldChunkGenerator;
}

struct AsyncChunkJob {
    int chunkX = 0;
    int chunkZ = 0;
    std::unique_ptr<Chunk> result;
    bool storageHit = false;
    bool failed = false;
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
};

class AsyncChunkLoader {
public:
    // chunkGeneratorSeed: used to construct per-worker generator clones.
    // storage: the chunk storage, guarded by its own mutex for worker reads.
    // genFactory: creates a fresh generator for a worker thread from the seed.
    AsyncChunkLoader(std::uint64_t seed, ChunkStorage* storage,
        std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory);

    ~AsyncChunkLoader();

    AsyncChunkLoader(const AsyncChunkLoader&) = delete;
    AsyncChunkLoader& operator=(const AsyncChunkLoader&) = delete;

    // Enqueue a chunk for background load/generation. Returns false when the
    // in-flight cap is hit or the chunk is already in flight or loaded.
    bool request(int chunkX, int chunkZ);

    // Publish all completed chunks into the owning LegacyChunkCache.
    // Called from the main thread (LegacyChunkCache::tick).
    // Returns completed jobs; the caller inserts them into the cache.
    [[nodiscard]] std::vector<std::unique_ptr<AsyncChunkJob>> pumpCompleted();

    // Wait for a specific chunk if it is in flight. Returns nullptr if no job
    // exists for that position (caller should fall back to sync generation).
    [[nodiscard]] std::unique_ptr<Chunk> waitForChunk(int chunkX, int chunkZ);

    // True if a job for this position is queued or running.
    [[nodiscard]] bool isInFlight(int chunkX, int chunkZ) const;

    // Drop queued (not started) jobs; in-flight tasks finish normally.
    void cancelPending();

    // Block until all jobs complete.
    void drain();

    [[nodiscard]] std::size_t inFlightCount() const { return inFlight_.size(); }

private:
    struct PositionHash {
        std::size_t operator()(const std::pair<int, int>& p) const noexcept
        {
            return static_cast<std::size_t>(p.first) * 31ULL + static_cast<std::size_t>(p.second);
        }
    };

    std::uint64_t seed_;
    ChunkStorage* storage_ = nullptr;
    std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory_;
    util::concurrent::WorkerPool pool_;

    mutable std::mutex mutex_;
    std::unordered_map<std::pair<int, int>, std::shared_ptr<AsyncChunkJob>, PositionHash> inFlight_;
    std::vector<std::shared_ptr<AsyncChunkJob>> completed_;
    static constexpr std::size_t kMaxInFlight = 16;
};

} // namespace net::minecraft
