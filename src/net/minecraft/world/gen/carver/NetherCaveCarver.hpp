#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/gen/Generator.hpp"

#include <cstdint>

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.carver.NetherCaveCarver.
class NetherCaveCarver : public Generator {
public:
    using Generator::place;

protected:
    static constexpr float PI_F = 3.1415927f; // (float)Math.PI

    void carveRoom(int chunkX, int chunkZ, Chunk& chunk, double x, double y, double z)
    {
        placeTunnels(chunkX, chunkZ, chunk, x, y, z, 1.0f + random.nextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
    }

    void placeTunnels(int chunkX, int chunkZ, Chunk& chunk, double x, double y, double z, float baseWidth,
        float yaw, float pitch, int tunnel, int tunnelCount, double widthHeightRatio)
    {
        const double centerX = chunkX * 16 + 8;
        const double centerZ = chunkZ * 16 + 8;
        float f = 0.0f;
        float f2 = 0.0f;
        JavaRandom rng(static_cast<std::uint64_t>(random.nextLong()));
        if (tunnelCount <= 0) {
            const int n = range * 16 - 16;
            tunnelCount = n - rng.nextInt(n / 4);
        }
        int n = 0;
        if (tunnel == -1) {
            tunnel = tunnelCount / 2;
            n = 1;
        }
        const int splitAt = rng.nextInt(tunnelCount / 2) + tunnelCount / 4;
        const bool steepPitch = rng.nextInt(6) == 0;
        while (tunnel < tunnelCount) {
            const double d3 = 1.5 + static_cast<double>(MathHelper::sin(static_cast<float>(tunnel) * PI_F / static_cast<float>(tunnelCount)) * baseWidth * 1.0f);
            const double d4 = d3 * widthHeightRatio;
            const float f3 = MathHelper::cos(pitch);
            const float f4 = MathHelper::sin(pitch);
            x += static_cast<double>(MathHelper::cos(yaw) * f3);
            y += static_cast<double>(f4);
            z += static_cast<double>(MathHelper::sin(yaw) * f3);
            pitch *= steepPitch ? 0.92f : 0.7f;
            pitch += f2 * 0.1f;
            yaw += f * 0.1f;
            f2 *= 0.9f;
            f *= 0.75f;
            f2 += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
            f += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;
            if (n == 0 && tunnel == splitAt && baseWidth > 1.0f) {
                placeTunnels(chunkX, chunkZ, chunk, x, y, z, rng.nextFloat() * 0.5f + 0.5f, yaw - 1.5707964f, pitch / 3.0f, tunnel, tunnelCount, 1.0);
                placeTunnels(chunkX, chunkZ, chunk, x, y, z, rng.nextFloat() * 0.5f + 0.5f, yaw + 1.5707964f, pitch / 3.0f, tunnel, tunnelCount, 1.0);
                return;
            }
            if (n != 0 || rng.nextInt(4) != 0) {
                const double d5 = x - centerX;
                const double d6 = z - centerZ;
                const double d7 = tunnelCount - tunnel;
                const double d8 = baseWidth + 2.0f + 16.0f;
                if (d5 * d5 + d6 * d6 - d7 * d7 > d8 * d8) {
                    return;
                }
                if (!(x < centerX - 16.0 - d3 * 2.0 || z < centerZ - 16.0 - d3 * 2.0 || x > centerX + 16.0 + d3 * 2.0 || z > centerZ + 16.0 + d3 * 2.0)) {
                    int minX = MathHelper::floor(x - d3) - chunkX * 16 - 1;
                    int maxX = MathHelper::floor(x + d3) - chunkX * 16 + 1;
                    int minY = MathHelper::floor(y - d4) - 1;
                    int maxY = MathHelper::floor(y + d4) + 1;
                    int minZ = MathHelper::floor(z - d3) - chunkZ * 16 - 1;
                    int maxZ = MathHelper::floor(z + d3) - chunkZ * 16 + 1;
                    if (minX < 0) { minX = 0; }
                    if (maxX > 16) { maxX = 16; }
                    if (minY < 1) { minY = 1; }
                    if (maxY > 120) { maxY = 120; }
                    if (minZ < 0) { minZ = 0; }
                    if (maxZ > 16) { maxZ = 16; }

                    bool blocked = false;
                    for (int lx = minX; !blocked && lx < maxX; ++lx) {
                        for (int lz = minZ; !blocked && lz < maxZ; ++lz) {
                            for (int ly = maxY + 1; !blocked && ly >= minY - 1; --ly) {
                                if (ly < 0 || ly >= 128) { continue; }
                                const int id = rawBlock(chunk, lx, ly, lz);
                                if (id == Block::FLOWING_LAVA->id || id == Block::LAVA->id) {
                                    blocked = true;
                                }
                                if (ly == minY - 1 || lx == minX || lx == maxX - 1 || lz == minZ || lz == maxZ - 1) {
                                    continue;
                                }
                                ly = minY;
                            }
                        }
                    }
                    if (!blocked) {
                        for (int lx = minX; lx < maxX; ++lx) {
                            const double dx = (static_cast<double>(lx + chunkX * 16) + 0.5 - x) / d3;
                            for (int lz = minZ; lz < maxZ; ++lz) {
                                const double dz = (static_cast<double>(lz + chunkZ * 16) + 0.5 - z) / d3;
                                for (int i = maxY - 1; i >= minY; --i) {
                                    const double dy = (static_cast<double>(i) + 0.5 - y) / d4;
                                    // Decompiled quirk: block index lags the sphere
                                    // coordinate by one (operates on y = i + 1).
                                    if (dy > -0.7 && dx * dx + dy * dy + dz * dz < 1.0) {
                                        const int by = rawBlock(chunk, lx, i + 1, lz);
                                        if (by == Block::NETHERRACK->id || by == Block::DIRT->id || by == Block::GRASS_BLOCK->id) {
                                            setRawBlock(chunk, lx, i + 1, lz, 0);
                                        }
                                    }
                                }
                            }
                        }
                        if (n != 0) {
                            break;
                        }
                    }
                }
            }
            ++tunnel;
        }
    }

    void place(World* /*world*/, int startChunkX, int startChunkZ, int chunkX, int chunkZ, Chunk& chunk) override
    {
        int n = random.nextInt(random.nextInt(random.nextInt(10) + 1) + 1);
        if (random.nextInt(5) != 0) {
            n = 0;
        }
        for (int i = 0; i < n; ++i) {
            const double dx = startChunkX * 16 + random.nextInt(16);
            const double dy = random.nextInt(128);
            const double dz = startChunkZ * 16 + random.nextInt(16);
            int count = 1;
            if (random.nextInt(4) == 0) {
                carveRoom(chunkX, chunkZ, chunk, dx, dy, dz);
                count += random.nextInt(4);
            }
            for (int j = 0; j < count; ++j) {
                const float yaw = random.nextFloat() * PI_F * 2.0f;
                const float pitch = (random.nextFloat() - 0.5f) * 2.0f / 8.0f;
                const float width = random.nextFloat() * 2.0f + random.nextFloat();
                placeTunnels(chunkX, chunkZ, chunk, dx, dy, dz, width * 2.0f, yaw, pitch, 0, 0, 0.5);
            }
        }
    }
};

} // namespace net::minecraft
