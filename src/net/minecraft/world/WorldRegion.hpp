#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/LightType.hpp"
#include <algorithm>
#include <vector>
namespace net::minecraft {
class BiomeSource;
class World;
// Faithful port of net.minecraft.world.WorldRegion (beta 1.7.3).
class WorldRegion : public BlockView {
public:
  WorldRegion(World* world, int minX, int minY, int minZ, int maxX, int maxY, int maxZ) : world_(world) {
    (void)minY;
    (void)maxY;
    chunkX_ = chunk_coord(minX);
    chunkZ_ = chunk_coord(minZ);
    const int maxChunkX = chunk_coord(maxX);
    const int maxChunkZ = chunk_coord(maxZ);
    chunkWidth_ = maxChunkX - chunkX_ + 1;
    chunkDepth_ = maxChunkZ - chunkZ_ + 1;
    chunks_.assign(static_cast<std::size_t>(chunkWidth_ * chunkDepth_), nullptr);
    if(world_ == nullptr) {
      return;
    }
    for(int cx = chunkX_; cx <= maxChunkX; ++cx) {
      for(int cz = chunkZ_; cz <= maxChunkZ; ++cz) {
        Chunk& chunk = world_->getChunk(cx, cz);
        chunks_[static_cast<std::size_t>((cx - chunkX_) + (cz - chunkZ_) * chunkWidth_)] = &chunk;
      }
    }
  }
  [[nodiscard]] int getBlockId(int x, int y, int z) const override {
    if(y < 0 || y >= Chunk::height) {
      return 0;
    }
    const Chunk* chunk = getChunkAt(x, z);
    if(chunk == nullptr) {
      return 0;
    }
    return chunk->getBlockId(x & 0xF, y, z & 0xF);
  }
  block::entity::BlockEntity* getBlockEntity(int x, int y, int z) override {
    const int localX = chunk_coord(x) - chunkX_;
    const int localZ = chunk_coord(z) - chunkZ_;
    if(localX < 0 || localZ < 0 || localX >= chunkWidth_ || localZ >= chunkDepth_) {
      return nullptr;
    }
    Chunk* chunk = chunks_[static_cast<std::size_t>(localX + localZ * chunkWidth_)];
    if(chunk == nullptr) {
      return nullptr;
    }
    return chunk->getBlockEntity(x & 0xF, y, z & 0xF);
  }
  [[nodiscard]] float getNaturalBrightness(int x, int y, int z, int blockLight) const override {
    int brightness = getRawBrightness(x, y, z);
    if(brightness < blockLight) {
      brightness = blockLight;
    }
    if(world_ == nullptr || world_->dimension == nullptr) {
      return Dimension::luminanceForLightLevel(brightness);
    }
    return world_->dimension->lightLevelToLuminance[static_cast<std::size_t>(brightness)];
  }
  [[nodiscard]] float getLightBrightness(int i, int j, int k) const override {
    if(world_ == nullptr || world_->dimension == nullptr) {
      return Dimension::luminanceForLightLevel(getRawBrightness(i, j, k));
    }
    return world_->dimension->lightLevelToLuminance[static_cast<std::size_t>(getRawBrightness(i, j, k))];
  }
  [[nodiscard]] int getRawBrightness(int x, int y, int z) const {
    return getRawBrightness(x, y, z, true);
  }
  [[nodiscard]] int getRawBrightness(int x, int y, int z, bool useNeighborLight) const {
    if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
      return 15;
    }
    if(world_ == nullptr) {
      return 0;
    }
    if(useNeighborLight && Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
      int brightness = getRawBrightness(x, y + 1, z, false);
      brightness = std::max(brightness, getRawBrightness(x + 1, y, z, false));
      brightness = std::max(brightness, getRawBrightness(x - 1, y, z, false));
      brightness = std::max(brightness, getRawBrightness(x, y, z + 1, false));
      brightness = std::max(brightness, getRawBrightness(x, y, z - 1, false));
      return brightness;
    }
    if(y < 0) {
      return 0;
    }
    if(y >= Chunk::height) {
      int brightness = 15 - world_->ambientDarkness;
      return brightness < 0 ? 0 : brightness;
    }
    const Chunk* chunk = getChunkAt(x, z);
    if(chunk == nullptr) {
      return 0;
    }
    return chunk->getLight(x & 0xF, y, z & 0xF, world_->ambientDarkness);
  }
  [[nodiscard]] int getBlockLight(int x, int y, int z) const {
    if(world_ == nullptr) {
      return 0;
    }
    return world_->getBrightness(LightType::Block, x, y, z);
  }
  [[nodiscard]] int getSkyLight(int x, int y, int z) const {
    if(world_ == nullptr) {
      return 0;
    }
    int sky = world_->getBrightness(LightType::Sky, x, y, z);
    return sky < 0 ? 0 : sky;
  }
  [[nodiscard]] int getBlockMeta(int x, int y, int z) const override {
    if(y < 0 || y >= Chunk::height) {
      return 0;
    }
    const Chunk* chunk = getChunkAt(x, z);
    if(chunk == nullptr) {
      return 0;
    }
    return chunk->getBlockMeta(x & 0xF, y, z & 0xF);
  }
  [[nodiscard]] block::material::Material& getMaterial(int x, int y, int z) const override {
    const int blockId = getBlockId(x, y, z);
    if(blockId == 0) {
      return block::material::Material::AIR;
    }
    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if(block == nullptr) {
      return block::material::Material::AIR;
    }
    return block->material;
  }
  [[nodiscard]] bool isBlockOpaqueCube(int i, int j, int k) const override {
    Block* block = Block::BLOCKS[static_cast<std::size_t>(getBlockId(i, j, k))];
    if(block == nullptr) {
      return false;
    }
    return block->isOpaque();
  }
  [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override {
    Block* block = Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
    if(block == nullptr) {
      return false;
    }
    return block->material.blocksMovement() && block->isFullCube();
  }
  BiomeSource* getBiomeSource() const override {
    if(world_ == nullptr || world_->dimension == nullptr || !world_->dimension->biomeSource) {
      return nullptr;
    }
    return world_->dimension->biomeSource.get();
  }

private:
  [[nodiscard]] Chunk* getChunkAt(int x, int z) const {
    const int localX = chunk_coord(x) - chunkX_;
    const int localZ = chunk_coord(z) - chunkZ_;
    if(localX < 0 || localZ < 0 || localX >= chunkWidth_ || localZ >= chunkDepth_) {
      return nullptr;
    }
    return chunks_[static_cast<std::size_t>(localX + localZ * chunkWidth_)];
  }
  int chunkX_ = 0;
  int chunkZ_ = 0;
  int chunkWidth_ = 0;
  int chunkDepth_ = 0;
  std::vector<Chunk*> chunks_;
  World* world_ = nullptr;
};
} // namespace net::minecraft
