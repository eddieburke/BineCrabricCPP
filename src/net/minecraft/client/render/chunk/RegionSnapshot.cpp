#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

#include <algorithm>

namespace net::minecraft::client::render::chunk {

bool RegionSnapshot::regionReady(net::minecraft::World& world, int minBlockX, int minBlockY, int minBlockZ,
    int maxBlockX, int maxBlockY, int maxBlockZ)
{
    (void)minBlockY;
    (void)maxBlockY;
    net::minecraft::ChunkSource* source = world.getChunkSource();
    if (source == nullptr) {
        return false;
    }
    const int minChunkX = minBlockX >> 4;
    const int minChunkZ = minBlockZ >> 4;
    const int maxChunkX = maxBlockX >> 4;
    const int maxChunkZ = maxBlockZ >> 4;
    for (int cx = minChunkX; cx <= maxChunkX; ++cx) {
        for (int cz = minChunkZ; cz <= maxChunkZ; ++cz) {
            if (!source->isChunkLoaded(cx, cz)) {
                return false;
            }
        }
    }
    return true;
}

RegionSnapshot::RegionSnapshot(net::minecraft::World& world, int minBlockX, int minBlockY, int minBlockZ, int maxBlockX,
    int maxBlockY, int maxBlockZ)
{
    chunkX_ = minBlockX >> 4;
    chunkZ_ = minBlockZ >> 4;
    const int maxChunkX = maxBlockX >> 4;
    const int maxChunkZ = maxBlockZ >> 4;
    chunkWidth_ = maxChunkX - chunkX_ + 1;
    chunkDepth_ = maxChunkZ - chunkZ_ + 1;
    chunks_.resize(static_cast<std::size_t>(chunkWidth_ * chunkDepth_));

    minY_ = std::clamp(minBlockY, 0, Chunk::height - 1);
    const int maxY = std::clamp(maxBlockY, 0, Chunk::height - 1);
    ySpan_ = maxY >= minY_ ? maxY - minY_ + 1 : 0;
    const std::size_t copiedBlockCount = static_cast<std::size_t>(16 * 16 * ySpan_);

    for (int cx = chunkX_; cx <= maxChunkX; ++cx) {
        for (int cz = chunkZ_; cz <= maxChunkZ; ++cz) {
            const Chunk& chunk = world.getChunk(cx, cz);
            ChunkCopy& copy = chunks_[static_cast<std::size_t>((cx - chunkX_) + (cz - chunkZ_) * chunkWidth_)];
            copy.blocks.assign(copiedBlockCount, 0);
            copy.meta.assign(copiedBlockCount, 0);
            copy.skyLight.assign(copiedBlockCount, 0);
            copy.blockLight.assign(copiedBlockCount, 0);
            for (int localX = 0; localX < 16; ++localX) {
                for (int localZ = 0; localZ < 16; ++localZ) {
                    for (int y = minY_; y < minY_ + ySpan_; ++y) {
                        const std::size_t sourceIndex = fullBlockIndex(localX, y, localZ);
                        const std::size_t destIndex = snapshotIndex(localX, y, localZ);
                        if (sourceIndex < chunk.blocks.size()) {
                            copy.blocks[destIndex] = chunk.blocks[sourceIndex];
                        }
                        copy.meta[destIndex] = static_cast<std::uint8_t>(nibble(chunk.meta.bytes, sourceIndex));
                        copy.skyLight[destIndex] = static_cast<std::uint8_t>(nibble(chunk.skyLight.bytes, sourceIndex));
                        copy.blockLight[destIndex] =
                            static_cast<std::uint8_t>(nibble(chunk.blockLight.bytes, sourceIndex));
                    }
                }
            }
            copy.present = true;
        }
    }

    ambientDarkness_ = world.ambientDarkness;
    if (world.dimension != nullptr) {
        lightLevelToLuminance_ = world.dimension->lightLevelToLuminance;
        if (world.dimension->biomeSource) {
            biomeSource_ = world.dimension->biomeSource->clone();
        }
    } else {
        for (int level = 0; level < 16; ++level) {
            lightLevelToLuminance_[static_cast<std::size_t>(level)] = Dimension::luminanceForLightLevel(level);
        }
    }
}

int RegionSnapshot::getBlockId(int x, int y, int z) const
{
    if (y < 0 || y >= Chunk::height) {
        return 0;
    }
    const ChunkCopy* chunk = chunkAt(x, z);
    if (chunk == nullptr || !containsY(y)) {
        return 0;
    }
    return static_cast<int>(chunk->blocks[snapshotIndex(x & 0xF, y, z & 0xF)] & 0xFFU);
}

int RegionSnapshot::getBlockMeta(int x, int y, int z) const
{
    if (y < 0 || y >= Chunk::height) {
        return 0;
    }
    const ChunkCopy* chunk = chunkAt(x, z);
    if (chunk == nullptr || !containsY(y)) {
        return 0;
    }
    return static_cast<int>(chunk->meta[snapshotIndex(x & 0xF, y, z & 0xF)] & 0xFU);
}

float RegionSnapshot::getNaturalBrightness(int x, int y, int z, int blockLight) const
{
    int brightness = getRawBrightness(x, y, z);
    if (brightness < blockLight) {
        brightness = blockLight;
    }
    return lightLevelToLuminance_[static_cast<std::size_t>(brightness)];
}

float RegionSnapshot::getLightBrightness(int x, int y, int z) const
{
    return lightLevelToLuminance_[static_cast<std::size_t>(getRawBrightness(x, y, z))];
}

int RegionSnapshot::getRawBrightness(int x, int y, int z, bool useNeighborLight) const
{
    if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
        return 15;
    }
    if (useNeighborLight && block::Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
        int brightness = getRawBrightness(x, y + 1, z, false);
        brightness = std::max(brightness, getRawBrightness(x + 1, y, z, false));
        brightness = std::max(brightness, getRawBrightness(x - 1, y, z, false));
        brightness = std::max(brightness, getRawBrightness(x, y, z + 1, false));
        brightness = std::max(brightness, getRawBrightness(x, y, z - 1, false));
        return brightness;
    }
    if (y < 0) {
        return 0;
    }
    if (y >= Chunk::height) {
        const int brightness = 15 - ambientDarkness_;
        return brightness < 0 ? 0 : brightness;
    }
    const ChunkCopy* chunk = chunkAt(x, z);
    if (chunk == nullptr || !containsY(y)) {
        return 0;
    }

    // Mirrors Chunk::getLight(localX, y, localZ, ambientDarkness), with the
    // skylight detection recorded per-snapshot instead of a global static.
    const std::size_t index = snapshotIndex(x & 0xF, y, z & 0xF);
    int sky = static_cast<int>(chunk->skyLight[index] & 0xFU);
    if (sky > 0) {
        sawSkyLight_ = true;
    }
    const int block = static_cast<int>(chunk->blockLight[index] & 0xFU);
    if (block > (sky -= ambientDarkness_)) {
        sky = block;
    }
    return sky < 0 ? 0 : sky;
}

net::minecraft::block::material::Material& RegionSnapshot::getMaterial(int x, int y, int z) const
{
    const int blockId = getBlockId(x, y, z);
    if (blockId == 0) {
        return block::material::Material::AIR;
    }
    block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if (block == nullptr) {
        return block::material::Material::AIR;
    }
    return block->material;
}

bool RegionSnapshot::isBlockOpaqueCube(int x, int y, int z) const
{
    block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
    if (block == nullptr) {
        return false;
    }
    return block->isOpaque();
}

bool RegionSnapshot::shouldSuffocate(int x, int y, int z) const
{
    block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
    if (block == nullptr) {
        return false;
    }
    return block->material.blocksMovement() && block->isFullCube();
}

} // namespace net::minecraft::client::render::chunk
