#pragma once
#include <limits>
#include <memory>
#include <vector>

#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/util/concurrent/WorkerHandoff.hpp"

namespace net::minecraft::client::render::chunk {
// Runs ChunkBuilder::buildMesh on a worker pool and hands finished jobs back to
// the main thread. Near-camera edits and the distance backlog share one priority
// queue so edits jump ahead of queued distant work.
class ChunkMeshScheduler {
   public:
    void enqueue(std::shared_ptr<ChunkMeshJob> job, int priority) {
        handoff_.enqueue(
            std::move(job),
            [](ChunkMeshJob& meshJob) {
                try {
                    ChunkBuilder::buildMesh(meshJob);
                } catch (...) {
                    meshJob.failed = true;
                }
            },
            priority);
    }

    void enqueueNear(std::shared_ptr<ChunkMeshJob> job) {
        job->nearLane = true;
        enqueue(std::move(job), std::numeric_limits<int>::min());
    }

    [[nodiscard]] std::vector<std::shared_ptr<ChunkMeshJob>> drainCompleted() {
        return handoff_.drainCompleted();
    }

    void cancelAll() {
        handoff_.cancelAll();
    }

    [[nodiscard]] bool idle() const {
        return handoff_.idle();
    }

    [[nodiscard]] std::size_t pendingJobs() const {
        return handoff_.pendingJobs();
    }

    [[nodiscard]] unsigned workerCount() const noexcept {
        return handoff_.workerCount();
    }

   private:
    net::minecraft::util::concurrent::WorkerHandoff<ChunkMeshJob> handoff_;
};
}  // namespace net::minecraft::client::render::chunk
