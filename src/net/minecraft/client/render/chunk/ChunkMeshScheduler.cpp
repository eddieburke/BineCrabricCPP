#include "net/minecraft/client/render/chunk/ChunkMeshScheduler.hpp"

#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"

namespace net::minecraft::client::render::chunk {

void ChunkMeshScheduler::enqueue(std::shared_ptr<ChunkMeshJob> job, int priority)
{
    pool_.submit(
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

} // namespace net::minecraft::client::render::chunk
