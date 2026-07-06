#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include <cstdint>
namespace net::minecraft {
// Faithful 1:1 port of net.minecraft.world.gen.carver.CaveWorldCarver.
class CaveWorldCarver : public Generator {
public:
  using Generator::place;

protected:
  static constexpr float PI_F = 3.1415927f; // (float)Math.PI
  void carveRoom(int chunkX, int chunkZ, Chunk& chunk, double x, double y, double z) {
    placeTunnels(chunkX, chunkZ, chunk, x, y, z, 1.0f + random.nextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
  }
  void placeTunnels(int chunkX, int chunkZ, Chunk& chunk, double x, double y, double z, float baseWidth, float yaw,
                    float pitch, int tunnel, int tunnelCount, double widthHeightRatio) {
    const double centerX = chunkX * 16 + 8;
    const double centerZ = chunkZ * 16 + 8;
    float yawDrift = 0.0f;
    float pitchDrift = 0.0f;
    JavaRandom rng(static_cast<std::uint64_t>(random.nextLong()));
    if(tunnelCount <= 0) {
      const int tunnelCountRange = range * 16 - 16;
      tunnelCount = tunnelCountRange - rng.nextInt(tunnelCountRange / 4);
    }
    int splitFlag = 0;
    if(tunnel == -1) {
      tunnel = tunnelCount / 2;
      splitFlag = 1;
    }
    const int splitAt = rng.nextInt(tunnelCount / 2) + tunnelCount / 4;
    const bool steepPitch = rng.nextInt(6) == 0;
    while(tunnel < tunnelCount) {
      const double tunnelRadius = 1.5 + static_cast<double>(MathHelper::sin(static_cast<float>(tunnel) * PI_F /
                                                                            static_cast<float>(tunnelCount)) *
                                                            baseWidth * 1.0f);
      const double tunnelHeight = tunnelRadius * widthHeightRatio;
      const float cosPitch = MathHelper::cos(pitch);
      const float sinPitch = MathHelper::sin(pitch);
      x += static_cast<double>(MathHelper::cos(yaw) * cosPitch);
      y += static_cast<double>(sinPitch);
      z += static_cast<double>(MathHelper::sin(yaw) * cosPitch);
      pitch *= steepPitch ? 0.92f : 0.7f;
      pitch += pitchDrift * 0.1f;
      yaw += yawDrift * 0.1f;
      pitchDrift *= 0.9f;
      yawDrift *= 0.75f;
      const float pitchDriftA = rng.nextFloat();
      const float pitchDriftB = rng.nextFloat();
      pitchDrift += (pitchDriftA - pitchDriftB) * rng.nextFloat() * 2.0f;
      const float yawDriftA = rng.nextFloat();
      const float yawDriftB = rng.nextFloat();
      yawDrift += (yawDriftA - yawDriftB) * rng.nextFloat() * 4.0f;
      if(splitFlag == 0 && tunnel == splitAt && baseWidth > 1.0f) {
        placeTunnels(chunkX, chunkZ, chunk, x, y, z, rng.nextFloat() * 0.5f + 0.5f, yaw - 1.5707964f,
                     pitch / 3.0f, tunnel, tunnelCount, 1.0);
        placeTunnels(chunkX, chunkZ, chunk, x, y, z, rng.nextFloat() * 0.5f + 0.5f, yaw + 1.5707964f,
                     pitch / 3.0f, tunnel, tunnelCount, 1.0);
        return;
      }
      if(splitFlag != 0 || rng.nextInt(4) != 0) {
        const double offsetXFromCenter = x - centerX;
        const double offsetZFromCenter = z - centerZ;
        const double remainingSegments = tunnelCount - tunnel;
        const double maxDistanceFromCenter = baseWidth + 2.0f + 16.0f;
        if(offsetXFromCenter * offsetXFromCenter + offsetZFromCenter * offsetZFromCenter -
               remainingSegments * remainingSegments >
           maxDistanceFromCenter * maxDistanceFromCenter) {
          return;
        }
        if(!(x < centerX - 16.0 - tunnelRadius * 2.0 || z < centerZ - 16.0 - tunnelRadius * 2.0 ||
             x > centerX + 16.0 + tunnelRadius * 2.0 || z > centerZ + 16.0 + tunnelRadius * 2.0)) {
          int minX = MathHelper::floor(x - tunnelRadius) - chunkX * 16 - 1;
          int maxX = MathHelper::floor(x + tunnelRadius) - chunkX * 16 + 1;
          int minY = MathHelper::floor(y - tunnelHeight) - 1;
          int maxY = MathHelper::floor(y + tunnelHeight) + 1;
          int minZ = MathHelper::floor(z - tunnelRadius) - chunkZ * 16 - 1;
          int maxZ = MathHelper::floor(z + tunnelRadius) - chunkZ * 16 + 1;
          if(minX < 0) {
            minX = 0;
          }
          if(maxX > 16) {
            maxX = 16;
          }
          if(minY < 1) {
            minY = 1;
          }
          if(maxY > 120) {
            maxY = 120;
          }
          if(minZ < 0) {
            minZ = 0;
          }
          if(maxZ > 16) {
            maxZ = 16;
          }
          bool blocked = false;
          for(int lx = minX; !blocked && lx < maxX; ++lx) {
            for(int lz = minZ; !blocked && lz < maxZ; ++lz) {
              for(int ly = maxY + 1; !blocked && ly >= minY - 1; --ly) {
                if(ly < 0 || ly >= 128) {
                  continue;
                }
                const int id = rawBlock(chunk, lx, ly, lz);
                if(id == Block::FLOWING_WATER->id || id == Block::WATER->id) {
                  blocked = true;
                }
                if(ly == minY - 1 || lx == minX || lx == maxX - 1 || lz == minZ || lz == maxZ - 1) {
                  continue;
                }
                ly = minY;
              }
            }
          }
          if(!blocked) {
            for(int lx = minX; lx < maxX; ++lx) {
              const double normX = (static_cast<double>(lx + chunkX * 16) + 0.5 - x) / tunnelRadius;
              for(int lz = minZ; lz < maxZ; ++lz) {
                const double normZ = (static_cast<double>(lz + chunkZ * 16) + 0.5 - z) / tunnelRadius;
                bool grassAbove = false;
                if(!(normX * normX + normZ * normZ < 1.0)) {
                  continue;
                }
                for(int blockY = maxY - 1; blockY >= minY; --blockY) {
                  const double normY = (static_cast<double>(blockY) + 0.5 - y) / tunnelHeight;
                  if(normY > -0.7 && normX * normX + normY * normY + normZ * normZ < 1.0) {
                    // Decompiled quirk: block index lags the sphere
                    // coordinate by one (operates on y = blockY + 1).
                    const int by = rawBlock(chunk, lx, blockY + 1, lz);
                    if(by == Block::GRASS_BLOCK->id) {
                      grassAbove = true;
                    }
                    if(by == Block::STONE->id || by == Block::DIRT->id ||
                       by == Block::GRASS_BLOCK->id) {
                      if(blockY < 10) {
                        setRawBlock(chunk, lx, blockY + 1, lz, Block::FLOWING_LAVA->id);
                      } else {
                        setRawBlock(chunk, lx, blockY + 1, lz, 0);
                        if(grassAbove && rawBlock(chunk, lx, blockY, lz) == Block::DIRT->id) {
                          setRawBlock(chunk, lx, blockY, lz, Block::GRASS_BLOCK->id);
                        }
                      }
                    }
                  }
                }
              }
            }
            if(splitFlag != 0) {
              break;
            }
          }
        }
      }
      ++tunnel;
    }
  }
  void place(World* /*world*/, int startChunkX, int startChunkZ, int chunkX, int chunkZ, Chunk& chunk) override {
    int caveCount = random.nextInt(random.nextInt(random.nextInt(40) + 1) + 1);
    if(random.nextInt(15) != 0) {
      caveCount = 0;
    }
    for(int i = 0; i < caveCount; ++i) {
      const double dx = startChunkX * 16 + random.nextInt(16);
      const double dy = random.nextInt(random.nextInt(120) + 8);
      const double dz = startChunkZ * 16 + random.nextInt(16);
      int count = 1;
      if(random.nextInt(4) == 0) {
        carveRoom(chunkX, chunkZ, chunk, dx, dy, dz);
        count += random.nextInt(4);
      }
      for(int j = 0; j < count; ++j) {
        const float yaw = random.nextFloat() * PI_F * 2.0f;
        const float pitch = (random.nextFloat() - 0.5f) * 2.0f / 8.0f;
        const float widthBase = random.nextFloat();
        const float width = widthBase * 2.0f + random.nextFloat();
        placeTunnels(chunkX, chunkZ, chunk, dx, dy, dz, width, yaw, pitch, 0, 0, 1.0);
      }
    }
  }
};
} // namespace net::minecraft
