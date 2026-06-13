#include "net/minecraft/world/light/LightingEngine.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

#include <algorithm>
#include <utility>

namespace net::minecraft {

namespace {

    [[nodiscard]] constexpr int defaultLight(LightType type) noexcept
    {
        return type == LightType::Sky ? 15 : 0;
    }

} // namespace

bool LightingEngine::Box::expand(int x0, int y0, int z0, int x1, int y1, int z1)
{
    if (x0 >= minX && y0 >= minY && z0 >= minZ && x1 <= maxX && y1 <= maxY && z1 <= maxZ) {
        return true;
    }
    constexpr int margin = 1;
    if (x0 < minX - margin || y0 < minY - margin || z0 < minZ - margin || x1 > maxX + margin || y1 > maxY + margin
        || z1 > maxZ + margin) {
        return false;
    }
    const int nx0 = std::min(minX, x0);
    const int ny0 = std::min(minY, y0);
    const int nz0 = std::min(minZ, z0);
    const int nx1 = std::max(maxX, x1);
    const int ny1 = std::max(maxY, y1);
    const int nz1 = std::max(maxZ, z1);
    const int oldVol = (maxX - minX) * (maxY - minY) * (maxZ - minZ);
    const int newVol = (nx1 - nx0) * (ny1 - ny0) * (nz1 - nz0);
    if (newVol - oldVol > 2) {
        return false;
    }
    minX = nx0;
    minY = ny0;
    minZ = nz0;
    maxX = nx1;
    maxY = ny1;
    maxZ = nz1;
    return true;
}

LightingEngine::LightingEngine()
    : thread_([this](const std::stop_token& stop) { threadLoop(stop); })
{
}

void LightingEngine::push(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge)
{
    bool notify = false;
    {
        const std::lock_guard lock(queueMutex_);
        if (merge) {
            const std::size_t n = std::min<std::size_t>(queue_.size(), 5);
            for (std::size_t i = 0; i < n; ++i) {
                Box& existing = queue_[queue_.size() - i - 1];
                if (existing.type == type && existing.expand(minX, minY, minZ, maxX, maxY, maxZ)) {
                    return;
                }
            }
        }
        queue_.push_back(Box {type, minX, minY, minZ, maxX, maxY, maxZ});
        if (queue_.size() > 1000000) {
            queue_.clear();
        }
        pendingCount_.store(queue_.size() + (working_ ? 1U : 0U), std::memory_order_relaxed);
        notify = !queue_.empty();
    }
    if (notify) {
        workCv_.notify_one();
    }
}

void LightingEngine::registerChunk(Chunk* chunk)
{
    if (chunk == nullptr || chunk->isEmpty()) {
        return;
    }
    const std::lock_guard lock(registryMutex_);
    registry_[chunkKey(chunk->x, chunk->z)] = chunk;
}

void LightingEngine::unregisterChunk(Chunk* chunk)
{
    if (chunk == nullptr) {
        return;
    }
    std::unique_lock lock(registryMutex_);
    const auto it = registry_.find(chunkKey(chunk->x, chunk->z));
    if (it != registry_.end() && it->second == chunk) {
        registry_.erase(it);
    }
    pinCv_.wait(lock, [&] { return pinned_.find(chunk) == pinned_.end(); });
}

std::vector<LightingEngine::DirtyRegion> LightingEngine::drainDirtyRegions(std::size_t maxRegions)
{
    const std::lock_guard lock(outboxMutex_);
    if (maxRegions == 0 || outbox_.empty()) {
        return {};
    }
    if (maxRegions >= outbox_.size()) {
        return std::exchange(outbox_, {});
    }
    const std::size_t count = std::min(maxRegions, outbox_.size());
    auto first = outbox_.end() - static_cast<std::ptrdiff_t>(count);
    std::vector<DirtyRegion> regions(first, outbox_.end());
    outbox_.erase(first, outbox_.end());
    return regions;
}

bool LightingEngine::hasDirtyRegions() const
{
    const std::lock_guard lock(outboxMutex_);
    return !outbox_.empty();
}

void LightingEngine::flush()
{
    std::unique_lock lock(queueMutex_);
    idleCv_.wait(lock, [this] { return queue_.empty() && !working_; });
}

void LightingEngine::stop()
{
    if (!thread_.joinable()) {
        return;
    }
    thread_.request_stop();
    workCv_.notify_all();
    thread_.join();
    const std::lock_guard lock(queueMutex_);
    queue_.clear();
    pendingCount_.store(0, std::memory_order_relaxed);
}

void LightingEngine::threadLoop(const std::stop_token& stop)
{
    for (;;) {
        Box box {LightType::Block, 0, 0, 0, 0, 0, 0};
        {
            std::unique_lock lock(queueMutex_);
            workCv_.wait(lock, [&] { return stop.stop_requested() || !queue_.empty(); });
            if (stop.stop_requested()) {
                return;
            }
            box = queue_.back();
            queue_.pop_back();
            working_ = true;
            pendingCount_.store(queue_.size() + 1, std::memory_order_relaxed);
        }

        runUpdate(box);
        releasePins();

        {
            const std::lock_guard lock(queueMutex_);
            working_ = false;
            pendingCount_.store(queue_.size(), std::memory_order_relaxed);
            if (queue_.empty()) {
                idleCv_.notify_all();
            }
        }
    }
}

Chunk* LightingEngine::chunkAt(int chunkX, int chunkZ)
{
    const std::uint64_t key = chunkKey(chunkX, chunkZ);
    if (const auto it = pinCache_.find(key); it != pinCache_.end()) {
        return it->second;
    }
    Chunk* chunk = nullptr;
    {
        const std::lock_guard lock(registryMutex_);
        if (const auto reg = registry_.find(key); reg != registry_.end()) {
            chunk = reg->second;
            pinned_.insert(chunk);
        }
    }
    pinCache_.emplace(key, chunk);
    return chunk;
}

void LightingEngine::releasePins()
{
    {
        const std::lock_guard lock(registryMutex_);
        pinned_.clear();
    }
    pinCv_.notify_all();
    pinCache_.clear();
}

int LightingEngine::blockId(int x, int y, int z)
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0 || y >= Chunk::height) {
        return 0;
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    return chunk != nullptr ? chunk->getBlockId(x & 15, y, z & 15) : 0;
}

int LightingEngine::brightness(LightType type, int x, int y, int z)
{
    y = std::clamp(y, 0, Chunk::height - 1);
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return defaultLight(type);
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    return chunk != nullptr ? chunk->getLight(type, x & 15, y, z & 15) : 0;
}

void LightingEngine::setBrightness(LightType type, int x, int y, int z, int value)
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0 || y >= Chunk::height) {
        return;
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    if (chunk == nullptr || chunk->getLight(type, x & 15, y, z & 15) == value) {
        return;
    }
    chunk->setLight(type, x & 15, y, z & 15, value);
}

bool LightingEngine::topY(int x, int y, int z)
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0) {
        return false;
    }
    if (y >= Chunk::height) {
        return true;
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    return chunk != nullptr && chunk->isAboveMaxHeight(x & 15, y, z & 15);
}

void LightingEngine::enqueueSpread(LightType type, int x, int y, int z, int level)
{
    if (type == LightType::Sky && skyLightSuppressed_.load(std::memory_order_relaxed)) {
        return;
    }
    if (y < 0 || y >= Chunk::height || chunkAt(x >> 4, z >> 4) == nullptr) {
        return;
    }
    if (type == LightType::Sky && topY(x, y, z)) {
        level = 15;
    } else if (type == LightType::Block) {
        const int lum = block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(blockId(x, y, z))];
        if (lum > level) {
            level = lum;
        }
    }
    if (brightness(type, x, y, z) != level) {
        push(type, x, y, z, x, y, z, true);
    }
}

void LightingEngine::runUpdate(const Box& update)
{
    using block::Block;

    const LightType lightType = update.type;
    int minY = std::max(0, update.minY);
    int maxY = std::min(Chunk::height - 1, update.maxY);
    const int volume = (update.maxX - update.minX + 1) * (maxY - minY + 1) * (update.maxZ - update.minZ + 1);
    if (volume > 32768) {
        return;
    }

    bool changed = false;
    int lastCx = 0;
    int lastCz = 0;
    bool lastLoaded = false;

    for (int x = update.minX; x <= update.maxX; ++x) {
        for (int z = update.minZ; z <= update.maxZ; ++z) {
            bool loaded = false;
            if (lastLoaded && (x >> 4) == lastCx && (z >> 4) == lastCz) {
                loaded = true;
            } else {
                loaded = true;
                for (int cx = (x - 1) >> 4; cx <= (x + 1) >> 4 && loaded; ++cx) {
                    for (int cz = (z - 1) >> 4; cz <= (z + 1) >> 4; ++cz) {
                        if (chunkAt(cx, cz) == nullptr) {
                            loaded = false;
                            break;
                        }
                    }
                }
                lastCx = x >> 4;
                lastCz = z >> 4;
                lastLoaded = loaded;
            }
            if (!loaded) {
                continue;
            }

            for (int y = minY; y <= maxY; ++y) {
                const int current = brightness(lightType, x, y, z);
                const int block = blockId(x, y, z);
                int opacity = Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(block)];
                if (opacity == 0) {
                    opacity = 1;
                }

                int emission = 0;
                if (lightType == LightType::Sky) {
                    if (topY(x, y, z)) {
                        emission = 15;
                    }
                } else {
                    emission = Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(block)];
                }

                int newLight = 0;
                if (opacity < 15 || emission != 0) {
                    int best = brightness(lightType, x - 1, y, z);
                    best = std::max(best, brightness(lightType, x + 1, y, z));
                    best = std::max(best, brightness(lightType, x, y - 1, z));
                    best = std::max(best, brightness(lightType, x, y + 1, z));
                    best = std::max(best, brightness(lightType, x, y, z - 1));
                    best = std::max(best, brightness(lightType, x, y, z + 1));
                    best = std::max(0, best - opacity);
                    newLight = std::max(best, emission);
                }

                if (current == newLight) {
                    continue;
                }
                setBrightness(lightType, x, y, z, newLight);
                changed = true;

                const int spread = std::max(0, newLight - 1);
                enqueueSpread(lightType, x - 1, y, z, spread);
                enqueueSpread(lightType, x, y - 1, z, spread);
                enqueueSpread(lightType, x, y, z - 1, spread);
                if (x + 1 >= update.maxX) {
                    enqueueSpread(lightType, x + 1, y, z, spread);
                }
                if (y + 1 >= update.maxY) {
                    enqueueSpread(lightType, x, y + 1, z, spread);
                }
                if (z + 1 >= update.maxZ) {
                    enqueueSpread(lightType, x, y, z + 1, spread);
                }
            }
        }
    }

    if (!changed) {
        return;
    }
    const std::lock_guard lock(outboxMutex_);
    constexpr std::size_t maxOutbox = 4096;
    if (outbox_.size() >= maxOutbox) {
        DirtyRegion& last = outbox_.back();
        last.minX = std::min(last.minX, update.minX);
        last.minY = std::min(last.minY, minY);
        last.minZ = std::min(last.minZ, update.minZ);
        last.maxX = std::max(last.maxX, update.maxX);
        last.maxY = std::max(last.maxY, maxY);
        last.maxZ = std::max(last.maxZ, update.maxZ);
    } else {
        outbox_.push_back(DirtyRegion {update.minX, minY, update.minZ, update.maxX, maxY, update.maxZ});
    }
}

} // namespace net::minecraft
