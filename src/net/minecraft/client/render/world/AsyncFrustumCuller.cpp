#include "net/minecraft/client/render/world/AsyncFrustumCuller.hpp"

#include <algorithm>
#include <utility>

namespace net::minecraft::client::render::world {

void AsyncFrustumCuller::submit(FrustumData frustum, double cameraX, double cameraY, double cameraZ,
    std::vector<Entry> entries, std::uint64_t chunkEpoch, std::uint64_t requestEpoch,
    int cullStep, int cullStride)
{
    {
        const std::lock_guard lock(mutex_);
        pending_ = Job {std::move(frustum), cameraX, cameraY, cameraZ, std::move(entries),
            chunkEpoch, requestEpoch, cullStep, std::max(1, cullStride)};
    }
    wake_.notify_one();
}

bool AsyncFrustumCuller::canSubmit()
{
    const std::lock_guard lock(mutex_);
    return !pending_.has_value() && !culling_;
}

std::optional<AsyncFrustumCuller::Result> AsyncFrustumCuller::tryTakeResult()
{
    const std::lock_guard lock(mutex_);
    return std::exchange(result_, std::nullopt);
}

void AsyncFrustumCuller::threadLoop(const std::stop_token& stop)
{
    for (;;) {
        Job job;
        {
            std::unique_lock lock(mutex_);
            if (!wake_.wait(lock, stop, [&] { return pending_.has_value(); })) {
                return;
            }
            job = std::move(*pending_);
            pending_.reset();
            culling_ = true;
        }

        Result result;
        result.chunkEpoch = job.chunkEpoch;
        result.requestEpoch = job.requestEpoch;
        result.visibility.reserve(job.entries.size());

        for (const Entry& entry : job.entries) {
            if (entry.hasNoGeometry) {
                continue;
            }
            const bool shouldRetest = !entry.currentlyVisible
                || (static_cast<int>(entry.index) + job.cullStep) % job.cullStride == 0;
            if (!shouldRetest) {
                continue;
            }

            const Box& box = entry.box;
            const bool visible = job.frustum.intersects(
                box.minX - job.cameraX,
                box.minY - job.cameraY,
                box.minZ - job.cameraZ,
                box.maxX - job.cameraX,
                box.maxY - job.cameraY,
                box.maxZ - job.cameraZ);
            result.visibility.push_back(Visibility {entry.index, visible});
        }

        {
            const std::lock_guard lock(mutex_);
            result_ = std::move(result);
            culling_ = false;
        }
    }
}

} // namespace net::minecraft::client::render::world
