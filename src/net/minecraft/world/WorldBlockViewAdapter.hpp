#pragma once

#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

#include <algorithm>

namespace net::minecraft {

// Adapts World to BlockView for BlockRenderManager (matches Java using World as BlockView).
class WorldBlockViewAdapter : public BlockView {
public:
    explicit WorldBlockViewAdapter(World* world)
        : world_(world)
    {
    }

    [[nodiscard]] int getBlockId(int x, int y, int z) const override
    {
        return world_ != nullptr ? world_->getBlockId(x, y, z) : 0;
    }

    block::entity::BlockEntity* getBlockEntity(int x, int y, int z) override
    {
        return world_ != nullptr ? world_->getBlockEntity(x, y, z) : nullptr;
    }

    [[nodiscard]] float getNaturalBrightness(int x, int y, int z, int blockLight) const override
    {
        int brightness = getRawBrightness(x, y, z);
        if (brightness < blockLight) {
            brightness = blockLight;
        }
        if (world_ == nullptr || world_->dimension == nullptr) {
            return Dimension::luminanceForLightLevel(brightness);
        }
        return world_->dimension->lightLevelToLuminance[static_cast<std::size_t>(brightness)];
    }

    [[nodiscard]] float getLightBrightness(int x, int y, int z) const override
    {
        if (world_ == nullptr || world_->dimension == nullptr) {
            return Dimension::luminanceForLightLevel(getRawBrightness(x, y, z));
        }
        return world_->dimension->lightLevelToLuminance[static_cast<std::size_t>(getRawBrightness(x, y, z))];
    }

    [[nodiscard]] int getBlockMeta(int x, int y, int z) const override
    {
        return world_ != nullptr ? static_cast<int>(world_->getBlockMeta(x, y, z)) : 0;
    }

    [[nodiscard]] block::material::Material& getMaterial(int x, int y, int z) const override
    {
        const int blockId = getBlockId(x, y, z);
        if (blockId == 0) {
            return block::material::Material::AIR;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block == nullptr) {
            return block::material::Material::AIR;
        }
        return block->material;
    }

    [[nodiscard]] bool isBlockOpaqueCube(int x, int y, int z) const override
    {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
        if (block == nullptr) {
            return false;
        }
        return block->isOpaque();
    }

    [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override
    {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
        if (block == nullptr) {
            return false;
        }
        return block->material.blocksMovement() && block->isFullCube();
    }

    BiomeSource* getBiomeSource() const override
    {
        if (world_ == nullptr || world_->dimension == nullptr || !world_->dimension->biomeSource) {
            return nullptr;
        }
        return world_->dimension->biomeSource.get();
    }

private:
    [[nodiscard]] int getRawBrightness(int x, int y, int z, bool useNeighborLight = true) const
    {
        if (x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
            return 15;
        }
        if (world_ == nullptr) {
            return 0;
        }
        if (useNeighborLight && Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
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
            int brightness = 15 - world_->ambientDarkness;
            return brightness < 0 ? 0 : brightness;
        }
        const Chunk* chunk = world_->getChunkIfLoaded(x, z);
        if (chunk == nullptr) {
            return 0;
        }
        return chunk->getLight(x & 0xF, y, z & 0xF, world_->ambientDarkness);
    }

    World* world_ = nullptr;
};

} // namespace net::minecraft
