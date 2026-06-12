#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace net::minecraft {
class World;
}

namespace net::minecraft::client::render::chunk {

// Immutable copy of every chunk a mesh rebuild can touch, taken on the main
// thread at job-enqueue time. Worker threads tessellate against this view, so
// they never read live Chunk arrays the tick/lighting code is mutating.
//
// Coverage is whole chunks in X/Z (the 3x3 neighborhood of a 16^3 section),
// plus the Y band that AO/neighbor-light sampling can touch. That keeps the
// worker detached from live chunk arrays without copying all 128 vertical
// levels for every 16-high section.
class RegionSnapshot final : public net::minecraft::BlockView {
public:
    // True when every world chunk the snapshot would copy is already resident.
    // Call before constructing a snapshot so mesh capture never triggers sync gen.
    [[nodiscard]] static bool regionReady(net::minecraft::World& world, int minBlockX, int minBlockY, int minBlockZ,
        int maxBlockX, int maxBlockY, int maxBlockZ);

    RegionSnapshot(net::minecraft::World& world, int minBlockX, int minBlockY, int minBlockZ, int maxBlockX,
        int maxBlockY, int maxBlockZ);

    // --- BlockView ---
    [[nodiscard]] int getBlockId(int x, int y, int z) const override;
    [[nodiscard]] net::minecraft::block::entity::BlockEntity* getBlockEntity(int, int, int) override
    {
        // Block entities are live objects; the mesh job records positions and
        // the main thread resolves pointers at upload time instead.
        return nullptr;
    }
    [[nodiscard]] float getNaturalBrightness(int x, int y, int z, int blockLight) const override;
    [[nodiscard]] float getLightBrightness(int x, int y, int z) const override;
    [[nodiscard]] int getBlockMeta(int x, int y, int z) const override;
    [[nodiscard]] net::minecraft::block::material::Material& getMaterial(int x, int y, int z) const override;
    [[nodiscard]] bool isBlockOpaqueCube(int x, int y, int z) const override;
    [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override;
    [[nodiscard]] net::minecraft::BiomeSource* getBiomeSource() const override
    {
        return biomeSource_.get();
    }

    [[nodiscard]] int getRawBrightness(int x, int y, int z) const
    {
        return getRawBrightness(x, y, z, true);
    }

    [[nodiscard]] int getRawBrightness(int x, int y, int z, bool useNeighborLight) const;

    // True if any brightness query so far touched a lit skylight value
    // (per-snapshot replacement for the old Chunk::hasSkyLight static).
    [[nodiscard]] bool sawSkyLight() const noexcept { return sawSkyLight_; }

private:
    struct ChunkCopy {
        std::vector<std::uint8_t> blocks;
        std::vector<std::uint8_t> meta;
        std::vector<std::uint8_t> skyLight;
        std::vector<std::uint8_t> blockLight;
        bool present = false;
    };

    [[nodiscard]] const ChunkCopy* chunkAt(int x, int z) const
    {
        const int localX = (x >> 4) - chunkX_;
        const int localZ = (z >> 4) - chunkZ_;
        if (localX < 0 || localZ < 0 || localX >= chunkWidth_ || localZ >= chunkDepth_) {
            return nullptr;
        }
        const ChunkCopy& copy = chunks_[static_cast<std::size_t>(localX + localZ * chunkWidth_)];
        return copy.present ? &copy : nullptr;
    }

    [[nodiscard]] bool containsY(int y) const noexcept
    {
        return y >= minY_ && y < minY_ + ySpan_;
    }

    [[nodiscard]] std::size_t snapshotIndex(int localX, int y, int localZ) const noexcept
    {
        return static_cast<std::size_t>(((localX << 4) | localZ) * ySpan_ + (y - minY_));
    }

    [[nodiscard]] static constexpr std::size_t fullBlockIndex(int localX, int y, int localZ)
    {
        return static_cast<std::size_t>((localX << 11) | (localZ << 7) | y);
    }

    [[nodiscard]] static int nibble(const std::vector<std::uint8_t>& bytes, std::size_t index)
    {
        const std::size_t byteIndex = index >> 1U;
        if (byteIndex >= bytes.size()) {
            return 0;
        }
        const std::uint8_t byte = bytes[byteIndex];
        return (index & 1U) != 0 ? (byte >> 4U) & 0xF : byte & 0xF;
    }

    int chunkX_ = 0;
    int chunkZ_ = 0;
    int chunkWidth_ = 0;
    int chunkDepth_ = 0;
    int minY_ = 0;
    int ySpan_ = 0;
    std::vector<ChunkCopy> chunks_;
    int ambientDarkness_ = 0;
    std::array<float, 16> lightLevelToLuminance_ {};
    std::unique_ptr<net::minecraft::BiomeSource> biomeSource_;
    mutable bool sawSkyLight_ = false;
};

} // namespace net::minecraft::client::render::chunk
