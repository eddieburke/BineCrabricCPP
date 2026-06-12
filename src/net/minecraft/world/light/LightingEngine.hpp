#pragma once

#include "net/minecraft/world/light/LightUpdate.hpp"

#include <atomic>
#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace net::minecraft {

class Chunk;

// Dedicated lighting thread. Owns the pending light-update queue and runs the
// Java light-propagation algorithm against registered chunks, off the main
// thread (Phosphor-style async lighting: light values converge eventually and
// every change is reported back as a dirty region the main thread forwards to
// the renderer).
//
// Threading contract:
//  - push / registerChunk / unregisterChunk / drainDirtyRegions / flush /
//    setSkyLightSuppressed are main-thread API. push is also called from the
//    engine thread for propagation spread.
//  - The engine only ever dereferences chunks present in its registry at
//    lookup time, and pins them for the duration of one update.
//    unregisterChunk blocks until the chunk is unpinned, so a chunk may be
//    freed immediately after unregisterChunk returns.
//  - Light nibble arrays are written with byte-CAS (see ChunkNibbleArray), so
//    concurrent main-thread column writes (Chunk::updateHeightMap) cannot lose
//    updates. Block/heightmap reads from the engine are racy-but-benign
//    single-byte reads; any stale value is corrected by the follow-up update
//    the mutating code enqueues.
class LightingEngine {
public:
    struct DirtyRegion {
        int minX, minY, minZ, maxX, maxY, maxZ;
    };

    LightingEngine();
    ~LightingEngine() { stop(); }

    LightingEngine(const LightingEngine&) = delete;
    LightingEngine& operator=(const LightingEngine&) = delete;

    // Queue an update. Same merge-into-5-most-recent coalescing and 1M-entry
    // pressure valve as the old main-thread queue. Caller does world-state
    // checks (chunk loaded, dimension ceiling) before calling.
    void push(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge);

    // Mirror of "dimension has ceiling" — suppresses sky-light spread on the
    // engine thread. Updated from the main thread on every enqueue.
    void setSkyLightSuppressed(bool suppressed) noexcept
    {
        skyLightSuppressed_.store(suppressed, std::memory_order_relaxed);
    }

    void registerChunk(Chunk* chunk);

    // Remove the chunk and wait until the engine is no longer touching it.
    void unregisterChunk(Chunk* chunk);

    [[nodiscard]] std::vector<DirtyRegion> drainDirtyRegions(std::size_t maxRegions);
    [[nodiscard]] bool hasDirtyRegions() const;

    [[nodiscard]] bool busy() const noexcept
    {
        return pendingCount_.load(std::memory_order_relaxed) != 0;
    }

    // Block until the queue (including cascaded spread) is fully processed.
    void flush();

    // Join the thread. Must run before the chunks the engine can see are
    // destroyed (World dtor body runs before its members are destroyed).
    void stop();

private:
    void threadLoop(const std::stop_token& stop);
    void processUpdate(const LightUpdate& update);

    // Registry lookup with per-update pin caching. Returns nullptr for
    // unloaded (or empty) chunks. The result stays valid until pins are
    // released at the end of the current update.
    Chunk* chunkAt(int chunkX, int chunkZ);
    void releasePins();

    [[nodiscard]] static constexpr std::uint64_t chunkKey(int chunkX, int chunkZ) noexcept
    {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32)
            | static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkZ));
    }

    // --- world-view accessors, parity with the World methods the old
    // --- main-thread LightUpdate::updateLight relied on.
    [[nodiscard]] int getBlockId(int x, int y, int z);
    [[nodiscard]] int getBrightness(LightType type, int x, int y, int z);
    void setLight(LightType type, int x, int y, int z, int value);
    [[nodiscard]] bool isTopY(int x, int y, int z);
    // World::updateLight parity — recompute target level and enqueue if changed.
    void spreadLight(LightType type, int x, int y, int z, int level);

    // Queue + lifecycle.
    mutable std::mutex queueMutex_;
    std::condition_variable workCv_;
    std::condition_variable idleCv_;
    std::vector<LightUpdate> queue_;
    bool processing_ = false;
    std::atomic<std::size_t> pendingCount_ {0};
    std::atomic<bool> skyLightSuppressed_ {false};

    // Chunk registry + pinning.
    std::mutex registryMutex_;
    std::condition_variable pinCv_;
    std::unordered_map<std::uint64_t, Chunk*> registry_;
    std::unordered_set<Chunk*> pinned_;
    // Engine-thread-local lookup cache for the current update.
    std::unordered_map<std::uint64_t, Chunk*> pinCache_;

    // Dirty regions awaiting main-thread renderer notification.
    mutable std::mutex outboxMutex_;
    std::vector<DirtyRegion> outbox_;

    std::jthread thread_;
};

} // namespace net::minecraft
