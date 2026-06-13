#include "net/minecraft/client/render/chunk/ChunkMeshScheduler.hpp"

#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"

namespace net::minecraft::client::render::chunk {

void ChunkMeshScheduler::submitTo(
    net::minecraft::util::concurrent::WorkerPool& pool, std::shared_ptr<ChunkMeshJob> job, int priority)
{
    pool.submit(
        [this, job = std::move(job)]() mutable {
            try {
                ChunkBuilder::buildMesh(*job);
            } catch (...) {
                job->failed = true;
            }
            const std::lock_guard lock(mutex_);
            completed_.push_back(std::move(job));
        },
        priority);
}

void ChunkMeshScheduler::enqueue(std::shared_ptr<ChunkMeshJob> job, int priority)
{
    submitTo(pool_, std::move(job), priority);
}

void ChunkMeshScheduler::enqueueNear(std::shared_ptr<ChunkMeshJob> job)
{
    job->nearLane = true;
    submitTo(nearPool_, std::move(job), 0);
}

} // namespace net::minecraft::client::render::chunk
