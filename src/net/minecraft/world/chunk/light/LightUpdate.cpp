#include "net/minecraft/world/chunk/light/LightUpdate.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft {

void LightUpdate::updateLight(World* world)
{
    if (world == nullptr) {
        return;
    }

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
                chunkLoaded = world->isRegionLoaded(x, 0, z, 1);
                if (chunkLoaded) {
                    Chunk& chunk = world->getChunk(chunkX, chunkZ);
                    if (chunk.isEmpty()) {
                        chunkLoaded = false;
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
                const int current = world->getBrightness(lightType, x, y, z);
                int newLight = 0;
                const int blockId = world->getBlockId(x, y, z);
                int opacity = Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockId)];
                if (opacity == 0) {
                    opacity = 1;
                }
                int emission = 0;
                if (lightType == LightType::Sky) {
                    if (world->isTopY(x, y, z)) {
                        emission = 15;
                    }
                } else if (lightType == LightType::Block) {
                    emission = Block::BLOCKS_LIGHT_LUMINANCE[static_cast<std::size_t>(blockId)];
                }

                if (opacity >= 15 && emission == 0) {
                    newLight = 0;
                } else {
                    int brightest = world->getBrightness(lightType, x - 1, y, z);
                    const int east = world->getBrightness(lightType, x + 1, y, z);
                    const int down = world->getBrightness(lightType, x, y - 1, z);
                    const int up = world->getBrightness(lightType, x, y + 1, z);
                    const int north = world->getBrightness(lightType, x, y, z - 1);
                    const int south = world->getBrightness(lightType, x, y, z + 1);
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
                world->setLight(lightType, x, y, z, newLight);
                anyLightChanged = true;
                int propagate = newLight - 1;
                if (propagate < 0) {
                    propagate = 0;
                }
                world->updateLight(lightType, x - 1, y, z, propagate);
                world->updateLight(lightType, x, y - 1, z, propagate);
                world->updateLight(lightType, x, y, z - 1, propagate);
                if (x + 1 >= maxX) {
                    world->updateLight(lightType, x + 1, y, z, propagate);
                }
                if (y + 1 >= maxY) {
                    world->updateLight(lightType, x, y + 1, z, propagate);
                }
                if (z + 1 < maxZ) {
                    continue;
                }
                world->updateLight(lightType, x, y, z + 1, propagate);
            }
        }
    }

    if (anyLightChanged) {
        world->setBlocksDirty(minX, dirtyMinY, minZ, maxX, dirtyMaxY, maxZ);
    }
}

bool LightUpdate::expand(int expandMinX, int expandMinY, int expandMinZ, int expandMaxX, int expandMaxY, int expandMaxZ)
{
    if (expandMinX >= minX && expandMinY >= minY && expandMinZ >= minZ && expandMaxX <= maxX && expandMaxY <= maxY
        && expandMaxZ <= maxZ) {
        return true;
    }

    constexpr int margin = 1;
    if (expandMinX >= minX - margin && expandMinY >= minY - margin && expandMinZ >= minZ - margin
        && expandMaxX <= maxX + margin && expandMaxY <= maxY + margin && expandMaxZ <= maxZ + margin) {
        int mergedMinX = expandMinX;
        int mergedMinY = expandMinY;
        int mergedMinZ = expandMinZ;
        int mergedMaxX = expandMaxX;
        int mergedMaxY = expandMaxY;
        int mergedMaxZ = expandMaxZ;
        if (mergedMinX > minX) {
            mergedMinX = minX;
        }
        if (mergedMinY > minY) {
            mergedMinY = minY;
        }
        if (mergedMinZ > minZ) {
            mergedMinZ = minZ;
        }
        if (mergedMaxX < maxX) {
            mergedMaxX = maxX;
        }
        if (mergedMaxY < maxY) {
            mergedMaxY = maxY;
        }
        if (mergedMaxZ < maxZ) {
            mergedMaxZ = maxZ;
        }
        const int oldVolume = (maxX - minX) * (maxY - minY) * (maxZ - minZ);
        const int newVolume = (mergedMaxX - mergedMinX) * (mergedMaxY - mergedMinY) * (mergedMaxZ - mergedMinZ);
        if (newVolume - oldVolume <= 2) {
            minX = mergedMinX;
            minY = mergedMinY;
            minZ = mergedMinZ;
            maxX = mergedMaxX;
            maxY = mergedMaxY;
            maxZ = mergedMaxZ;
            return true;
        }
    }
    return false;
}

} // namespace net::minecraft
