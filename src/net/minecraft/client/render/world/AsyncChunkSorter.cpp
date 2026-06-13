#include "net/minecraft/client/render/world/AsyncChunkSorter.hpp"

#include <algorithm>
#include <utility>

namespace net::minecraft::client::render::world {

void AsyncChunkSorter::submit(std::vector<Entry> entries, std::uint64_t epoch)
{
    {
        const std::lock_guard lock(mutex_);
        pending_ = Result {std::move(entries), epoch};
    }
    wake_.notify_one();
}

bool AsyncChunkSorter::canSubmit()
{
    const std::lock_guard lock(mutex_);
    return !pending_.has_value() && !sorting_;
}

std::optional<AsyncChunkSorter::Result> AsyncChunkSorter::tryTakeResult()
{
    const std::lock_guard lock(mutex_);
    return std::exchange(result_, std::nullopt);
}

void AsyncChunkSorter::threadLoop(const std::stop_token& stop)
{
    for (;;) {
        Result job;
        {
            std::unique_lock lock(mutex_);
            if (!wake_.wait(lock, stop, [&] { return pending_.has_value(); })) {
                return; // stop requested with no pending work
            }
            job = std::move(*pending_);
            pending_.reset();
            sorting_ = true;
        }

        std::sort(job.entries.begin(), job.entries.end(),
            [](const Entry& a, const Entry& b) { return a.key < b.key; });

        {
            const std::lock_guard lock(mutex_);
            // A newer order simply replaces an unconsumed older one.
            result_ = std::move(job);
            sorting_ = false;
        }
    }
}

} // namespace net::minecraft::client::render::world
