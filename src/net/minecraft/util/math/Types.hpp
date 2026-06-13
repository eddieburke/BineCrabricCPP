#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>

#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft {

inline int floor_int(double value)
{
    return static_cast<int>(std::floor(value));
}

inline int mod_16(int value)
{
    int result = value % 16;
    return result < 0 ? result + 16 : result;
}

inline int chunk_coord(int blockCoord)
{
    return blockCoord >= 0 ? blockCoord / 16 : -((15 - blockCoord) / 16);
}

inline std::uint8_t nibble_get(const std::array<std::uint8_t, 16384>& data, std::size_t index)
{
    const std::uint8_t byte = data[index >> 1];
    if ((index & 1U) == 0U) {
        return static_cast<std::uint8_t>(byte & 0x0F);
    }
    return static_cast<std::uint8_t>((byte >> 4) & 0x0F);
}

inline void nibble_set(std::array<std::uint8_t, 16384>& data, std::size_t index, std::uint8_t value)
{
    value &= 0x0F;
    std::uint8_t& byte = data[index >> 1];
    if ((index & 1U) == 0U) {
        byte = static_cast<std::uint8_t>((byte & 0xF0) | value);
    } else {
        byte = static_cast<std::uint8_t>((byte & 0x0F) | (static_cast<std::uint8_t>(value << 4)));
    }
}

struct Vec3i {
    int x = 0;
    int y = 0;
    int z = 0;

    constexpr Vec3i() = default;
    constexpr Vec3i(int x, int y, int z) : x(x), y(y), z(z) {}

    constexpr bool operator==(const Vec3i& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    constexpr bool operator!=(const Vec3i& other) const noexcept
    {
        return !(*this == other);
    }
};

struct Vec3iHash {
    [[nodiscard]] std::size_t operator()(const Vec3i& pos) const noexcept
    {
        // Java: this.x + this.z << 8 + this.y << 16 (+ and << are equal precedence, left-to-right)
        return static_cast<std::size_t>((((pos.x + pos.z) << 8) + pos.y) << 16);
    }
};

struct Vec3d {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    constexpr Vec3d() = default;
    constexpr Vec3d(double x, double y, double z) : x(x), y(y), z(z) {}

    constexpr Vec3d operator+(const Vec3d& other) const noexcept
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    constexpr Vec3d operator-(const Vec3d& other) const noexcept
    {
        return {x - other.x, y - other.y, z - other.z};
    }

    constexpr Vec3d operator*(double scalar) const noexcept
    {
        return {x * scalar, y * scalar, z * scalar};
    }

    [[nodiscard]] constexpr Vec3d relativize(const Vec3d& other) const noexcept
    {
        return {other.x - x, other.y - y, other.z - z};
    }

    [[nodiscard]] constexpr Vec3d crossProduct(const Vec3d& other) const noexcept
    {
        return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
    }

    [[nodiscard]] Vec3d normalize() const noexcept
    {
        const double length = std::sqrt(x * x + y * y + z * z);
        if (length < 1.0E-4) {
            return {};
        }
        return {x / length, y / length, z / length};
    }
};

struct Box {
    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;

    constexpr Box() = default;
    constexpr Box(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
        : minX(minX), minY(minY), minZ(minZ), maxX(maxX), maxY(maxY), maxZ(maxZ) {}

    [[nodiscard]] constexpr Box offset(double dx, double dy, double dz) const noexcept
    {
        return {minX + dx, minY + dy, minZ + dz, maxX + dx, maxY + dy, maxZ + dz};
    }

    [[nodiscard]] constexpr Box expand(double amount) const noexcept
    {
        return {minX - amount, minY - amount, minZ - amount, maxX + amount, maxY + amount, maxZ + amount};
    }

    [[nodiscard]] constexpr Box expand(double x, double y, double z) const noexcept
    {
        return {minX - x, minY - y, minZ - z, maxX + x, maxY + y, maxZ + z};
    }

    [[nodiscard]] constexpr Box contract(double amount) const noexcept
    {
        return {minX + amount, minY + amount, minZ + amount, maxX - amount, maxY - amount, maxZ - amount};
    }

    [[nodiscard]] constexpr Box contract(double x, double y, double z) const noexcept
    {
        return {minX + x, minY + y, minZ + z, maxX - x, maxY - y, maxZ - z};
    }

    [[nodiscard]] constexpr bool intersects(const Box& other) const noexcept
    {
        return maxX > other.minX && minX < other.maxX
            && maxY > other.minY && minY < other.maxY
            && maxZ > other.minZ && minZ < other.maxZ;
    }

    [[nodiscard]] constexpr Box copy() const noexcept
    {
        return *this;
    }

    [[nodiscard]] constexpr Box translate(double dx, double dy, double dz) const noexcept
    {
        return {minX + dx, minY + dy, minZ + dz, maxX + dx, maxY + dy, maxZ + dz};
    }

    [[nodiscard]] constexpr Box stretch(double dx, double dy, double dz) const noexcept
    {
        double minXOut = minX;
        double minYOut = minY;
        double minZOut = minZ;
        double maxXOut = maxX;
        double maxYOut = maxY;
        double maxZOut = maxZ;
        if (dx < 0.0) {
            minXOut += dx;
        } else {
            maxXOut += dx;
        }
        if (dy < 0.0) {
            minYOut += dy;
        } else {
            maxYOut += dy;
        }
        if (dz < 0.0) {
            minZOut += dz;
        } else {
            maxZOut += dz;
        }
        return {minXOut, minYOut, minZOut, maxXOut, maxYOut, maxZOut};
    }

    [[nodiscard]] constexpr double getXOffset(const Box& other, double offsetX) const noexcept
    {
        if (other.maxY <= minY || other.minY >= maxY || other.maxZ <= minZ || other.minZ >= maxZ) {
            return offsetX;
        }
        if (offsetX > 0.0 && other.maxX <= minX) {
            return std::min(minX - other.maxX, offsetX);
        }
        if (offsetX < 0.0 && other.minX >= maxX) {
            return std::max(maxX - other.minX, offsetX);
        }
        return offsetX;
    }

    [[nodiscard]] constexpr double getYOffset(const Box& other, double offsetY) const noexcept
    {
        if (other.maxX <= minX || other.minX >= maxX || other.maxZ <= minZ || other.minZ >= maxZ) {
            return offsetY;
        }
        if (offsetY > 0.0 && other.maxY <= minY) {
            return std::min(minY - other.maxY, offsetY);
        }
        if (offsetY < 0.0 && other.minY >= maxY) {
            return std::max(maxY - other.minY, offsetY);
        }
        return offsetY;
    }

    [[nodiscard]] constexpr double getZOffset(const Box& other, double offsetZ) const noexcept
    {
        if (other.maxX <= minX || other.minX >= maxX || other.maxY <= minY || other.minY >= maxY) {
            return offsetZ;
        }
        if (offsetZ > 0.0 && other.maxZ <= minZ) {
            return std::min(minZ - other.maxZ, offsetZ);
        }
        if (offsetZ < 0.0 && other.minZ >= maxZ) {
            return std::max(maxZ - other.minZ, offsetZ);
        }
        return offsetZ;
    }
};

// MathHelper lives in its own faithful header (real 65536-entry sine table).
// Types.hpp pulls it in so existing MathHelper::sin/cos/sqrt/floor/absMax call
// sites resolve to the parity-correct class.

struct ChunkPos {
    int x = 0;
    int z = 0;

    constexpr ChunkPos() = default;
    constexpr ChunkPos(int x, int z) : x(x), z(z) {}

    constexpr bool operator==(const ChunkPos& other) const noexcept
    {
        return x == other.x && z == other.z;
    }
};

inline int chunkPosHashCode(int x, int z) noexcept
{
    return (x < 0 ? std::numeric_limits<int>::min() : 0)
        | ((x & 32767) << 16)
        | (z < 0 ? 32768 : 0)
        | (z & 32767);
}

struct ChunkPosHash {
    [[nodiscard]] std::size_t operator()(const ChunkPos& pos) const noexcept
    {
        return static_cast<std::size_t>(chunkPosHashCode(pos.x, pos.z));
    }
};

class JavaRandom {
public:
    explicit JavaRandom(std::uint64_t seed = 0)
    {
        setSeed(seed);
    }

    void setSeed(std::uint64_t seed)
    {
        state_ = (seed ^ 0x5DEECE66DULL) & mask_;
        haveNextGaussian_ = false;
    }

    [[nodiscard]] int next(int bits) const
    {
        state_ = (state_ * 0x5DEECE66DULL + 0xBULL) & mask_;
        return javaIntFromBits(static_cast<std::uint32_t>(state_ >> (48U - bits)));
    }

    [[nodiscard]] int nextInt() const
    {
        return next(32);
    }

    [[nodiscard]] int nextInt(int bound) const
    {
        if (bound <= 0) {
            return 0;
        }

        if ((bound & -bound) == bound) {
            return static_cast<int>((static_cast<std::int64_t>(bound) * next(31)) >> 31);
        }

        int bits = 0;
        int value = 0;
        do {
            bits = next(31);
            value = bits % bound;
        } while (javaIntOverflowIsNegative(static_cast<std::uint32_t>(bits - value) + static_cast<std::uint32_t>(bound - 1)));
        return value;
    }

    [[nodiscard]] float nextFloat() const
    {
        return static_cast<float>(next(24)) / static_cast<float>(1 << 24);
    }

    [[nodiscard]] double nextDouble() const
    {
        const std::uint64_t high = static_cast<std::uint64_t>(next(26));
        const std::uint64_t low = static_cast<std::uint64_t>(next(27));
        return static_cast<double>((high << 27U) + low) / static_cast<double>(1ULL << 53U);
    }

    [[nodiscard]] double nextGaussian() const
    {
        if (haveNextGaussian_) {
            haveNextGaussian_ = false;
            return nextGaussianValue_;
        }

        double v1 = 0.0;
        double v2 = 0.0;
        double s = 0.0;
        do {
            v1 = 2.0 * nextDouble() - 1.0;
            v2 = 2.0 * nextDouble() - 1.0;
            s = v1 * v1 + v2 * v2;
        } while (s >= 1.0 || s == 0.0);

        const double multiplier = std::sqrt(-2.0 * std::log(s) / s);
        nextGaussianValue_ = v2 * multiplier;
        haveNextGaussian_ = true;
        return v1 * multiplier;
    }

    [[nodiscard]] std::int64_t nextLong() const
    {
        const std::uint64_t high = static_cast<std::uint32_t>(next(32));
        const std::int64_t low = next(32);
        return javaLongFromBits((high << 32U) + static_cast<std::uint64_t>(low));
    }

private:
    static constexpr std::uint64_t mask_ = (1ULL << 48U) - 1ULL;

    [[nodiscard]] static constexpr int javaIntFromBits(std::uint32_t value) noexcept
    {
        return (value & 0x80000000U) == 0U
            ? static_cast<int>(value)
            : -1 - static_cast<int>(~value);
    }

    [[nodiscard]] static constexpr std::int64_t javaLongFromBits(std::uint64_t value) noexcept
    {
        return (value & 0x8000000000000000ULL) == 0ULL
            ? static_cast<std::int64_t>(value)
            : -1LL - static_cast<std::int64_t>(~value);
    }

    [[nodiscard]] static constexpr bool javaIntOverflowIsNegative(std::uint32_t value) noexcept
    {
        return (value & 0x80000000U) != 0U;
    }

    mutable std::uint64_t state_ = 0;
    mutable bool haveNextGaussian_ = false;
    mutable double nextGaussianValue_ = 0.0;
};

} // namespace net::minecraft
