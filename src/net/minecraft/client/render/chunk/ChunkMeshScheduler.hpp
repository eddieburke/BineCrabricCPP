#pragma once

#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/util/concurrent/WorkerPool.hpp"

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace net::minecraft::client::render::chunk {

// Runs ChunkBuilder::buildMesh on a worker pool and hands finished jobs back
// to the main thread. All methods except the worker lambda run on the main
// thread.
class ChunkMeshScheduler {
public:
    ChunkMeshScheduler()
        : pool_(defaultThreadCount(), "chunk-mesh")
    {
    }

    void enqueue(std::shared_ptr<ChunkMeshJob> job, int priority);

    [[nodiscard]] std::vector<std::shared_ptr<ChunkMeshJob>> drainCompleted()
    {
        const std::lock_guard lock(mutex_);
        return std::exchange(completed_, {});
    }

    // Drop queued jobs, wait out in-flight ones, and discard their results.
    // Must be called before the ChunkBuilders referenced by jobs are destroyed.
    void cancelAll()
    {
        pool_.cancelPending();
        pool_.drain();
        const std::lock_guard lock(mutex_);
        completed_.clear();
    }

    [[nodiscard]] bool idle() const
    {
        if (pool_.pendingCount() != 0) {
            return false;
        }
        const std::lock_guard lock(mutex_);
        return completed_.empty();
    }

private:
    [[nodiscard]] static unsigned defaultThreadCount()
    {
        const unsigned hw = std::thread::hardware_concurrency();
        return std::clamp(hw > 2 ? hw - 2 : 1U, 1U, 8U);
    }

    net::minecraft::util::concurrent::WorkerPool pool_;
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<ChunkMeshJob>> completed_;
};

} // namespace net::minecraft::client::render::chunk
