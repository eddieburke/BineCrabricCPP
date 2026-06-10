#include "net/minecraft/world/chunk/AsyncChunkLoader.hpp"

#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"

namespace net::minecraft {

AsyncChunkLoader::AsyncChunkLoader(std::uint64_t seed, ChunkStorage* storage,
    std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory)
    : seed_(seed),
      storage_(storage),
      genFactory_(std::move(genFactory)),
      pool_(2, "chunk-gen")
{
}

AsyncChunkLoader::~AsyncChunkLoader()
{
    pool_.cancelPending();
    pool_.drain();
}

bool AsyncChunkLoader::request(int chunkX, int chunkZ)
{
    const std::pair key {chunkX, chunkZ};
    {
        const std::lock_guard lock(mutex_);
        if (inFlight_.contains(key)) {
            return false;
        }
        if (inFlight_.size() >= kMaxInFlight) {
            return false;
        }
    }

    auto job = std::make_shared<AsyncChunkJob>();
    job->chunkX = chunkX;
    job->chunkZ = chunkZ;

    {
        const std::lock_guard lock(mutex_);
        inFlight_[key] = job;
    }

    pool_.submit([this, job, key]() {
        // Try storage first (storage_ has its own mutex)
        if (storage_ != nullptr) {
            try {
                auto loaded = storage_->loadChunk(nullptr, job->chunkX, job->chunkZ);
                auto chunk = std::make_unique<Chunk>(std::move(loaded));
                if (!chunk->empty && chunk->chunkPosEquals(job->chunkX, job->chunkZ)) {
                    job->result = std::move(chunk);
                    job->storageHit = true;
                }
            } catch (...) {
                // Storage read failed; fall through to generation.
            }
        }

        // Generate if storage didn't produce a chunk
        if (job->result == nullptr && genFactory_) {
            try {
                auto gen = genFactory_(seed_);
                job->result = std::make_unique<Chunk>(std::move(gen->getChunk(job->chunkX, job->chunkZ)));
                // fill() touches only chunk-local arrays
                job->result->fill();
            } catch (...) {
                job->failed = true;
            }
        }

        if (job->result == nullptr) {
            job->failed = true;
        }

        {
            std::lock_guard lock(job->mutex);
            job->done = true;
        }
        job->cv.notify_all();

        {
            const std::lock_guard lock(mutex_);
            completed_.push_back(job);
        }
    }, 0);

    return true;
}

std::vector<std::unique_ptr<AsyncChunkJob>> AsyncChunkLoader::pumpCompleted()
{
    std::vector<std::shared_ptr<AsyncChunkJob>> done;
    {
        const std::lock_guard lock(mutex_);
        done.swap(completed_);
    }

    std::vector<std::unique_ptr<AsyncChunkJob>> result;
    result.reserve(done.size());

    for (auto& job : done) {
        const std::pair key {job->chunkX, job->chunkZ};
        {
            const std::lock_guard lock(mutex_);
            inFlight_.erase(key);
        }
        // Transfer ownership out of the shared_ptr
        auto owned = std::make_unique<AsyncChunkJob>();
        owned->chunkX = job->chunkX;
        owned->chunkZ = job->chunkZ;
        owned->result = std::move(job->result);
        owned->storageHit = job->storageHit;
        owned->failed = job->failed;
        result.push_back(std::move(owned));
    }

    return result;
}

std::unique_ptr<Chunk> AsyncChunkLoader::waitForChunk(int chunkX, int chunkZ)
{
    std::shared_ptr<AsyncChunkJob> job;
    {
        const std::lock_guard lock(mutex_);
        auto it = inFlight_.find({chunkX, chunkZ});
        if (it == inFlight_.end()) {
            return nullptr;
        }
        job = it->second;
    }

    std::unique_lock lock(job->mutex);
    job->cv.wait(lock, [&] { return job->done; });

    auto result = std::move(job->result);
    {
        const std::lock_guard lock2(mutex_);
        inFlight_.erase({chunkX, chunkZ});
    }
    return result;
}

bool AsyncChunkLoader::isInFlight(int chunkX, int chunkZ) const
{
    const std::lock_guard lock(mutex_);
    return inFlight_.contains({chunkX, chunkZ});
}

void AsyncChunkLoader::cancelPending()
{
    pool_.cancelPending();
}

void AsyncChunkLoader::drain()
{
    pool_.drain();
}

} // namespace net::minecraft
