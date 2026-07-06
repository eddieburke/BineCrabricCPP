#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include <cmath>
#include <memory>
#include <optional>
namespace net::minecraft::entity::projectile {
namespace {
[[nodiscard]] Entity* findHookedEntity(FishingBobberEntity& bobber, const Vec3d& start, const Vec3d& end) {
  if(bobber.world == nullptr || bobber.owner == nullptr) {
    return nullptr;
  }
  const Box searchBox = bobber.boundingBox.stretch(end.x - start.x, end.y - start.y, end.z - start.z).expand(1.0);
  const std::vector<Entity*> candidates = bobber.world->getEntities(&bobber, searchBox);
  Entity* closest = nullptr;
  double closestDist = 0.0;
  for(Entity* candidate : candidates) {
    if(candidate == nullptr || !candidate->isCollidable()) {
      continue;
    }
    if(candidate == bobber.owner && bobber.ignoresOwnerForHook()) {
      continue;
    }
    const Box hitBox = candidate->boundingBox.expand(0.3);
    if(!hitBox.intersects(searchBox)) {
      continue;
    }
    const double centerX = (hitBox.minX + hitBox.maxX) * 0.5;
    const double centerY = (hitBox.minY + hitBox.maxY) * 0.5;
    const double centerZ = (hitBox.minZ + hitBox.maxZ) * 0.5;
    const double dx = centerX - start.x;
    const double dy = centerY - start.y;
    const double dz = centerZ - start.z;
    const double dist = dx * dx + dy * dy + dz * dz;
    if(closest == nullptr || dist < closestDist) {
      closest = candidate;
      closestDist = dist;
    }
  }
  return closest;
}
void updateBobberRotation(FishingBobberEntity& bobber) {
  const float horizontal =
      MathHelper::sqrt(static_cast<float>(bobber.velocityX * bobber.velocityX + bobber.velocityZ * bobber.velocityZ));
  bobber.yaw = static_cast<float>(std::atan2(bobber.velocityX, bobber.velocityZ) * 180.0 / 3.141592653589793);
  bobber.pitch =
      static_cast<float>(std::atan2(bobber.velocityY, static_cast<double>(horizontal)) * 180.0 / 3.141592653589793);
  while(bobber.pitch - bobber.prevPitch < -180.0f) {
    bobber.prevPitch -= 360.0f;
  }
  while(bobber.pitch - bobber.prevPitch >= 180.0f) {
    bobber.prevPitch += 360.0f;
  }
  while(bobber.yaw - bobber.prevYaw < -180.0f) {
    bobber.prevYaw -= 360.0f;
  }
  while(bobber.yaw - bobber.prevYaw >= 180.0f) {
    bobber.prevYaw += 360.0f;
  }
  bobber.pitch = bobber.prevPitch + (bobber.pitch - bobber.prevPitch) * 0.2f;
  bobber.yaw = bobber.prevYaw + (bobber.yaw - bobber.prevYaw) * 0.2f;
}
} // namespace
FishingBobberEntity::FishingBobberEntity(World* world) : Entity(world) {
  setBoundingBoxSpacing(0.25f, 0.25f);
  ignoreFrustumCull = true;
}
FishingBobberEntity::FishingBobberEntity(World* world, player::PlayerEntity* thrower) : Entity(world) {
  ignoreFrustumCull = true;
  owner = thrower;
  if(owner != nullptr) {
    owner->fishHook = this;
  }
  setBoundingBoxSpacing(0.25f, 0.25f);
  if(thrower != nullptr) {
    setPositionAndAnglesKeepPrevAngles(thrower->x,
                                       thrower->y + 1.62 - static_cast<double>(thrower->standingEyeHeight),
                                       thrower->z, thrower->yaw, thrower->pitch);
    x -= static_cast<double>(MathHelper::cos(yaw / 180.0f * kPiF) * 0.16f);
    y -= 0.1;
    z -= static_cast<double>(MathHelper::sin(yaw / 180.0f * kPiF) * 0.16f);
    setPosition(x, y, z);
    standingEyeHeight = 0.0f;
    constexpr float spread = 0.4f;
    velocityX = -MathHelper::sin(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF) * spread;
    velocityZ = MathHelper::cos(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF) * spread;
    velocityY = -MathHelper::sin(pitch / 180.0f * kPiF) * spread;
    setProjectileVelocity(*this, velocityX, velocityY, velocityZ, 1.5f, 1.0f);
  }
}
void FishingBobberEntity::tick() {
  Entity::tick();
  if(world == nullptr) {
    return;
  }
  if(owner != nullptr && (owner->dead || !owner->isAlive())) {
    markDead();
    owner->fishHook = nullptr;
    return;
  }
  if(!world->isRemote() && owner != nullptr) {
    const ItemStack hand = owner->getHand();
    const int rodId = Item::byRawId(90) != nullptr ? Item::byRawId(90)->id : 346;
    if(hand.empty() || hand.itemId != rodId || getSquaredDistance(*owner) > 1024.0) {
      markDead();
      owner->fishHook = nullptr;
      return;
    }
    if(hookedEntity != nullptr) {
      if(!hookedEntity->dead) {
        x = hookedEntity->x;
        y = hookedEntity->boundingBox.minY + static_cast<double>(hookedEntity->height) * 0.8;
        z = hookedEntity->z;
        return;
      }
      hookedEntity = nullptr;
    }
  }
  if(shake > 0) {
    --shake;
  }
  if(inGround) {
    const int currentBlockId = world->getBlockId(blockX, blockY, blockZ);
    if(currentBlockId != blockId) {
      inGround = false;
      velocityX *= static_cast<double>(random.nextFloat() * 0.2f);
      velocityY *= static_cast<double>(random.nextFloat() * 0.2f);
      velocityZ *= static_cast<double>(random.nextFloat() * 0.2f);
      removalTimer = 0;
      inAirTime = 0;
    } else {
      ++removalTimer;
      if(removalTimer == 1200) {
        markDead();
      }
      return;
    }
  }
  ++inAirTime;
  const Vec3d start{x, y, z};
  const Vec3d end{x + velocityX, y + velocityY, z + velocityZ};
  std::optional<HitResult> hit = world->raycast(start, end);
  Vec3d clippedEnd = end;
  if(hit.has_value()) {
    clippedEnd = hit->pos;
  }
  Entity* entityHit = findHookedEntity(*this, start, clippedEnd);
  if(entityHit != nullptr) {
    hit = HitResult(entityHit, Vec3d{entityHit->x, entityHit->y, entityHit->z});
  }
  if(hit.has_value()) {
    if(hit->entity != nullptr && owner != nullptr) {
      if(hit->entity->damage(owner, 0)) {
        hookedEntity = hit->entity;
      }
    } else {
      inGround = true;
      blockX = hit->blockX;
      blockY = hit->blockY;
      blockZ = hit->blockZ;
      blockId = world->getBlockId(blockX, blockY, blockZ);
    }
  }
  if(inGround) {
    return;
  }
  move(velocityX, velocityY, velocityZ);
  updateBobberRotation(*this);
  float drag = 0.92f;
  if(onGround || horizontalCollision) {
    drag = 0.5f;
  }
  double waterFraction = 0.0;
  constexpr int samples = 5;
  for(int i = 0; i < samples; ++i) {
    const double sampleMinY =
        boundingBox.minY +
        (boundingBox.maxY - boundingBox.minY) * static_cast<double>(i) / static_cast<double>(samples) - 0.125 +
        0.125;
    const double sampleMaxY =
        boundingBox.minY +
        (boundingBox.maxY - boundingBox.minY) * static_cast<double>(i + 1) / static_cast<double>(samples) - 0.125 +
        0.125;
    const Box probe{boundingBox.minX, sampleMinY, boundingBox.minZ, boundingBox.maxX, sampleMaxY, boundingBox.maxZ};
    if(world->isMaterialInBox(probe, block::material::Material::WATER)) {
      waterFraction += 1.0 / static_cast<double>(samples);
    }
  }
  if(waterFraction > 0.0) {
    if(hookCountdown > 0) {
      --hookCountdown;
    } else {
      int biteDelay = 500;
      if(world->isRaining(MathHelper::floor(x), MathHelper::floor(y) + 1, MathHelper::floor(z))) {
        biteDelay = 300;
      }
      if(random.nextInt(biteDelay) == 0) {
        hookCountdown = random.nextInt(30) + 10;
        velocityY -= 0.2;
        world->playSound(this, "random.splash", 0.25f, 1.0f + (random.nextFloat() - random.nextFloat()) * 0.4f);
        const float splashY = static_cast<float>(MathHelper::floor(boundingBox.minY));
        for(float particle = 0.0f; particle < 1.0f + width * 20.0f; particle += 1.0f) {
          const float offsetX = (random.nextFloat() * 2.0f - 1.0f) * width;
          const float offsetZ = (random.nextFloat() * 2.0f - 1.0f) * width;
          world->addParticle("bubble", x + static_cast<double>(offsetX), static_cast<double>(splashY) + 1.0f,
                             z + static_cast<double>(offsetZ), velocityX,
                             velocityY - static_cast<double>(random.nextFloat() * 0.2f), velocityZ);
        }
        for(float particle = 0.0f; particle < 1.0f + width * 20.0f; particle += 1.0f) {
          const float offsetX = (random.nextFloat() * 2.0f - 1.0f) * width;
          const float offsetZ = (random.nextFloat() * 2.0f - 1.0f) * width;
          world->addParticle("splash", x + static_cast<double>(offsetX), static_cast<double>(splashY) + 1.0f,
                             z + static_cast<double>(offsetZ), velocityX, velocityY, velocityZ);
        }
      }
    }
  }
  if(hookCountdown > 0) {
    velocityY -= static_cast<double>(random.nextFloat() * random.nextFloat() * random.nextFloat()) * 0.2;
  }
  const double buoyancy = waterFraction * 2.0 - 1.0;
  velocityY += 0.04 * buoyancy;
  if(waterFraction > 0.0) {
    drag = static_cast<float>(static_cast<double>(drag) * 0.9);
    velocityY *= 0.8;
  }
  velocityX *= drag;
  velocityY *= drag;
  velocityZ *= drag;
  setPosition(x, y, z);
}
void FishingBobberEntity::writeNbt(NbtCompound& nbt) const {
  Entity::writeNbt(nbt);
  nbt.putShort("xTile", static_cast<std::int16_t>(blockX));
  nbt.putShort("yTile", static_cast<std::int16_t>(blockY));
  nbt.putShort("zTile", static_cast<std::int16_t>(blockZ));
  nbt.putByte("inTile", static_cast<std::int8_t>(blockId));
  nbt.putByte("shake", static_cast<std::int8_t>(shake));
  nbt.putByte("inGround", static_cast<std::int8_t>(inGround ? 1 : 0));
}
void FishingBobberEntity::readNbt(const NbtCompound& nbt) {
  Entity::readNbt(nbt);
  blockX = nbt.getShort("xTile");
  blockY = nbt.getShort("yTile");
  blockZ = nbt.getShort("zTile");
  blockId = static_cast<int>(nbt.getByte("inTile")) & 0xFF;
  shake = static_cast<int>(nbt.getByte("shake")) & 0xFF;
  inGround = nbt.getByte("inGround") == 1;
}
int FishingBobberEntity::use() {
  int result = 0;
  if(hookedEntity != nullptr && owner != nullptr) {
    const double dx = owner->x - x;
    const double dy = owner->y - y;
    const double dz = owner->z - z;
    const double dist = MathHelper::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));
    constexpr double pull = 0.1;
    hookedEntity->velocityX += dx * pull;
    hookedEntity->velocityY += dy * pull + static_cast<double>(MathHelper::sqrt(static_cast<float>(dist))) * 0.08;
    hookedEntity->velocityZ += dz * pull;
    result = 3;
  } else if(hookCountdown > 0 && world != nullptr && owner != nullptr) {
    const int fishId = Item::byRawId(93) != nullptr ? Item::byRawId(93)->id : 349;
    auto item = std::make_unique<ItemEntity>(world, x, y, z, ItemStack(fishId, 1, 0));
    const double dx = owner->x - x;
    const double dy = owner->y - y;
    const double dz = owner->z - z;
    const double dist = MathHelper::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));
    constexpr double pull = 0.1;
    item->velocityX = dx * pull;
    item->velocityY = dy * pull + static_cast<double>(MathHelper::sqrt(static_cast<float>(dist))) * 0.08;
    item->velocityZ = dz * pull;
    if(world->spawnEntity(item.get())) {
      item.release();
      owner->increaseStat(stat::Stats::FISH_CAUGHT.id, 1);
      result = 1;
    }
  } else if(inGround) {
    result = 2;
  }
  markDead();
  if(owner != nullptr) {
    owner->fishHook = nullptr;
  }
  return result;
}
MC_REGISTER_ENTITY(FishingBobberEntity)
} // namespace net::minecraft::entity::projectile
