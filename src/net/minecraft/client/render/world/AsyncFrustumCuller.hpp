#pragma once

#include "net/minecraft/client/render/culling/FrustumData.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace net::minecraft::client::render::world {

// Runs chunk frustum tests on a dedicated camera/visibility thread. The render
// thread submits only value data, then applies the last finished result a frame
// later; no ChunkBuilder pointers or GL state cross the thread boundary.
class AsyncFrustumCuller {
public:
    struct Entry {
        net::minecraft::Box box {};
        std::int32_t index = 0;
        bool currentlyVisible = false;
        bool hasNoGeometry = false;
    };

    struct Visibility {
        std::int32_t index = 0;
        bool visible = false;
    };

    struct Result {
        std::vector<Visibility> visibility;
        std::uint64_t chunkEpoch = 0;
        std::uint64_t requestEpoch = 0;
    };

    AsyncFrustumCuller()
        : thread_([this](const std::stop_token& stop) { threadLoop(stop); })
    {
    }

    ~AsyncFrustumCuller() = default;

    AsyncFrustumCuller(const AsyncFrustumCuller&) = delete;
    AsyncFrustumCuller& operator=(const AsyncFrustumCuller&) = delete;

    void submit(FrustumData frustum, double cameraX, double cameraY, double cameraZ,
        std::vector<Entry> entries, std::uint64_t chunkEpoch, std::uint64_t requestEpoch,
        int cullStep, int cullStride);

    [[nodiscard]] bool canSubmit();
    [[nodiscard]] std::optional<Result> tryTakeResult();

private:
    struct Job {
        FrustumData frustum {};
        double cameraX = 0.0;
        double cameraY = 0.0;
        double cameraZ = 0.0;
        std::vector<Entry> entries;
        std::uint64_t chunkEpoch = 0;
        std::uint64_t requestEpoch = 0;
        int cullStep = 0;
        int cullStride = 1;
    };

    void threadLoop(const std::stop_token& stop);

    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::optional<Job> pending_;
    std::optional<Result> result_;
    bool culling_ = false;
    std::jthread thread_;
};

} // namespace net::minecraft::client::render::world
