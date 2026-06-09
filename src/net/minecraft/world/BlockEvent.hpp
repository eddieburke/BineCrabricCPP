#pragma once

#include <cstddef>
#include <cstdint>

namespace net::minecraft {

class BlockEvent {
public:
    BlockEvent(int xIn, int yIn, int zIn, int blockIdIn)
        : x(xIn),
          y(yIn),
          z(zIn),
          blockId(blockIdIn),
          globalId(nextGlobalId())
    {
    }

    int x = 0;
    int y = 0;
    int z = 0;
    int blockId = 0;
    long long ticks = 0;

    [[nodiscard]] bool operator==(const BlockEvent& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z && blockId == other.blockId;
    }

    [[nodiscard]] BlockEvent withTicks(long long ticksIn) const
    {
        BlockEvent copy = *this;
        copy.ticks = ticksIn;
        return copy;
    }

    [[nodiscard]] int compareTo(const BlockEvent& other) const noexcept
    {
        if (ticks < other.ticks) {
            return -1;
        }
        if (ticks > other.ticks) {
            return 1;
        }
        if (globalId < other.globalId) {
            return -1;
        }
        if (globalId > other.globalId) {
            return 1;
        }
        return 0;
    }

private:
    static long long nextGlobalId()
    {
        static long long counter = 0;
        return counter++;
    }

    long long globalId = 0;
};

struct BlockEventHash {
    [[nodiscard]] std::size_t operator()(const BlockEvent& event) const noexcept
    {
        std::size_t hash = static_cast<std::size_t>(event.x);
        hash = hash * 31U + static_cast<std::size_t>(event.y);
        hash = hash * 31U + static_cast<std::size_t>(event.z);
        hash = hash * 31U + static_cast<std::size_t>(event.blockId);
        return hash;
    }
};

struct BlockEventComparator {
    [[nodiscard]] bool operator()(const BlockEvent& left, const BlockEvent& right) const noexcept
    {
        return left.compareTo(right) < 0;
    }
};

} // namespace net::minecraft
