#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/MobEntity.hpp"
#include "net/minecraft/entity/SpawnableEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity::passive {
class AnimalEntity : public MobEntity, public SpawnableEntity {
public:
  explicit AnimalEntity(World* world = nullptr) : MobEntity(world) {
  }
  [[nodiscard]] float getPathfindingFavor(int x, int y, int z) const override {
    if(world && world->getBlockId(x, y - 1, z) == Block::GRASS_BLOCK->id) {
      return 10.0f;
    }
    return world ? world->getLightBrightness(x, y, z) - 0.5f : -0.5f;
  }
  [[nodiscard]] bool canSpawn() const override {
    if(!world)
      return false;
    const int x = MathHelper::floor(this->x);
    const int y = MathHelper::floor(boundingBox.minY);
    const int z = MathHelper::floor(this->z);
    return world->getBlockId(x, y - 1, z) == Block::GRASS_BLOCK->id && world->getBrightness(x, y, z) > 8 &&
           MobEntity::canSpawn();
  }
  [[nodiscard]] int getMinAmbientSoundDelay() const override {
    return 120;
  }
};
} // namespace net::minecraft::entity::passive
