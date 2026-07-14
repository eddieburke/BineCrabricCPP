#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/projectile/ProjectileUtil.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity::projectile {
namespace {
Entity* findEntityOnPath(Entity& projectile, LivingEntity* owner, int inAirTime, const Vec3d& start, const Vec3d& end) {
  if(projectile.world == nullptr) {
    return nullptr;
  }
  const Box searchBox =
      projectile.boundingBox.stretch(end.x - start.x, end.y - start.y, end.z - start.z).expand(1.0, 1.0, 1.0);
  const std::vector<Entity*> candidates = projectile.world->getEntities(&projectile, searchBox);
  Entity* closest = nullptr;
  double closestDist = -1.0;
  for(Entity* candidate : candidates) {
    if(candidate == nullptr || !candidate->isCollidable()) {
      continue;
    }
    if(candidate == owner && inAirTime < 25) {
      continue;
    }
    const Box hitBox = candidate->boundingBox.expand(0.3, 0.3, 0.3);
    const std::optional<HitResult> entityHit = Block::raycastLocalBounds(
        hitBox.minX, hitBox.minY, hitBox.minZ, hitBox.maxX, hitBox.maxY, hitBox.maxZ, 0, 0, 0, start, end);
    if(!entityHit.has_value()) {
      continue;
    }
    const double dx = entityHit->pos.x - start.x;
    const double dy = entityHit->pos.y - start.y;
    const double dz = entityHit->pos.z - start.z;
    const double dist = dx * dx + dy * dy + dz * dz;
    if(!(dist < closestDist) && closestDist != -1.0) {
      continue;
    }
    closest = candidate;
    closestDist = dist;
  }
  return closest;
}
} // namespace
FireballEntity::FireballEntity(World* world) : Entity(world) {
  setBoundingBoxSpacing(1.0f, 1.0f);
}
FireballEntity::FireballEntity(
    World* world, double xIn, double yIn, double zIn, double velocityXIn, double velocityYIn, double velocityZIn)
    : Entity(world) {
  setBoundingBoxSpacing(1.0f, 1.0f);
  setPositionAndAnglesKeepPrevAngles(xIn, yIn, zIn, yaw, pitch);
  setPosition(x, y, z);
  const double length = MathHelper::sqrt(
      static_cast<float>(velocityXIn * velocityXIn + velocityYIn * velocityYIn + velocityZIn * velocityZIn));
  if(length > 1.0e-4) {
    powerX = velocityXIn / length * 0.1;
    powerY = velocityYIn / length * 0.1;
    powerZ = velocityZIn / length * 0.1;
  }
}
FireballEntity::FireballEntity(
    World* world, LivingEntity* ownerIn, double velocityXIn, double velocityYIn, double velocityZIn)
    : Entity(world) {
  owner = ownerIn;
  setBoundingBoxSpacing(1.0f, 1.0f);
  if(owner != nullptr) {
    setPositionAndAnglesKeepPrevAngles(owner->x, owner->y, owner->z, owner->yaw, owner->pitch);
    setPosition(x, y, z);
  }
  standingEyeHeight = 0.0f;
  velocityX = 0.0;
  velocityY = 0.0;
  velocityZ = 0.0;
  double vx = velocityXIn + random.nextGaussian() * 0.4;
  double vy = velocityYIn + random.nextGaussian() * 0.4;
  double vz = velocityZIn + random.nextGaussian() * 0.4;
  const double length = MathHelper::sqrt(static_cast<float>(vx * vx + vy * vy + vz * vz));
  if(length > 1.0e-4) {
    powerX = vx / length * 0.1;
    powerY = vy / length * 0.1;
    powerZ = vz / length * 0.1;
  }
}
void FireballEntity::tick() {
  Entity::tick();
  setPosition(x, y, z);
  fireTicks = 10;
  if(shake > 0) {
    --shake;
  }
  if(inGround) {
    const int currentBlock = world != nullptr ? world->getBlockId(blockX, blockY, blockZ) : 0;
    if(currentBlock != blockId) {
      inGround = false;
      velocityX *= random.nextFloat() * 0.2f;
      velocityY *= random.nextFloat() * 0.2f;
      velocityZ *= random.nextFloat() * 0.2f;
      removalTimer = 0;
      inAirTime = 0;
    } else {
      if(++removalTimer == 1200) {
        markDead();
      }
      return;
    }
  }
  ++inAirTime;
  const Vec3d start{x, y, z};
  const Vec3d end{x + velocityX, y + velocityY, z + velocityZ};
  std::optional<HitResult> hit = world != nullptr ? world->raycast(start, end) : std::nullopt;
  Vec3d hitPos = end;
  if(hit.has_value()) {
    hitPos = hit->pos;
  }
  if(Entity* entityHit = findEntityOnPath(*this, owner, inAirTime, start, hitPos); entityHit != nullptr) {
    hit = HitResult(entityHit, Vec3d{entityHit->x, entityHit->y, entityHit->z});
  }
  if(hit.has_value()) {
    if(world != nullptr && !world->isRemote()) {
      if(hit->entity != nullptr) {
        hit->entity->damage(owner, 0);
      }
      world->createExplosion(nullptr, x, y, z, 1.0f, true);
    }
    markDead();
    return;
  }
  x += velocityX;
  y += velocityY;
  z += velocityZ;
  const float horizontal = MathHelper::sqrt(static_cast<float>(velocityX * velocityX + velocityZ * velocityZ));
  yaw = static_cast<float>(std::atan2(velocityX, velocityZ) * 180.0 / 3.141592653589793);
  pitch = static_cast<float>(std::atan2(velocityY, static_cast<double>(horizontal)) * 180.0 / 3.141592653589793);
  while(pitch - prevPitch < -180.0f) {
    prevPitch -= 360.0f;
  }
  while(pitch - prevPitch >= 180.0f) {
    prevPitch += 360.0f;
  }
  while(yaw - prevYaw < -180.0f) {
    prevYaw -= 360.0f;
  }
  while(yaw - prevYaw >= 180.0f) {
    prevYaw += 360.0f;
  }
  pitch = prevPitch + (pitch - prevPitch) * 0.2f;
  yaw = prevYaw + (yaw - prevYaw) * 0.2f;
  float drag = 0.95f;
  if(isSubmergedInWater()) {
    if(world != nullptr) {
      for(int i = 0; i < 4; ++i) {
        constexpr float offset = 0.25f;
        world->addParticle("bubble",
                           x - velocityX * static_cast<double>(offset),
                           y - velocityY * static_cast<double>(offset),
                           z - velocityZ * static_cast<double>(offset),
                           velocityX,
                           velocityY,
                           velocityZ);
      }
    }
    drag = 0.8f;
  }
  velocityX += powerX;
  velocityY += powerY;
  velocityZ += powerZ;
  velocityX *= drag;
  velocityY *= drag;
  velocityZ *= drag;
  if(world != nullptr) {
    world->addParticle("smoke", x, y + 0.5, z, 0.0, 0.0, 0.0);
  }
  setPosition(x, y, z);
}
bool FireballEntity::damage(Entity* damageSource, int amount) {
  (void)amount;
  scheduleVelocityUpdate();
  if(damageSource != nullptr) {
    const Vec3d look = damageSource->getLookVector();
    velocityX = look.x;
    velocityY = look.y;
    velocityZ = look.z;
    powerX = velocityX * 0.1;
    powerY = velocityY * 0.1;
    powerZ = velocityZ * 0.1;
    return true;
  }
  return false;
}
void FireballEntity::writeNbt(NbtCompound& nbt) const {
  Entity::writeNbt(nbt);
  nbt.putShort("xTile", static_cast<std::int16_t>(blockX));
  nbt.putShort("yTile", static_cast<std::int16_t>(blockY));
  nbt.putShort("zTile", static_cast<std::int16_t>(blockZ));
  nbt.putByte("inTile", static_cast<std::int8_t>(blockId));
  nbt.putByte("shake", static_cast<std::int8_t>(shake));
  nbt.putByte("inGround", static_cast<std::int8_t>(inGround ? 1 : 0));
}
void FireballEntity::readNbt(const NbtCompound& nbt) {
  Entity::readNbt(nbt);
  blockX = nbt.getShort("xTile");
  blockY = nbt.getShort("yTile");
  blockZ = nbt.getShort("zTile");
  blockId = static_cast<int>(nbt.getByte("inTile")) & 0xFF;
  shake = static_cast<int>(nbt.getByte("shake")) & 0xFF;
  inGround = nbt.getByte("inGround") == 1;
}
MC_REGISTER_ENTITY(FireballEntity)
} // namespace net::minecraft::entity::projectile
