#include "net/minecraft/world/light/LightingEngine.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

#include <algorithm>
#include <utility>

namespace net::minecraft {

namespace {

    [[nodiscard]] constexpr int defaultLightValue(LightType type) noexcept
    {
        return type == LightType::Sky ? 15 : 0;
    }

} // namespace

LightingEngine::LightingEngine()
    : thread_([this](const std::stop_token& stop) { threadLoop(stop); })
{
}

void LightingEngine::push(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge)
{
    bool added = false;
    {
        const std::lock_guard lock(queueMutex_);
        if (merge) {
            constexpr int mergeBudget = 5;
            const std::size_t queueSize = queue_.size();
            const int budget
                = queueSize < static_cast<std::size_t>(mergeBudget) ? static_cast<int>(queueSize) : mergeBudget;
            for (int i = 0; i < budget; ++i) {
                LightUpdate& existing = queue_[queue_.size() - static_cast<std::size_t>(i) - 1];
                if (existing.lightType == type && existing.expand(minX, minY, minZ, maxX, maxY, maxZ)) {
                    return;
                }
            }
        }
        queue_.emplace_back(type, minX, minY, minZ, maxX, maxY, maxZ);
        constexpr std::size_t maxQueueSize = 1000000;
        if (queue_.size() > maxQueueSize) {
            queue_.clear();
        }
        pendingCount_.store(queue_.size() + (processing_ ? 1U : 0U), std::memory_order_relaxed);
        added = !queue_.empty();
    }
    if (added) {
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
    std::vector<DirtyRegion> regions;
    regions.reserve(count);
    for (auto it = first; it != outbox_.end(); ++it) {
        regions.push_back(std::move(*it));
    }
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
    idleCv_.wait(lock, [this] { return queue_.empty() && !processing_; });
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
        LightUpdate update(LightType::Block, 0, 0, 0, 0, 0, 0);
        {
            std::unique_lock lock(queueMutex_);
            workCv_.wait(lock, [&] { return stop.stop_requested() || !queue_.empty(); });
            if (stop.stop_requested()) {
                return;
            }
            update = queue_.back();
            queue_.pop_back();
            processing_ = true;
            pendingCount_.store(queue_.size() + 1, std::memory_order_relaxed);
        }

        processUpdate(update);
        releasePins();

        {
            const std::lock_guard lock(queueMutex_);
            processing_ = false;
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
    if (const auto cached = pinCache_.find(key); cached != pinCache_.end()) {
        return cached->second;
    }
    Chunk* chunk = nullptr;
    {
        const std::lock_guard lock(registryMutex_);
        const auto it = registry_.find(key);
        if (it != registry_.end()) {
            chunk = it->second;
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

int LightingEngine::getBlockId(int x, int y, int z)
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return 0;
    }
    if (y < 0 || y >= Chunk::height) {
        return 0;
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    if (chunk == nullptr) {
        return 0;
    }
    return chunk->getBlockId(x & 15, y, z & 15);
}

int LightingEngine::getBrightness(LightType type, int x, int y, int z)
{
    if (y < 0) {
        y = 0;
    }
    if (y >= Chunk::height) {
        y = Chunk::height - 1;
    }
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return defaultLightValue(type);
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    if (chunk == nullptr) {
        return 0;
    }
    return chunk->getLight(type, x & 15, y, z & 15);
}

void LightingEngine::setLight(LightType type, int x, int y, int z, int value)
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return;
    }
    if (y < 0 || y >= Chunk::height) {
        return;
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    if (chunk == nullptr) {
        return;
    }
    if (chunk->getLight(type, x & 15, y, z & 15) == value) {
        return;
    }
    chunk->setLight(type, x & 15, y, z & 15, value);
}

bool LightingEngine::isTopY(int x, int y, int z)
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return false;
    }
    if (y < 0) {
        return false;
    }
    if (y >= Chunk::height) {
        return true;
    }
    Chunk* chunk = chunkAt(x >> 4, z >> 4);
    if (chunk == nullptr) {
        return false;
    }
    return chunk->isAboveMaxHeight(x & 15, y, z & 15);
}

void LightingEngine::spreadLight(LightType type, int x, int y, int z, int level)
{
    if (type == LightType::Sky && skyLightSuppressed_.load(std::memory_order_relaxed)) {
        return;
    }
    if (y < 0 || y >= Chunk::height) {
        return;
    }
    if (chunkAt(x >> 4, z >> 4) == nullptr) {
        return;
    }
    if (type == LightType::Sky) {
        if (isTopY(x, y, z)) {
            level = 15;
        }
    } else if (type == LightType::Block) {
        const int blockId = getBlockId(x, y, z);
        const int luminance = block::Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(blockId)];
        if (luminance > level) {
            level = luminance;
        }
    }
    if (getBrightness(type, x, y, z) != level) {
        push(type, x, y, z, x, y, z, true);
    }
}

void LightingEngine::processUpdate(const LightUpdate& update)
{
    using block::Block;

    const LightType lightType = update.lightType;
    int minX = update.minX;
    int minY = update.minY;
    int minZ = update.minZ;
    const int maxX = update.maxX;
    int maxY = update.maxY;
    const int maxZ = update.maxZ;

    const int sizeX = maxX - minX + 1;
    const int sizeY = maxY - minY + 1;
    const int sizeZ = maxZ - minZ + 1;
    const int volume = sizeX * sizeY * sizeZ;
    if (volume > 32768) {
        return;
    }

    int lastChunkX = 0;
    int lastChunkZ = 0;
    bool lastChunkLoaded = false;
    bool anyLightChanged = false;
    int dirtyMinY = minY;
    int dirtyMaxY = maxY;
    if (dirtyMinY < 0) {
        dirtyMinY = 0;
    }
    if (dirtyMaxY >= Chunk::height) {
        dirtyMaxY = Chunk::height - 1;
    }

    for (int x = minX; x <= maxX; ++x) {
        for (int z = minZ; z <= maxZ; ++z) {
            const int chunkX = x >> 4;
            const int chunkZ = z >> 4;
            bool chunkLoaded = false;
            if (lastChunkLoaded && chunkX == lastChunkX && chunkZ == lastChunkZ) {
                chunkLoaded = lastChunkLoaded;
            } else {
                // Java parity: isRegionLoaded(x, 0, z, 1) — the 3x3 block
                // neighborhood, so border columns need the adjacent chunk too.
                chunkLoaded = true;
                for (int cx = (x - 1) >> 4; cx <= (x + 1) >> 4 && chunkLoaded; ++cx) {
                    for (int cz = (z - 1) >> 4; cz <= (z + 1) >> 4; ++cz) {
                        if (chunkAt(cx, cz) == nullptr) {
                            chunkLoaded = false;
                            break;
                        }
                    }
                }
                lastChunkLoaded = chunkLoaded;
                lastChunkX = chunkX;
                lastChunkZ = chunkZ;
            }
            if (!chunkLoaded) {
                continue;
            }

            if (minY < 0) {
                minY = 0;
            }
            if (maxY >= Chunk::height) {
                maxY = Chunk::height - 1;
            }

            for (int y = minY; y <= maxY; ++y) {
                const int current = getBrightness(lightType, x, y, z);
                int newLight = 0;
                const int blockId = getBlockId(x, y, z);
                int opacity = Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockId)];
                if (opacity == 0) {
                    opacity = 1;
                }
                int emission = 0;
                if (lightType == LightType::Sky) {
                    if (isTopY(x, y, z)) {
                        emission = 15;
                    }
                } else if (lightType == LightType::Block) {
                    emission = Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(blockId)];
                }

                if (opacity >= 15 && emission == 0) {
                    newLight = 0;
                } else {
                    int brightest = getBrightness(lightType, x - 1, y, z);
                    const int east = getBrightness(lightType, x + 1, y, z);
                    const int down = getBrightness(lightType, x, y - 1, z);
                    const int up = getBrightness(lightType, x, y + 1, z);
                    const int north = getBrightness(lightType, x, y, z - 1);
                    const int south = getBrightness(lightType, x, y, z + 1);
                    if (east > brightest) {
                        brightest = east;
                    }
                    if (down > brightest) {
                        brightest = down;
                    }
                    if (up > brightest) {
                        brightest = up;
                    }
                    if (north > brightest) {
                        brightest = north;
                    }
                    if (south > brightest) {
                        brightest = south;
                    }
                    brightest -= opacity;
                    if (brightest < 0) {
                        brightest = 0;
                    }
                    if (emission > brightest) {
                        brightest = emission;
                    }
                    newLight = brightest;
                }

                if (current == newLight) {
                    continue;
                }
                setLight(lightType, x, y, z, newLight);
                anyLightChanged = true;
                int propagate = newLight - 1;
                if (propagate < 0) {
                    propagate = 0;
                }
                spreadLight(lightType, x - 1, y, z, propagate);
                spreadLight(lightType, x, y - 1, z, propagate);
                spreadLight(lightType, x, y, z - 1, propagate);
                if (x + 1 >= maxX) {
                    spreadLight(lightType, x + 1, y, z, propagate);
                }
                if (y + 1 >= maxY) {
                    spreadLight(lightType, x, y + 1, z, propagate);
                }
                if (z + 1 < maxZ) {
                    continue;
                }
                spreadLight(lightType, x, y, z + 1, propagate);
            }
        }
    }

    if (anyLightChanged) {
        const std::lock_guard lock(outboxMutex_);
        // Soft cap: under load (initial world light) collapse into the last
        // entry instead of growing without bound; the renderer just re-marks
        // a wider area dirty.
        constexpr std::size_t maxOutbox = 4096;
        if (outbox_.size() >= maxOutbox) {
            DirtyRegion& last = outbox_.back();
            last.minX = std::min(last.minX, minX);
            last.minY = std::min(last.minY, dirtyMinY);
            last.minZ = std::min(last.minZ, minZ);
            last.maxX = std::max(last.maxX, maxX);
            last.maxY = std::max(last.maxY, dirtyMaxY);
            last.maxZ = std::max(last.maxZ, maxZ);
        } else {
            outbox_.push_back(DirtyRegion {minX, dirtyMinY, minZ, maxX, dirtyMaxY, maxZ});
        }
    }
}

} // namespace net::minecraft
