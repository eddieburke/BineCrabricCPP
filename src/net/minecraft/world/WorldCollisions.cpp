#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LiquidBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft {
namespace {
struct BlockScanBounds {
 int minX;
 int maxX;
 int minY;
 int maxY;
 int minZ;
 int maxZ;
};
[[nodiscard]] bool isValidBlockId(int blockId) {
 return blockId > 0 && blockId < static_cast<int>(Block::BLOCK_COUNT);
}
[[nodiscard]] Block* resolveBlock(int blockId) {
 return isValidBlockId(blockId) ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
}
[[nodiscard]] Block* resolveBlock(const World& world, int x, int y, int z) {
 return resolveBlock(world.getBlockId(x, y, z));
}
[[nodiscard]] BlockScanBounds blockScanBounds(const Box& box) {
 return BlockScanBounds{
     .minX = MathHelper::floor(box.minX),
     .maxX = MathHelper::floor(box.maxX + 1.0),
     .minY = MathHelper::floor(box.minY) - 1,
     .maxY = MathHelper::floor(box.maxY + 1.0),
     .minZ = MathHelper::floor(box.minZ),
     .maxZ = MathHelper::floor(box.maxZ + 1.0),
 };
}
[[nodiscard]] BlockScanBounds blockScanBoundsWithNegativePadding(const Box& box) {
 BlockScanBounds bounds = blockScanBounds(box);
 if(box.minX < 0.0) {
  --bounds.minX;
 }
 if(box.minY < 0.0) {
  --bounds.minY;
 }
 if(box.minZ < 0.0) {
  --bounds.minZ;
 }
 return bounds;
}
template <typename Fn>
bool forEachBlockInBounds(const World& world, const BlockScanBounds& bounds, Fn&& fn) {
 for(int x = bounds.minX; x < bounds.maxX; ++x) {
  for(int y = bounds.minY; y < bounds.maxY; ++y) {
   for(int z = bounds.minZ; z < bounds.maxZ; ++z) {
    if(Block* block = resolveBlock(world, x, y, z); block != nullptr && fn(x, y, z, *block)) {
     return true;
    }
   }
  }
 }
 return false;
}
template <typename Fn>
void forEachLoadedBlockInBounds(const World& world, const BlockScanBounds& bounds, Fn&& fn) {
 for(int x = bounds.minX; x < bounds.maxX; ++x) {
  for(int z = bounds.minZ; z < bounds.maxZ; ++z) {
   if(!world.isPosLoaded(x, 64, z)) {
    continue;
   }
   for(int y = bounds.minY; y < bounds.maxY; ++y) {
    if(Block* block = resolveBlock(world, x, y, z); block != nullptr) {
     fn(x, y, z, *block);
    }
   }
  }
 }
}
} // namespace
bool World::shouldSuffocate(int x, int y, int z) const {
 if(Block* block = resolveBlock(*this, x, y, z); block != nullptr) {
  return block->material.suffocates() && block->isFullCube();
 }
 return false;
}
bool World::isAnyBlockInBox(const Box& box) const {
 int minX = MathHelper::floor(box.minX);
 int maxX = MathHelper::floor(box.maxX + 1.0);
 int minY = MathHelper::floor(box.minY);
 int maxY = MathHelper::floor(box.maxY + 1.0);
 int minZ = MathHelper::floor(box.minZ);
 int maxZ = MathHelper::floor(box.maxZ + 1.0);
 if(box.minX < 0.0) {
  --minX;
 }
 if(box.minY < 0.0) {
  --minY;
 }
 if(box.minZ < 0.0) {
  --minZ;
 }
 for(int x = minX; x < maxX; ++x) {
  for(int y = minY; y < maxY; ++y) {
   for(int z = minZ; z < maxZ; ++z) {
    if(resolveBlock(*this, x, y, z) != nullptr) {
     return true;
    }
   }
  }
 }
 return false;
}
bool World::isAir(int x, int y, int z) const {
 return getBlockId(x, y, z) == 0;
}
block::material::Material& World::getMaterial(int x, int y, int z) const {
 if(Block* block = resolveBlock(*this, x, y, z); block != nullptr) {
  return block->material;
 }
 return block::material::Material::AIR;
}
bool World::isSolidBlock(int x, int y, int z) const {
 if(Block* block = resolveBlock(*this, x, y, z); block != nullptr) {
  return block->material.blocksMovement();
 }
 return false;
}
bool World::isBlockOpaqueCube(int x, int y, int z) const {
 if(Block* block = resolveBlock(*this, x, y, z); block != nullptr) {
  return block->isOpaque();
 }
 return false;
}
std::vector<Box> World::getEntityCollisions(Entity* except, const Box& box) {
 std::vector<Box> result;
 BlockScanBounds bounds = blockScanBounds(box);
 --bounds.minY;
 forEachLoadedBlockInBounds(*this, bounds, [&](int x, int y, int z, Block& block) {
  block.addIntersectingBoundingBox(this, x, y, z, box, result);
 });
 for(Entity* other : getEntities(except, box.expand(0.25))) {
  if(other == nullptr) {
   continue;
  }
  if(const std::optional<Box> otherBox = other->getBoundingBox()) {
   if(otherBox->intersects(box)) {
    result.push_back(*otherBox);
   }
  }
  if(except != nullptr) {
   if(const std::optional<Box> collisionShape = except->getCollisionAgainstShape(other)) {
    if(collisionShape->intersects(box)) {
     result.push_back(*collisionShape);
    }
   }
  }
 }
 return result;
}
std::vector<Box> World::getBlockCollisions(const Box& box) const {
 std::vector<Box> result;
 BlockScanBounds bounds = blockScanBounds(box);
 --bounds.minY;
 forEachLoadedBlockInBounds(*this, bounds, [&](int x, int y, int z, Block& block) {
  block.addIntersectingBoundingBox(const_cast<World*>(this), x, y, z, box, result);
 });
 return result;
}
bool World::canSpawnEntity(const Box& box) {
 for(Entity* entity : getEntities(nullptr, box)) {
  if(entity == nullptr || entity->dead || !entity->blocksSameBlockSpawning) {
   continue;
  }
  return false;
 }
 return true;
}
bool World::isMaterialInBox(const Box& boundingBox, block::material::Material& material) const {
 return forEachBlockInBounds(
     *this, blockScanBounds(boundingBox), [&](int /*x*/, int /*y*/, int /*z*/, Block& block) {
      return &block.material == &material;
     });
}
bool World::isFluidInBox(const Box& boundingBox, block::material::Material& fluid) const {
 return forEachBlockInBounds(*this, blockScanBounds(boundingBox), [&](int x, int y, int z, Block& block) {
  if(&block.material != &fluid) {
   return false;
  }
  const int meta = getBlockMeta(x, y, z);
  double fluidSurfaceY = static_cast<double>(y + 1);
  if(meta < 8) {
   fluidSurfaceY = static_cast<double>(y + 1) - static_cast<double>(meta) / 8.0;
  }
  return fluidSurfaceY >= boundingBox.minY;
 });
}
bool World::isFireOrLavaInBox(const Box& box) const {
 const BlockScanBounds bounds = blockScanBounds(box);
 if(!isRegionLoaded(bounds.minX, bounds.minY, bounds.minZ, bounds.maxX, bounds.maxY, bounds.maxZ)) {
  return false;
 }
 return forEachBlockInBounds(*this, bounds, [&](int /*x*/, int /*y*/, int /*z*/, Block& block) {
  return (Block::FIRE != nullptr && &block == Block::FIRE) ||
         (Block::FLOWING_LAVA != nullptr && &block == Block::FLOWING_LAVA) ||
         (Block::LAVA != nullptr && &block == Block::LAVA);
 });
}
bool World::updateMovementInFluid(const Box& entityBoundingBox,
                                  block::material::Material& fluidMaterial,
                                  Entity* entity) {
 const BlockScanBounds bounds = blockScanBounds(entityBoundingBox);
 if(!isRegionLoaded(bounds.minX, bounds.minY, bounds.minZ, bounds.maxX, bounds.maxY, bounds.maxZ)) {
  return false;
 }
 bool inFluid = false;
 Vec3d flowVelocity{};
 forEachBlockInBounds(*this, bounds, [&](int x, int y, int z, Block& block) {
  if(&block.material != &fluidMaterial) {
   return false;
  }
  const double fluidSurface = static_cast<double>(
      static_cast<float>(y + 1) - block::LiquidBlock::getFluidHeightFromMeta(getBlockMeta(x, y, z)));
  if(static_cast<double>(bounds.maxY) < fluidSurface) {
   return false;
  }
  inFluid = true;
  if(entity != nullptr) {
   block.applyVelocity(this, x, y, z, entity, flowVelocity);
  }
  return false;
 });
 if(entity != nullptr) {
  const double flowLength = std::sqrt(flowVelocity.x * flowVelocity.x + flowVelocity.y * flowVelocity.y +
                                      flowVelocity.z * flowVelocity.z);
  if(flowLength > 0.0) {
   const Vec3d normalized = flowVelocity.normalize();
   constexpr double flowPush = 0.014;
   entity->velocityX += normalized.x * flowPush;
   entity->velocityY += normalized.y * flowPush;
   entity->velocityZ += normalized.z * flowPush;
  }
 }
 return inFluid;
}
bool World::isBoxSubmergedInFluid(const Box& box) const {
 return forEachBlockInBounds(
     *this, blockScanBoundsWithNegativePadding(box), [&](int /*x*/, int /*y*/, int /*z*/, Block& block) {
      return block.material.isFluid();
     });
}
void World::extinguishFire(PlayerEntity* /*player*/, int x, int y, int z, int direction) {
 if(direction == 0) {
  --y;
 } else if(direction == 1) {
  ++y;
 } else if(direction == 2) {
  --z;
 } else if(direction == 3) {
  ++z;
 } else if(direction == 4) {
  --x;
 } else if(direction == 5) {
  ++x;
 }
 if(Block::FIRE != nullptr && getBlockId(x, y, z) == Block::FIRE->id) {
  JavaRandom& rng = random();
  playSound(static_cast<double>(x) + 0.5,
            static_cast<double>(y) + 0.5,
            static_cast<double>(z) + 0.5,
            "random.fizz",
            0.5f,
            2.6f + (rng.nextFloat() - rng.nextFloat()) * 0.8f);
  setBlock(x, y, z, 0);
 }
}
std::optional<HitResult> World::raycast(const Vec3d& start,
                                        const Vec3d& end,
                                        bool ignoreLiquids,
                                        bool ignoreNonFullBlocks) const {
 if(std::isnan(start.x) || std::isnan(start.y) || std::isnan(start.z) || std::isnan(end.x) || std::isnan(end.y) ||
    std::isnan(end.z)) {
  return std::nullopt;
 }
 int x = MathHelper::floor(start.x);
 int y = MathHelper::floor(start.y);
 int z = MathHelper::floor(start.z);
 const int endX = MathHelper::floor(end.x);
 const int endY = MathHelper::floor(end.y);
 const int endZ = MathHelper::floor(end.z);
 const double dx = end.x - start.x;
 const double dy = end.y - start.y;
 const double dz = end.z - start.z;
 double tMaxX = 0.0;
 double tMaxY = 0.0;
 double tMaxZ = 0.0;
 double tDeltaX = 0.0;
 double tDeltaY = 0.0;
 double tDeltaZ = 0.0;
 int stepX = 0;
 int stepY = 0;
 int stepZ = 0;
 if(dx > 0.0) {
  stepX = 1;
  tMaxX = ((static_cast<double>(x) + 1.0) - start.x) / dx;
  tDeltaX = 1.0 / dx;
 } else if(dx < 0.0) {
  stepX = -1;
  tMaxX = (start.x - static_cast<double>(x)) / -dx;
  tDeltaX = 1.0 / -dx;
 } else {
  tMaxX = std::numeric_limits<double>::infinity();
  tDeltaX = std::numeric_limits<double>::infinity();
 }
 if(dy > 0.0) {
  stepY = 1;
  tMaxY = ((static_cast<double>(y) + 1.0) - start.y) / dy;
  tDeltaY = 1.0 / dy;
 } else if(dy < 0.0) {
  stepY = -1;
  tMaxY = (start.y - static_cast<double>(y)) / -dy;
  tDeltaY = 1.0 / -dy;
 } else {
  tMaxY = std::numeric_limits<double>::infinity();
  tDeltaY = std::numeric_limits<double>::infinity();
 }
 if(dz > 0.0) {
  stepZ = 1;
  tMaxZ = ((static_cast<double>(z) + 1.0) - start.z) / dz;
  tDeltaZ = 1.0 / dz;
 } else if(dz < 0.0) {
  stepZ = -1;
  tMaxZ = (start.z - static_cast<double>(z)) / -dz;
  tDeltaZ = 1.0 / -dz;
 } else {
  tMaxZ = std::numeric_limits<double>::infinity();
  tDeltaZ = std::numeric_limits<double>::infinity();
 }
 for(int i = 0; i < 200; ++i) {
  if(Block* block = resolveBlock(*this, x, y, z); block != nullptr) {
   const int meta = getBlockMeta(x, y, z);
   const std::optional<Box> collisionShape = block->getCollisionShape(const_cast<World*>(this), x, y, z);
   const bool hasShape = collisionShape.has_value();
   if((!ignoreNonFullBlocks || hasShape) && block->hasCollision(meta, ignoreLiquids)) {
    if(const std::optional<HitResult> hit = block->raycast(const_cast<World*>(this), x, y, z, start, end);
       hit.has_value()) {
     return hit;
    }
   }
  }
  if(x == endX && y == endY && z == endZ) {
   return std::nullopt;
  }
  if(tMaxX < tMaxY) {
   if(tMaxX < tMaxZ) {
    x += stepX;
    tMaxX += tDeltaX;
   } else {
    z += stepZ;
    tMaxZ += tDeltaZ;
   }
  } else if(tMaxY < tMaxZ) {
   y += stepY;
   tMaxY += tDeltaY;
  } else {
   z += stepZ;
   tMaxZ += tDeltaZ;
  }
 }
 return std::nullopt;
}
float World::getVisibilityRatio(const Vec3d& vec, const Box& box) const {
 const double stepX = 1.0 / ((box.maxX - box.minX) * 2.0 + 1.0);
 const double stepY = 1.0 / ((box.maxY - box.minY) * 2.0 + 1.0);
 const double stepZ = 1.0 / ((box.maxZ - box.minZ) * 2.0 + 1.0);
 int visible = 0;
 int total = 0;
 for(float fx = 0.0f; fx <= 1.0f; fx = static_cast<float>(static_cast<double>(fx) + stepX)) {
  for(float fy = 0.0f; fy <= 1.0f; fy = static_cast<float>(static_cast<double>(fy) + stepY)) {
   for(float fz = 0.0f; fz <= 1.0f; fz = static_cast<float>(static_cast<double>(fz) + stepZ)) {
    const double sampleX = box.minX + (box.maxX - box.minX) * static_cast<double>(fx);
    const double sampleY = box.minY + (box.maxY - box.minY) * static_cast<double>(fy);
    const double sampleZ = box.minZ + (box.maxZ - box.minZ) * static_cast<double>(fz);
    if(!raycast(Vec3d{sampleX, sampleY, sampleZ}, vec).has_value()) {
     ++visible;
    }
    ++total;
   }
  }
 }
 if(total == 0) {
  return 0.0f;
 }
 return static_cast<float>(visible) / static_cast<float>(total);
}
} // namespace net::minecraft
