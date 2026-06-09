#pragma once

#include <cstddef>

namespace net::minecraft::util::math {

// Faithful port of net.minecraft.util.math.BlockPos (beta 1.7.3).
class BlockPos {
public:
    int x;
    int y;
    int z;

    BlockPos(int x, int y, int z) : x(x), y(y), z(z) {}

    bool operator==(const BlockPos& other) const {
        return other.x == this->x && other.y == this->y && other.z == this->z;
    }

    bool operator!=(const BlockPos& other) const { return !(*this == other); }

    [[nodiscard]] int hashCode() const {
        return this->x * 8976890 + this->y * 981131 + this->z;
    }
};

struct BlockPosHash {
    [[nodiscard]] std::size_t operator()(const BlockPos& pos) const {
        return static_cast<std::size_t>(pos.hashCode());
    }
};

} // namespace net::minecraft::util::math
