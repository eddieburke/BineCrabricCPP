#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/ProjectileUtil.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include <cmath>
namespace net::minecraft::entity::mob {
SkeletonEntity::SkeletonEntity(World* world) : MonsterEntity(world) {
  texture = "/mob/skeleton.png";
}
void SkeletonEntity::tickMovement() {
  if(world != nullptr && world->canMonsterSpawn()) {
    const float brightness = getBrightnessAtEyes(1.0f);
    if(brightness > 0.5f && world->hasSkyLight(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z)) &&
       random.nextFloat() * 30.0f < (brightness - 0.4f) * 2.0f) {
      fireTicks = 300;
    }
  }
  MonsterEntity::tickMovement();
}
void SkeletonEntity::dropItems() {
  if(world != nullptr && world->isRemote()) {
    return;
  }
  int count = random.nextInt(3);
  const int arrowId = Item::byRawId(6) != nullptr ? Item::byRawId(6)->id : 262;
  for(int i = 0; i < count; ++i) {
    dropItem(arrowId, 1);
  }
  count = random.nextInt(3);
  const int boneId = Item::byRawId(96) != nullptr ? Item::byRawId(96)->id : 352;
  for(int i = 0; i < count; ++i) {
    dropItem(boneId, 1);
  }
}
void SkeletonEntity::attack(Entity* other, float distance) {
  if(world == nullptr || world->isRemote() || other == nullptr) {
    return;
  }
  if(distance >= 10.0f) {
    return;
  }
  const double deltaX = other->x - x;
  const double deltaZ = other->z - z;
  if(attackCooldown == 0) {
    auto* arrow = new projectile::ArrowEntity(world, this);
    arrow->y += 1.4;
    const double deltaY = other->y + static_cast<double>(other->getEyeHeight()) - 0.2 - arrow->y;
    const float spread = MathHelper::sqrt(static_cast<float>(deltaX * deltaX + deltaZ * deltaZ)) * 0.2f;
    world->playSound(this, "random.bow", 1.0f, 1.0f / (random.nextFloat() * 0.4f + 0.8f));
    world->spawnEntity(arrow);
    projectile::setProjectileVelocity(*arrow, deltaX, deltaY + static_cast<double>(spread), deltaZ, 0.6f, 12.0f);
    attackCooldown = 30;
  }
  yaw = static_cast<float>(std::atan2(deltaZ, deltaX) * 180.0 / 3.141592653589793) - 90.0f;
  movementBlocked = true;
}
MC_REGISTER_ENTITY(SkeletonEntity)
} // namespace net::minecraft::entity::mob
