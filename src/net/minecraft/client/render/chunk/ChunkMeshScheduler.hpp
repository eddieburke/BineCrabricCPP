#pragma once

#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/util/concurrent/WorkerPool.hpp"

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace net::minecraft::client::render::chunk {

// Runs ChunkBuilder::buildMesh on a worker pool and hands finished jobs back to
// the main thread. All methods except the worker lambda run on the main thread.
//
// The shared pool_ handles the distance backlog, fed in near->far order at a
// per-frame budget. A separate single-thread nearPool_ handles sections the
// player just edited next to the camera: at huge render distance the shared pool
// is permanently saturated by the distant backlog, so without a dedicated worker
// an edit's remesh would wait many frames behind it. The near worker is almost
// always idle, so near edits remesh the next frame.
class ChunkMeshScheduler {
public:
    ChunkMeshScheduler()
        : pool_(defaultThreadCount(), "chunk-mesh")
        , nearPool_(1, "chunk-mesh-near")
    {
    }

    ~ChunkMeshScheduler()
    {
        // Workers push into completed_ under mutex_; both are destroyed before
        // the pools join, so the pools must be idle before member destruction.
        pool_.cancelPending();
        nearPool_.cancelPending();
        pool_.drain();
        nearPool_.drain();
    }

    void enqueue(std::shared_ptr<ChunkMeshJob> job, int priority);

    // Dedicated single worker for sections next to the camera (block edits), so
    // their rebuilds never queue behind the distance backlog on pool_.
    void enqueueNear(std::shared_ptr<ChunkMeshJob> job);

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
        nearPool_.cancelPending();
        pool_.drain();
        nearPool_.drain();
        const std::lock_guard lock(mutex_);
        completed_.clear();
    }

    [[nodiscard]] bool idle() const
    {
        if (pool_.pendingCount() != 0 || nearPool_.pendingCount() != 0) {
            return false;
        }
        const std::lock_guard lock(mutex_);
        return completed_.empty();
    }

    // Jobs queued or being built on the shared pool (excludes the near lane and
    // finished jobs awaiting drain).
    [[nodiscard]] std::size_t pendingJobs() const
    {
        return pool_.pendingCount();
    }

    [[nodiscard]] std::size_t nearPendingJobs() const
    {
        return nearPool_.pendingCount();
    }

    [[nodiscard]] unsigned workerCount() const noexcept
    {
        return pool_.threadCount();
    }

private:
    [[nodiscard]] static unsigned defaultThreadCount()
    {
        const unsigned hw = std::thread::hardware_concurrency();
        return std::clamp(hw > 2 ? hw - 2 : 1U, 1U, 8U);
    }

    void submitTo(net::minecraft::util::concurrent::WorkerPool& pool, std::shared_ptr<ChunkMeshJob> job, int priority);

    net::minecraft::util::concurrent::WorkerPool pool_;
    net::minecraft::util::concurrent::WorkerPool nearPool_;
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<ChunkMeshJob>> completed_;
};

} // namespace net::minecraft::client::render::chunk
