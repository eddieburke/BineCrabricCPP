#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/RailBlock.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtList.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include <cmath>
namespace net::minecraft::entity::vehicle {
namespace {
constexpr int kAdjacentRailPositions[10][2][3] = {
    {{0, 0, -1}, {0, 0, 1}},
    {{-1, 0, 0}, {1, 0, 0}},
    {{-1, -1, 0}, {1, 0, 0}},
    {{-1, 0, 0}, {1, -1, 0}},
    {{0, 0, -1}, {0, -1, 1}},
    {{0, -1, -1}, {0, 0, 1}},
    {{0, 0, 1}, {1, 0, 0}},
    {{0, 0, 1}, {-1, 0, 0}},
    {{0, 0, -1}, {-1, 0, 0}},
    {{0, 0, -1}, {1, 0, 0}},
};
void dropStackWithVelocity(World* world, double xIn, double yIn, double zIn, ItemStack stack, JavaRandom& random) {
  if(world == nullptr || stack.empty()) {
    return;
  }
  while(stack.count > 0) {
    int split = random.nextInt(21) + 10;
    if(split > stack.count) {
      split = stack.count;
    }
    ItemStack dropped = stack;
    dropped.count = split;
    stack.count -= split;
    const float spread = 0.8f;
    const float offsetX = random.nextFloat() * spread + 0.1f;
    const float offsetY = random.nextFloat() * spread + 0.1f;
    const float offsetZ = random.nextFloat() * spread + 0.1f;
    auto* itemEntity = new ItemEntity(world, xIn + static_cast<double>(offsetX), yIn + static_cast<double>(offsetY),
                                      zIn + static_cast<double>(offsetZ), std::move(dropped));
    constexpr float velocitySpread = 0.05f;
    itemEntity->velocityX = static_cast<float>(random.nextGaussian() * velocitySpread);
    itemEntity->velocityY = static_cast<float>(random.nextGaussian() * velocitySpread + 0.2);
    itemEntity->velocityZ = static_cast<float>(random.nextGaussian() * velocitySpread);
    world->spawnEntity(itemEntity);
  }
}
} // namespace
MinecartEntity::MinecartEntity(World* world) : Entity(world) {
  inventory_.assign(36, {});
  blocksSameBlockSpawning = true;
  setBoundingBoxSpacing(0.98f, 0.7f);
  standingEyeHeight = height / 2.0f;
}
MinecartEntity::MinecartEntity(World* world, double xIn, double yIn, double zIn, int typeIn) : MinecartEntity(world) {
  setPosition(xIn, yIn + static_cast<double>(standingEyeHeight), zIn);
  velocityX = 0.0;
  velocityY = 0.0;
  velocityZ = 0.0;
  prevX = xIn;
  prevY = yIn;
  prevZ = zIn;
  type = typeIn;
}
std::optional<Box> MinecartEntity::getCollisionAgainstShape(Entity* other) const {
  if(other == nullptr) {
    return std::nullopt;
  }
  return other->boundingBox;
}
bool MinecartEntity::canPlayerUse(PlayerEntity* player) const {
  if(dead || player == nullptr) {
    return false;
  }
  return player->getSquaredDistance(*this) <= 64.0;
}
ItemStack MinecartEntity::getStack(std::size_t slot) const {
  if(slot >= inventory_.size()) {
    return {};
  }
  return inventory_[slot];
}
ItemStack MinecartEntity::removeStack(std::size_t slot, int amount) {
  if(slot >= inventory_.size() || inventory_[slot].empty()) {
    return {};
  }
  if(inventory_[slot].count <= amount) {
    ItemStack removed = inventory_[slot];
    inventory_[slot] = {};
    return removed;
  }
  ItemStack removed = inventory_[slot];
  removed.count = amount;
  inventory_[slot].count -= amount;
  if(inventory_[slot].count == 0) {
    inventory_[slot] = {};
  }
  return removed;
}
void MinecartEntity::setStack(std::size_t slot, ItemStack stack) {
  if(slot >= inventory_.size()) {
    return;
  }
  inventory_[slot] = std::move(stack);
  if(!inventory_[slot].empty() && inventory_[slot].count > getMaxCountPerStack()) {
    inventory_[slot].count = getMaxCountPerStack();
  }
}
void MinecartEntity::dropInventoryContents() {
  if(world == nullptr) {
    return;
  }
  for(const ItemStack& stack : inventory_) {
    if(stack.empty()) {
      continue;
    }
    dropStackWithVelocity(world, x, y, z, stack, random);
  }
}
void MinecartEntity::markDead() {
  dropInventoryContents();
  Entity::markDead();
}
bool MinecartEntity::damage(Entity* damageSource, int amount) {
  (void)damageSource;
  if(world == nullptr || world->isRemote() || dead) {
    return true;
  }
  damageWobbleSide = -damageWobbleSide;
  damageWobbleTicks = 10;
  scheduleVelocityUpdate();
  damageWobbleStrength += static_cast<float>(amount * 10);
  if(damageWobbleStrength > 40.0f) {
    if(passenger != nullptr) {
      passenger->setVehicle(this);
    }
    markDead();
    const int minecartId = Item::byRawId(72) != nullptr ? Item::byRawId(72)->id : 328;
    dropItem(minecartId, 1, 0.0f);
    if(type == 1) {
      dropInventoryContents();
      const int chestId = Block::CHEST != nullptr ? Block::CHEST->id : 54;
      dropItem(chestId, 1, 0.0f);
    } else if(type == 2) {
      const int furnaceId = Block::FURNACE != nullptr ? Block::FURNACE->id : 61;
      dropItem(furnaceId, 1, 0.0f);
    }
  }
  return true;
}
std::optional<Vec3d> MinecartEntity::snapPositionToRail(double xIn, double yIn, double zIn) const {
  if(world == nullptr) {
    return std::nullopt;
  }
  int blockX = MathHelper::floor(xIn);
  int blockY = MathHelper::floor(yIn);
  int blockZ = MathHelper::floor(zIn);
  if(block::RailBlock::isRail(world, blockX, blockY - 1, blockZ)) {
    --blockY;
  }
  const int railId = world->getBlockId(blockX, blockY, blockZ);
  if(!block::RailBlock::isRail(railId)) {
    return std::nullopt;
  }
  int meta = world->getBlockMeta(blockX, blockY, blockZ);
  auto* rail = dynamic_cast<block::RailBlock*>(Block::BLOCKS[static_cast<std::size_t>(railId)]);
  if(rail != nullptr && rail->alwaysStraight) {
    meta &= 7;
  }
  const int (&points)[2][3] = kAdjacentRailPositions[meta];
  double deltaX = static_cast<double>(points[1][0] - points[0][0]);
  double deltaY = static_cast<double>(points[1][1] - points[0][1]) * 2.0;
  double deltaZ = static_cast<double>(points[1][2] - points[0][2]);
  double progress = 0.0;
  const double startX = static_cast<double>(blockX) + 0.5 + static_cast<double>(points[0][0]) * 0.5;
  const double startY = static_cast<double>(blockY) + 0.5 + static_cast<double>(points[0][1]) * 0.5;
  const double startZ = static_cast<double>(blockZ) + 0.5 + static_cast<double>(points[0][2]) * 0.5;
  const double endX = static_cast<double>(blockX) + 0.5 + static_cast<double>(points[1][0]) * 0.5;
  const double endY = static_cast<double>(blockY) + 0.5 + static_cast<double>(points[1][1]) * 0.5;
  const double endZ = static_cast<double>(blockZ) + 0.5 + static_cast<double>(points[1][2]) * 0.5;
  deltaX = endX - startX;
  deltaY = endY - startY;
  deltaZ = endZ - startZ;
  double outX = xIn;
  double outY = yIn;
  double outZ = zIn;
  if(deltaX == 0.0) {
    outX = static_cast<double>(blockX) + 0.5;
    progress = zIn - static_cast<double>(blockZ);
  } else if(deltaZ == 0.0) {
    outZ = static_cast<double>(blockZ) + 0.5;
    progress = xIn - static_cast<double>(blockX);
  } else {
    const double offsetX = xIn - startX;
    const double offsetZ = zIn - startZ;
    progress = (offsetX * deltaX + offsetZ * deltaZ) * 2.0;
  }
  outX = startX + deltaX * progress;
  outY = startY + deltaY * progress;
  outZ = startZ + deltaZ * progress;
  if(deltaY < 0.0) {
    outY += 1.0;
  }
  if(deltaY > 0.0) {
    outY += 0.5;
  }
  return Vec3d{outX, outY, outZ};
}
std::optional<Vec3d> MinecartEntity::snapPositionToRailWithOffset(double xIn, double yIn, double zIn,
                                                                  double offset) const {
  if(world == nullptr) {
    return std::nullopt;
  }
  int blockX = MathHelper::floor(xIn);
  int blockY = MathHelper::floor(yIn);
  int blockZ = MathHelper::floor(zIn);
  if(block::RailBlock::isRail(world, blockX, blockY - 1, blockZ)) {
    --blockY;
  }
  const int railId = world->getBlockId(blockX, blockY, blockZ);
  if(!block::RailBlock::isRail(railId)) {
    return std::nullopt;
  }
  int meta = world->getBlockMeta(blockX, blockY, blockZ);
  auto* rail = dynamic_cast<block::RailBlock*>(Block::BLOCKS[static_cast<std::size_t>(railId)]);
  if(rail != nullptr && rail->alwaysStraight) {
    meta &= 7;
  }
  double railY = static_cast<double>(blockY);
  if(meta >= 2 && meta <= 5) {
    railY = static_cast<double>(blockY + 1);
  }
  const int (&points)[2][3] = kAdjacentRailPositions[meta];
  double deltaX = static_cast<double>(points[1][0] - points[0][0]);
  double deltaZ = static_cast<double>(points[1][2] - points[0][2]);
  double segmentLength = std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
  if(segmentLength == 0.0) {
    return std::nullopt;
  }
  double x = xIn;
  double y = railY;
  double z = zIn;
  deltaX /= segmentLength;
  deltaZ /= segmentLength;
  x += deltaX * offset;
  z += deltaZ * offset;
  if(points[0][1] != 0 && MathHelper::floor(x) - blockX == points[0][0] &&
     MathHelper::floor(z) - blockZ == points[0][2]) {
    y += static_cast<double>(points[0][1]);
  } else if(points[1][1] != 0 && MathHelper::floor(x) - blockX == points[1][0] &&
            MathHelper::floor(z) - blockZ == points[1][2]) {
    y += static_cast<double>(points[1][1]);
  }
  return snapPositionToRail(x, y, z);
}
void MinecartEntity::tick() {
  if(world == nullptr) {
    return;
  }
  if(damageWobbleTicks > 0) {
    --damageWobbleTicks;
  }
  if(damageWobbleStrength > 0.0f) {
    --damageWobbleStrength;
  }
  if(world->isRemote() && clientInterpolationSteps > 0) {
    double yawDelta = clientPitch - static_cast<double>(yaw);
    while(yawDelta < -180.0) {
      yawDelta += 360.0;
    }
    while(yawDelta >= 180.0) {
      yawDelta -= 360.0;
    }
    const double interpX = x + (clientX - x) / static_cast<double>(clientInterpolationSteps);
    const double interpY = y + (clientY - y) / static_cast<double>(clientInterpolationSteps);
    const double interpZ = z + (clientZ - z) / static_cast<double>(clientInterpolationSteps);
    yaw = static_cast<float>(static_cast<double>(yaw) + yawDelta / static_cast<double>(clientInterpolationSteps));
    pitch = static_cast<float>(static_cast<double>(pitch) + (clientYaw - static_cast<double>(pitch)) /
                                                                static_cast<double>(clientInterpolationSteps));
    --clientInterpolationSteps;
    setPosition(interpX, interpY, interpZ);
    setRotation(yaw, pitch);
    return;
  }
  prevX = x;
  prevY = y;
  prevZ = z;
  velocityY -= 0.04;
  int blockX = MathHelper::floor(x);
  int blockY = MathHelper::floor(y);
  int blockZ = MathHelper::floor(z);
  if(block::RailBlock::isRail(world, blockX, blockY - 1, blockZ)) {
    --blockY;
  }
  constexpr double maxSpeed = 0.4;
  constexpr double slopeAccel = 0.0078125;
  bool furnaceSmoke = false;
  const int railId = world->getBlockId(blockX, blockY, blockZ);
  if(block::RailBlock::isRail(railId)) {
    const std::optional<Vec3d> beforeSnap = snapPositionToRail(x, y, z);
    int meta = world->getBlockMeta(blockX, blockY, blockZ);
    y = static_cast<double>(blockY);
    bool powered = false;
    bool unpoweredBrake = false;
    if(Block::POWERED_RAIL != nullptr && railId == Block::POWERED_RAIL->id) {
      powered = (meta & 8) != 0;
      unpoweredBrake = !powered;
    }
    auto* rail = dynamic_cast<block::RailBlock*>(Block::BLOCKS[static_cast<std::size_t>(railId)]);
    if(rail != nullptr && rail->alwaysStraight) {
      meta &= 7;
    }
    if(meta >= 2 && meta <= 5) {
      y = static_cast<double>(blockY + 1);
    }
    if(meta == 2) {
      velocityX -= slopeAccel;
    }
    if(meta == 3) {
      velocityX += slopeAccel;
    }
    if(meta == 4) {
      velocityZ += slopeAccel;
    }
    if(meta == 5) {
      velocityZ -= slopeAccel;
    }
    const int (&points)[2][3] = kAdjacentRailPositions[meta];
    double deltaX = static_cast<double>(points[1][0] - points[0][0]);
    double deltaZ = static_cast<double>(points[1][2] - points[0][2]);
    double segmentLength = std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
    double velocityAlong = velocityX * deltaX + velocityZ * deltaZ;
    if(velocityAlong < 0.0) {
      deltaX = -deltaX;
      deltaZ = -deltaZ;
    }
    const double horizontalSpeed = std::sqrt(velocityX * velocityX + velocityZ * velocityZ);
    velocityX = horizontalSpeed * deltaX / segmentLength;
    velocityZ = horizontalSpeed * deltaZ / segmentLength;
    if(unpoweredBrake) {
      const double speed = std::sqrt(velocityX * velocityX + velocityZ * velocityZ);
      if(speed < 0.03) {
        velocityX = 0.0;
        velocityY = 0.0;
        velocityZ = 0.0;
      } else {
        velocityX *= 0.5;
        velocityY = 0.0;
        velocityZ *= 0.5;
      }
    }
    double progress = 0.0;
    const double startX = static_cast<double>(blockX) + 0.5 + static_cast<double>(points[0][0]) * 0.5;
    const double startZ = static_cast<double>(blockZ) + 0.5 + static_cast<double>(points[0][2]) * 0.5;
    const double endX = static_cast<double>(blockX) + 0.5 + static_cast<double>(points[1][0]) * 0.5;
    const double endZ = static_cast<double>(blockZ) + 0.5 + static_cast<double>(points[1][2]) * 0.5;
    deltaX = endX - startX;
    deltaZ = endZ - startZ;
    if(deltaX == 0.0) {
      x = static_cast<double>(blockX) + 0.5;
      progress = z - static_cast<double>(blockZ);
    } else if(deltaZ == 0.0) {
      z = static_cast<double>(blockZ) + 0.5;
      progress = x - static_cast<double>(blockX);
    } else {
      const double offsetX = x - startX;
      const double offsetZ = z - startZ;
      progress = (offsetX * deltaX + offsetZ * deltaZ) * 2.0;
    }
    x = startX + deltaX * progress;
    z = startZ + deltaZ * progress;
    setPosition(x, y + static_cast<double>(standingEyeHeight), z);
    double moveX = velocityX;
    double moveZ = velocityZ;
    if(passenger != nullptr) {
      moveX *= 0.75;
      moveZ *= 0.75;
    }
    moveX = std::clamp(moveX, -maxSpeed, maxSpeed);
    moveZ = std::clamp(moveZ, -maxSpeed, maxSpeed);
    move(moveX, 0.0, moveZ);
    if(points[0][1] != 0 && MathHelper::floor(x) - blockX == points[0][0] &&
       MathHelper::floor(z) - blockZ == points[0][2]) {
      setPosition(x, y + static_cast<double>(points[0][1]), z);
    } else if(points[1][1] != 0 && MathHelper::floor(x) - blockX == points[1][0] &&
              MathHelper::floor(z) - blockZ == points[1][2]) {
      setPosition(x, y + static_cast<double>(points[1][1]), z);
    }
    if(passenger != nullptr) {
      velocityX *= 0.997;
      velocityY = 0.0;
      velocityZ *= 0.997;
    } else {
      if(type == 2) {
        const double pushLength = MathHelper::sqrt(pushX * pushX + pushZ * pushZ);
        if(pushLength > 0.01) {
          furnaceSmoke = true;
          pushX /= pushLength;
          pushZ /= pushLength;
          velocityX *= 0.8;
          velocityY = 0.0;
          velocityZ *= 0.8;
          velocityX += pushX * 0.04;
          velocityZ += pushZ * 0.04;
        } else {
          velocityX *= 0.9;
          velocityY = 0.0;
          velocityZ *= 0.9;
        }
      }
      velocityX *= 0.96;
      velocityY = 0.0;
      velocityZ *= 0.96;
    }
    const std::optional<Vec3d> afterSnap = snapPositionToRail(x, y, z);
    if(afterSnap.has_value() && beforeSnap.has_value()) {
      const double heightBoost = (beforeSnap->y - afterSnap->y) * 0.05;
      const double speed = std::sqrt(velocityX * velocityX + velocityZ * velocityZ);
      if(speed > 0.0) {
        velocityX = velocityX / speed * (speed + heightBoost);
        velocityZ = velocityZ / speed * (speed + heightBoost);
      }
      setPosition(x, afterSnap->y, z);
    }
    const int newBlockX = MathHelper::floor(x);
    const int newBlockZ = MathHelper::floor(z);
    if(newBlockX != blockX || newBlockZ != blockZ) {
      const double speed = std::sqrt(velocityX * velocityX + velocityZ * velocityZ);
      velocityX = speed * static_cast<double>(newBlockX - blockX);
      velocityZ = speed * static_cast<double>(newBlockZ - blockZ);
    }
    if(type == 2) {
      const double pushLength = MathHelper::sqrt(pushX * pushX + pushZ * pushZ);
      if(pushLength > 0.01 && velocityX * velocityX + velocityZ * velocityZ > 0.001) {
        pushX /= pushLength;
        pushZ /= pushLength;
        if(pushX * velocityX + pushZ * velocityZ < 0.0) {
          pushX = 0.0;
          pushZ = 0.0;
        } else {
          pushX = velocityX;
          pushZ = velocityZ;
        }
      }
    }
    if(powered) {
      const double speed = std::sqrt(velocityX * velocityX + velocityZ * velocityZ);
      if(speed > 0.01) {
        constexpr double boost = 0.06;
        velocityX += velocityX / speed * boost;
        velocityZ += velocityZ / speed * boost;
      } else if(meta == 1) {
        if(world->shouldSuffocate(blockX - 1, blockY, blockZ)) {
          velocityX = 0.02;
        } else if(world->shouldSuffocate(blockX + 1, blockY, blockZ)) {
          velocityX = -0.02;
        }
      } else if(meta == 0) {
        if(world->shouldSuffocate(blockX, blockY, blockZ - 1)) {
          velocityZ = 0.02;
        } else if(world->shouldSuffocate(blockX, blockY, blockZ + 1)) {
          velocityZ = -0.02;
        }
      }
    }
  } else {
    velocityX = std::clamp(velocityX, -maxSpeed, maxSpeed);
    velocityZ = std::clamp(velocityZ, -maxSpeed, maxSpeed);
    if(onGround) {
      velocityX *= 0.5;
      velocityY *= 0.5;
      velocityZ *= 0.5;
    }
    move(velocityX, velocityY, velocityZ);
    if(!onGround) {
      velocityX *= 0.95;
      velocityY *= 0.95;
      velocityZ *= 0.95;
    }
  }
  pitch = 0.0f;
  const double deltaXPos = prevX - x;
  const double deltaZPos = prevZ - z;
  if(deltaXPos * deltaXPos + deltaZPos * deltaZPos > 0.001) {
    yaw = static_cast<float>(std::atan2(deltaZPos, deltaXPos) * 180.0 / 3.141592653589793);
    if(yawFlipped) {
      yaw += 180.0f;
    }
  }
  double yawDelta = static_cast<double>(yaw - prevYaw);
  while(yawDelta >= 180.0) {
    yawDelta -= 360.0;
  }
  while(yawDelta < -180.0) {
    yawDelta += 360.0;
  }
  if(yawDelta < -170.0 || yawDelta >= 170.0) {
    yaw += 180.0f;
    yawFlipped = !yawFlipped;
  }
  setRotation(yaw, pitch);
  const Box collisionBox = boundingBox.expand(0.2f, 0.0, 0.2f);
  for(Entity* entity : world->getEntities(this, collisionBox)) {
    if(entity == passenger || entity == nullptr || !entity->isPushable()) {
      continue;
    }
    if(dynamic_cast<MinecartEntity*>(entity) != nullptr) {
      entity->onCollision(this);
    }
  }
  if(passenger != nullptr && passenger->dead) {
    passenger = nullptr;
  }
  if(furnaceSmoke && random.nextInt(4) == 0) {
    --fuel;
    if(fuel < 0) {
      pushX = 0.0;
      pushZ = 0.0;
    }
    world->addParticle("largesmoke", x, y + 0.8, z, 0.0, 0.0, 0.0);
  }
}
void MinecartEntity::onCollision(Entity* otherEntity) {
  if(world == nullptr || world->isRemote() || otherEntity == nullptr || otherEntity == passenger) {
    return;
  }
  if(dynamic_cast<LivingEntity*>(otherEntity) != nullptr &&
     dynamic_cast<player::PlayerEntity*>(otherEntity) == nullptr && type == 0 &&
     velocityX * velocityX + velocityZ * velocityZ > 0.01 && passenger == nullptr &&
     otherEntity->vehicle == nullptr) {
    otherEntity->setVehicle(this);
  }
  const double deltaX = otherEntity->x - x;
  const double deltaZ = otherEntity->z - z;
  double distanceSq = deltaX * deltaX + deltaZ * deltaZ;
  if(distanceSq < 1.0e-4) {
    return;
  }
  double distance = MathHelper::sqrt(distanceSq);
  double normX = deltaX / distance;
  double normZ = deltaZ / distance;
  double push = 1.0 / distance;
  if(push > 1.0) {
    push = 1.0;
  }
  normX *= push;
  normZ *= push;
  normX *= 0.1;
  normZ *= 0.1;
  normX *= static_cast<double>(1.0f - pushSpeedReduction);
  normZ *= static_cast<double>(1.0f - pushSpeedReduction);
  normX *= 0.5;
  normZ *= 0.5;
  auto* otherCart = dynamic_cast<MinecartEntity*>(otherEntity);
  if(otherCart != nullptr) {
    const double cross = (otherEntity->x - x) * otherEntity->velocityZ + (otherEntity->z - z) * otherEntity->prevX;
    if(cross * cross > 5.0) {
      return;
    }
    double combinedX = otherEntity->velocityX + velocityX;
    double combinedZ = otherEntity->velocityZ + velocityZ;
    if(otherCart->type == 2 && type != 2) {
      velocityX *= 0.2;
      velocityZ *= 0.2;
      addVelocity(otherEntity->velocityX - normX, 0.0, otherEntity->velocityZ - normZ);
      otherEntity->velocityX *= 0.7;
      otherEntity->velocityZ *= 0.7;
    } else if(otherCart->type != 2 && type == 2) {
      otherEntity->velocityX *= 0.2;
      otherEntity->velocityZ *= 0.2;
      otherEntity->addVelocity(velocityX + normX, 0.0, velocityZ + normZ);
      velocityX *= 0.7;
      velocityZ *= 0.7;
    } else {
      velocityX *= 0.2;
      velocityZ *= 0.2;
      combinedX /= 2.0;
      combinedZ /= 2.0;
      addVelocity(combinedX - normX, 0.0, combinedZ - normZ);
      otherEntity->velocityX *= 0.2;
      otherEntity->velocityZ *= 0.2;
      otherEntity->addVelocity(combinedX + normX, 0.0, combinedZ + normZ);
    }
  } else {
    addVelocity(-normX, 0.0, -normZ);
    otherEntity->addVelocity(normX / 4.0, 0.0, normZ / 4.0);
  }
}
bool MinecartEntity::interact(player::PlayerEntity* player) {
  if(player == nullptr || world == nullptr) {
    return false;
  }
  if(type == 0) {
    if(passenger != nullptr && dynamic_cast<player::PlayerEntity*>(passenger) != nullptr && passenger != player) {
      return true;
    }
    if(!world->isRemote()) {
      player->setVehicle(this);
    }
  } else if(type == 1) {
    if(!world->isRemote()) {
      player->openChestScreen(this);
    }
  } else if(type == 2) {
    ItemStack* hand = player->inventory.getSelectedItem();
    if(hand != nullptr && Item::byRawId(7) != nullptr && hand->itemId == Item::byRawId(7)->id) {
      --hand->count;
      if(hand->count == 0) {
        player->inventory.setStack(static_cast<std::size_t>(player->inventory.selectedSlot), {});
      }
      fuel += 1200;
    }
    pushX = x - player->x;
    pushZ = z - player->z;
  }
  return true;
}
void MinecartEntity::writeNbt(NbtCompound& nbt) const {
  Entity::writeNbt(nbt);
  nbt.putInt("Type", type);
  if(type == 2) {
    nbt.putDouble("PushX", pushX);
    nbt.putDouble("PushZ", pushZ);
    nbt.putShort("Fuel", static_cast<std::int16_t>(fuel));
  } else if(type == 1) {
    NbtList items;
    auto& list = items.storage().asList();
    for(std::size_t i = 0; i < inventory_.size(); ++i) {
      if(inventory_[i].empty()) {
        continue;
      }
      NbtCompound entry = inventory_[i].toNbt();
      entry.putByte("Slot", static_cast<std::int8_t>(i));
      list.push_back(entry.storage());
    }
    nbt.put("Items", items);
  }
}
void MinecartEntity::setPositionAndAnglesAvoidEntities(double xIn, double yIn, double zIn, float yawIn, float pitchIn,
                                                       int interpolationSteps) {
  clientX = xIn;
  clientY = yIn;
  clientZ = zIn;
  clientPitch = static_cast<double>(yawIn);
  clientYaw = static_cast<double>(pitchIn);
  clientInterpolationSteps = interpolationSteps + 2;
  velocityX = clientVelocityX;
  velocityY = clientVelocityY;
  velocityZ = clientVelocityZ;
}
void MinecartEntity::setVelocityClient(double xIn, double yIn, double zIn) {
  clientVelocityX = xIn;
  clientVelocityY = yIn;
  clientVelocityZ = zIn;
  velocityX = xIn;
  velocityY = yIn;
  velocityZ = zIn;
}
void MinecartEntity::readNbt(const NbtCompound& nbt) {
  Entity::readNbt(nbt);
  type = nbt.getInt("Type");
  if(type == 2) {
    pushX = nbt.getDouble("PushX");
    pushZ = nbt.getDouble("PushZ");
    fuel = nbt.getShort("Fuel");
  } else if(type == 1) {
    inventory_.assign(inventory_.size(), {});
    if(nbt.contains("Items")) {
      const NbtList items = nbt.getList("Items");
      for(const Nbt& entryTag : items.entries()) {
        if(!entryTag.isCompound()) {
          continue;
        }
        const NbtCompound entry(entryTag);
        const int slot = entry.getByte("Slot") & 0xFF;
        if(slot >= 0 && slot < static_cast<int>(inventory_.size())) {
          inventory_[static_cast<std::size_t>(slot)] = ItemStack::fromNbt(entry);
        }
      }
    }
  }
}
MC_REGISTER_ENTITY(MinecartEntity)
} // namespace net::minecraft::entity::vehicle
