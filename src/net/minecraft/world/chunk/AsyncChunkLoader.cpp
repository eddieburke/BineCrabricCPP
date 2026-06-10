#include "net/minecraft/world/chunk/AsyncChunkLoader.hpp"

#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"

namespace net::minecraft {

AsyncChunkLoader::AsyncChunkLoader(World* world, std::uint64_t seed, ChunkStorage* storage,
    std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory)
    : world_(world),
      seed_(seed),
      storage_(storage),
      genFactory_(std::move(genFactory)),
      pool_(2, "chunk-gen")
{
}

std::unique_ptr<ChunkSource> AsyncChunkLoader::acquireGenerator()
{
    {
        const std::lock_guard lock(mutex_);
        if (!generatorPool_.empty()) {
            auto generator = std::move(generatorPool_.back());
            generatorPool_.pop_back();
            return generator;
        }
    }
    return genFactory_ ? genFactory_(seed_) : nullptr;
}

void AsyncChunkLoader::releaseGenerator(std::unique_ptr<ChunkSource> generator)
{
    if (generator == nullptr) {
        return;
    }
    const std::lock_guard lock(mutex_);
    generatorPool_.push_back(std::move(generator));
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
        // Try storage first (region IO is serialized by RegionIo's mutex).
        // world_ is passed for its immutable dimension/seed reads only.
        if (storage_ != nullptr) {
            try {
                auto loaded = storage_->loadChunk(world_, job->chunkX, job->chunkZ);
                auto chunk = std::make_unique<Chunk>(std::move(loaded));
                if (!chunk->empty && chunk->chunkPosEquals(job->chunkX, job->chunkZ)) {
                    job->result = std::move(chunk);
                    job->storageHit = true;
                }
            } catch (...) {
                // Storage read failed; fall through to generation.
            }
        }

        // Generate if storage didn't produce a chunk.
        if (job->result == nullptr) {
            if (auto gen = acquireGenerator()) {
                try {
                    job->result = std::make_unique<Chunk>(std::move(gen->getChunk(job->chunkX, job->chunkZ)));
                    // fill() touches only chunk-local arrays.
                    job->result->fill();
                } catch (...) {
                    job->failed = true;
                }
                releaseGenerator(std::move(gen));
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

std::vector<std::shared_ptr<AsyncChunkJob>> AsyncChunkLoader::pumpCompleted()
{
    std::vector<std::shared_ptr<AsyncChunkJob>> done;
    {
        const std::lock_guard lock(mutex_);
        done.swap(completed_);
        for (const auto& job : done) {
            inFlight_.erase({job->chunkX, job->chunkZ});
        }
    }
    return done;
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
