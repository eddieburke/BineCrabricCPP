#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

#include <algorithm>
#include <cstring>

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

    // Force an even Y origin and span so the packed-nibble copies below stay
    // byte-aligned with the source arrays (2 nibbles per byte along Y).
    minY_ = std::clamp(minBlockY, 0, Chunk::height - 1) & ~1;
    const int maxY = std::clamp(maxBlockY, 0, Chunk::height - 1);
    int span = maxY >= minY_ ? maxY - minY_ + 1 : 0;
    span += span & 1;
    ySpan_ = std::min(span, Chunk::height - minY_);
    const int halfSpan = ySpan_ >> 1;

    constexpr std::size_t kFullBlockCount = 16 * 16 * static_cast<std::size_t>(Chunk::height);
    const std::size_t blockBytes = static_cast<std::size_t>(16 * 16 * ySpan_);
    const std::size_t nibbleBytes = static_cast<std::size_t>(16 * 16 * halfSpan);

    for (int cx = chunkX_; cx <= maxChunkX; ++cx) {
        for (int cz = chunkZ_; cz <= maxChunkZ; ++cz) {
            const Chunk& chunk = world.getChunk(cx, cz);
            ChunkCopy& copy = chunks_[static_cast<std::size_t>((cx - chunkX_) + (cz - chunkZ_) * chunkWidth_)];
            copy.present = true;

            // Empty/blank chunks have no storage; leave the zeroed copies.
            if (chunk.blocks.size() < kFullBlockCount || chunk.meta.bytes.size() < kFullBlockCount / 2
                || chunk.skyLight.bytes.size() < kFullBlockCount / 2
                || chunk.blockLight.bytes.size() < kFullBlockCount / 2) {
                copy.blocks.assign(blockBytes, 0);
                copy.meta.assign(nibbleBytes, 0);
                copy.skyLight.assign(nibbleBytes, 0);
                copy.blockLight.assign(nibbleBytes, 0);
                continue;
            }

            copy.blocks.resize(blockBytes);
            copy.meta.resize(nibbleBytes);
            copy.skyLight.resize(nibbleBytes);
            copy.blockLight.resize(nibbleBytes);

            // Both source and destination keep Y contiguous per (x, z) column,
            // so each column is three nibble memcpys and one block memcpy.
            for (int column = 0; column < 16 * 16; ++column) {
                const int localX = column >> 4;
                const int localZ = column & 0xF;
                const std::size_t srcBlockBase = static_cast<std::size_t>((localX << 11) | (localZ << 7) | minY_);
                const std::size_t srcNibbleBase = srcBlockBase >> 1;
                const std::size_t dstColumn = static_cast<std::size_t>((localX << 4) | localZ);

                std::memcpy(copy.blocks.data() + dstColumn * static_cast<std::size_t>(ySpan_),
                    chunk.blocks.data() + srcBlockBase, static_cast<std::size_t>(ySpan_));
                std::memcpy(copy.meta.data() + dstColumn * static_cast<std::size_t>(halfSpan),
                    chunk.meta.bytes.data() + srcNibbleBase, static_cast<std::size_t>(halfSpan));
                std::memcpy(copy.skyLight.data() + dstColumn * static_cast<std::size_t>(halfSpan),
                    chunk.skyLight.bytes.data() + srcNibbleBase, static_cast<std::size_t>(halfSpan));
                std::memcpy(copy.blockLight.data() + dstColumn * static_cast<std::size_t>(halfSpan),
                    chunk.blockLight.bytes.data() + srcNibbleBase, static_cast<std::size_t>(halfSpan));
            }
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

RegionSnapshot::RegionSnapshot(std::span<const SourceChunk> sourceChunks, int ambientDarkness,
    const std::array<float, 16>& lightLevelToLuminance, std::unique_ptr<net::minecraft::BiomeSource> biomeSource,
    int minBlockX, int minBlockY, int minBlockZ, int maxBlockX, int maxBlockY, int maxBlockZ)
{
    chunkX_ = minBlockX >> 4;
    chunkZ_ = minBlockZ >> 4;
    const int maxChunkX = maxBlockX >> 4;
    const int maxChunkZ = maxBlockZ >> 4;
    chunkWidth_ = maxChunkX - chunkX_ + 1;
    chunkDepth_ = maxChunkZ - chunkZ_ + 1;
    chunks_.resize(static_cast<std::size_t>(chunkWidth_ * chunkDepth_));

    minY_ = std::clamp(minBlockY, 0, Chunk::height - 1) & ~1;
    const int maxY = std::clamp(maxBlockY, 0, Chunk::height - 1);
    int span = maxY >= minY_ ? maxY - minY_ + 1 : 0;
    span += span & 1;
    ySpan_ = std::min(span, Chunk::height - minY_);
    const int halfSpan = ySpan_ >> 1;

    constexpr std::size_t kFullBlockCount = 16 * 16 * static_cast<std::size_t>(Chunk::height);
    const std::size_t blockBytes = static_cast<std::size_t>(16 * 16 * ySpan_);
    const std::size_t nibbleBytes = static_cast<std::size_t>(16 * 16 * halfSpan);

    for (const SourceChunk& source : sourceChunks) {
        if (source.chunk == nullptr || source.chunkX < chunkX_ || source.chunkZ < chunkZ_
            || source.chunkX > maxChunkX || source.chunkZ > maxChunkZ) {
            continue;
        }

        const Chunk& chunk = *source.chunk;
        ChunkCopy& copy = chunks_[static_cast<std::size_t>(
            (source.chunkX - chunkX_) + (source.chunkZ - chunkZ_) * chunkWidth_)];
        copy.present = true;

        if (chunk.blocks.size() < kFullBlockCount || chunk.meta.bytes.size() < kFullBlockCount / 2
            || chunk.skyLight.bytes.size() < kFullBlockCount / 2
            || chunk.blockLight.bytes.size() < kFullBlockCount / 2) {
            copy.blocks.assign(blockBytes, 0);
            copy.meta.assign(nibbleBytes, 0);
            copy.skyLight.assign(nibbleBytes, 0);
            copy.blockLight.assign(nibbleBytes, 0);
            continue;
        }

        copy.blocks.resize(blockBytes);
        copy.meta.resize(nibbleBytes);
        copy.skyLight.resize(nibbleBytes);
        copy.blockLight.resize(nibbleBytes);

        for (int column = 0; column < 16 * 16; ++column) {
            const int localX = column >> 4;
            const int localZ = column & 0xF;
            const std::size_t srcBlockBase = static_cast<std::size_t>((localX << 11) | (localZ << 7) | minY_);
            const std::size_t srcNibbleBase = srcBlockBase >> 1;
            const std::size_t dstColumn = static_cast<std::size_t>((localX << 4) | localZ);

            std::memcpy(copy.blocks.data() + dstColumn * static_cast<std::size_t>(ySpan_),
                chunk.blocks.data() + srcBlockBase, static_cast<std::size_t>(ySpan_));
            std::memcpy(copy.meta.data() + dstColumn * static_cast<std::size_t>(halfSpan),
                chunk.meta.bytes.data() + srcNibbleBase, static_cast<std::size_t>(halfSpan));
            std::memcpy(copy.skyLight.data() + dstColumn * static_cast<std::size_t>(halfSpan),
                chunk.skyLight.bytes.data() + srcNibbleBase, static_cast<std::size_t>(halfSpan));
            std::memcpy(copy.blockLight.data() + dstColumn * static_cast<std::size_t>(halfSpan),
                chunk.blockLight.bytes.data() + srcNibbleBase, static_cast<std::size_t>(halfSpan));
        }
    }

    ambientDarkness_ = ambientDarkness;
    lightLevelToLuminance_ = lightLevelToLuminance;
    biomeSource_ = std::move(biomeSource);
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
    return nibbleAt(chunk->meta, x & 0xF, y, z & 0xF);
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
    int sky = nibbleAt(chunk->skyLight, x & 0xF, y, z & 0xF);
    if (sky > 0) {
        sawSkyLight_ = true;
    }
    const int block = nibbleAt(chunk->blockLight, x & 0xF, y, z & 0xF);
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
