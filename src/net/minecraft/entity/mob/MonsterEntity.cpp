#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity::mob {
void MonsterEntity::tickMovement() {
  if(getBrightnessAtEyes(1.0f) > 0.5f) {
    despawnCounter += 2;
  }
  LivingEntity::tickMovement();
}
void MonsterEntity::tick() {
  LivingEntity::tick();
  if(world != nullptr && !world->isRemote() && world->difficulty == 0) {
    markDead();
  }
}
bool MonsterEntity::damage(Entity* damageSource, int amount) {
  if(!LivingEntity::damage(damageSource, amount)) {
    return false;
  }
  if(passenger == damageSource || vehicle == damageSource) {
    return true;
  }
  if(damageSource != nullptr && damageSource != this) {
    target = damageSource;
  }
  return true;
}
Entity* MonsterEntity::getTargetInRange() {
  if(world == nullptr) {
    return nullptr;
  }
  PlayerEntity* player = world->getClosestPlayer(this, 16.0);
  if(player != nullptr && canSee(player)) {
    return player;
  }
  return nullptr;
}
void MonsterEntity::attack(Entity* other, float distance) {
  if(attackCooldown <= 0 && distance < 2.0f && other != nullptr && other->boundingBox.maxY > boundingBox.minY &&
     other->boundingBox.minY < boundingBox.maxY) {
    attackCooldown = 20;
    other->damage(this, attackDamage);
  }
}
float MonsterEntity::getPathfindingFavor(int x, int y, int z) const {
  if(world == nullptr) {
    return 0.0f;
  }
  return 0.5f - world->getLightBrightness(x, y, z);
}
MC_REGISTER_ENTITY(MonsterEntity)
} // namespace net::minecraft::entity::mob
