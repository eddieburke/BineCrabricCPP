#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace net::minecraft::client::render::world {

// Sorts the renderer's chunk draw order off the main thread. The main thread
// submits plain (key, index) pairs — no ChunkBuilder pointers cross the thread
// boundary — and polls for the sorted result a frame or two later, rendering
// with the previous order in the meantime. Submitting while a request is
// pending replaces it (latest camera wins). Results carry the epoch of the
// chunk array they were computed from so a reload() can invalidate stale
// orders.
class AsyncChunkSorter {
public:
    struct Entry {
        float key;
        std::int32_t index;
    };

    struct Result {
        std::vector<Entry> entries;
        std::uint64_t epoch = 0;
    };

    AsyncChunkSorter()
        : thread_([this](const std::stop_token& stop) { threadLoop(stop); })
    {
    }

    // jthread requests stop and joins on destruction; the stop-token-aware
    // wait wakes without an explicit notify. thread_ is the last member so it
    // joins before mutex_/pending_/result_ are destroyed.
    ~AsyncChunkSorter() = default;

    AsyncChunkSorter(const AsyncChunkSorter&) = delete;
    AsyncChunkSorter& operator=(const AsyncChunkSorter&) = delete;

    // Replace any not-yet-started request with this one.
    void submit(std::vector<Entry> entries, std::uint64_t epoch);

    // Cheap main-thread backpressure: if a sort is already queued or running,
    // callers can skip rebuilding a large throwaway input vector this frame.
    [[nodiscard]] bool canSubmit();

    // The most recent finished sort, if one is ready.
    [[nodiscard]] std::optional<Result> tryTakeResult();

private:
    void threadLoop(const std::stop_token& stop);

    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::optional<Result> pending_;
    std::optional<Result> result_;
    bool sorting_ = false;
    std::jthread thread_;
};

} // namespace net::minecraft::client::render::world
