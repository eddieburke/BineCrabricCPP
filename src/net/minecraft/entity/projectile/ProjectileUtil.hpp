#pragma once
#include <cmath>
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::entity::projectile {
void tickThrownProjectile(Entity& projectile,
                          LivingEntity* owner,
                          int& inAirTime,
                          int& blockX,
                          int& blockY,
                          int& blockZ,
                          int& blockId,
                          bool& inGround,
                          int& removalTimer,
                          int& shake,
                          bool spawnChickens);
void tickArrowProjectile(Entity& projectile,
                         LivingEntity*& owner,
                         int& inAirTime,
                         int& blockX,
                         int& blockY,
                         int& blockZ,
                         int& blockId,
                         int& blockMeta,
                         bool& inGround,
                         int& life,
                         int& shake,
                         bool pickupAllowed);
inline void setProjectileVelocity(
    Entity& entity, double dirX, double dirY, double dirZ, float speed, float divergence) {
 float length = MathHelper::sqrt(static_cast<float>(dirX * dirX + dirY * dirY + dirZ * dirZ));
 if(length < 1.0e-4f) {
  return;
 }
 dirX /= static_cast<double>(length);
 dirY /= static_cast<double>(length);
 dirZ /= static_cast<double>(length);
 dirX += entity.random.nextGaussian() * 0.0075 * static_cast<double>(divergence);
 dirY += entity.random.nextGaussian() * 0.0075 * static_cast<double>(divergence);
 dirZ += entity.random.nextGaussian() * 0.0075 * static_cast<double>(divergence);
 entity.velocityX = dirX * static_cast<double>(speed);
 entity.velocityY = dirY * static_cast<double>(speed);
 entity.velocityZ = dirZ * static_cast<double>(speed);
 const float horizontal =
     MathHelper::sqrt(static_cast<float>(entity.velocityX * entity.velocityX + entity.velocityZ * entity.velocityZ));
 entity.prevYaw = entity.yaw =
     static_cast<float>(std::atan2(entity.velocityX, entity.velocityZ) * 180.0 / 3.141592653589793);
 entity.prevPitch = entity.pitch =
     static_cast<float>(std::atan2(entity.velocityY, static_cast<double>(horizontal)) * 180.0 / 3.141592653589793);
}
} // namespace net::minecraft::entity::projectile
