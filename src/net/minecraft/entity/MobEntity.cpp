#include "net/minecraft/entity/MobEntity.hpp"
#include <cmath>
#include <memory>
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity {
void MobEntity::attack(Entity* /*other*/, float /*distance*/) {
}
void MobEntity::resetAttack(Entity* /*other*/, float /*distance*/) {
}
void MobEntity::pathingUpdate() {
  if(world == nullptr) {
    return;
  }
  int bestX = -1;
  int bestY = -1;
  int bestZ = -1;
  float bestFavor = -99999.0f;
  bool found = false;
  for(int i = 0; i < 10; ++i) {
    const int sampleX = MathHelper::floor(x + static_cast<double>(random.nextInt(13) - 6));
    const int sampleY = MathHelper::floor(y + static_cast<double>(random.nextInt(7) - 3));
    const int sampleZ = MathHelper::floor(z + static_cast<double>(random.nextInt(13) - 6));
    const float favor = getPathfindingFavor(sampleX, sampleY, sampleZ);
    if(favor > bestFavor) {
      bestFavor = favor;
      bestX = sampleX;
      bestY = sampleY;
      bestZ = sampleZ;
      found = true;
    }
  }
  if(found) {
    ai::pathing::Path candidate = world->findPath(this, bestX, bestY, bestZ, 10.0f);
    if(candidate.length > 0) {
      path = std::make_unique<ai::pathing::Path>(std::move(candidate));
    } else {
      path.reset();
    }
  }
}
void MobEntity::tickLiving() {
  movementBlocked = isMovementBlocked();
  constexpr float targetRange = 16.0f;
  if(target == nullptr) {
    target = getTargetInRange();
    if(target != nullptr && world != nullptr) {
      ai::pathing::Path candidate = world->findPath(this, target, targetRange);
      if(candidate.length > 0) {
        path = std::make_unique<ai::pathing::Path>(std::move(candidate));
      } else {
        path.reset();
      }
    }
  } else if(!target->isAlive()) {
    target = nullptr;
  } else {
    const float distance = target->getDistance(*this);
    if(canSee(target)) {
      attack(target, distance);
    } else {
      resetAttack(target, distance);
    }
  }
  if(!movementBlocked && target != nullptr && (path == nullptr || random.nextInt(20) == 0)) {
    if(world != nullptr) {
      ai::pathing::Path candidate = world->findPath(this, target, targetRange);
      if(candidate.length > 0) {
        path = std::make_unique<ai::pathing::Path>(std::move(candidate));
      } else {
        path.reset();
      }
    }
  } else if(!movementBlocked && ((path == nullptr && random.nextInt(80) == 0) || random.nextInt(80) == 0)) {
    pathingUpdate();
  }
  const int floorY = MathHelper::floor(boundingBox.minY + 0.5);
  const bool submerged = isSubmergedInWater();
  const bool touchingLava = isTouchingLava();
  pitch = 0.0f;
  if(path == nullptr || random.nextInt(100) == 0) {
    LivingEntity::tickLiving();
    path.reset();
    return;
  }
  Vec3d nodePosition = path->getNodePosition(*this);
  const double reach = static_cast<double>(width * 2.0f);
  while(true) {
    const double dx = nodePosition.x - x;
    const double dz = nodePosition.z - z;
    if(dx * dx + dz * dz >= reach * reach) {
      break;
    }
    path->next();
    if(path->isFinished()) {
      nodePosition = {};
      path.reset();
      break;
    }
    nodePosition = path->getNodePosition(*this);
  }
  jumping = false;
  if(path != nullptr) {
    const double deltaX = nodePosition.x - x;
    const double deltaZ = nodePosition.z - z;
    const double deltaY = nodePosition.y - static_cast<double>(floorY);
    const float desiredYaw = static_cast<float>(std::atan2(deltaZ, deltaX) * 180.0 / 3.141592653589793) - 90.0f;
    forwardSpeed = movementSpeed;
    float deltaYaw = desiredYaw - yaw;
    while(deltaYaw < -180.0f) {
      deltaYaw += 360.0f;
    }
    while(deltaYaw >= 180.0f) {
      deltaYaw -= 360.0f;
    }
    if(deltaYaw > 30.0f) {
      deltaYaw = 30.0f;
    }
    if(deltaYaw < -30.0f) {
      deltaYaw = -30.0f;
    }
    yaw += deltaYaw;
    if(movementBlocked && target != nullptr) {
      const double targetDeltaX = target->x - x;
      const double targetDeltaZ = target->z - z;
      const float previousYaw = yaw;
      yaw = static_cast<float>(std::atan2(targetDeltaZ, targetDeltaX) * 180.0 / 3.141592653589793) - 90.0f;
      deltaYaw = (previousYaw - yaw + 90.0f) * static_cast<float>(kPiF / 180.0f);
      sidewaysSpeed = -MathHelper::sin(deltaYaw) * forwardSpeed;
      forwardSpeed = MathHelper::cos(deltaYaw) * forwardSpeed;
    }
    if(deltaY > 0.0) {
      jumping = true;
    }
  }
  if(target != nullptr) {
    lookAt(target, 30.0f, 30.0f);
  }
  if(horizontalCollision && !hasPath()) {
    jumping = true;
  }
  if(random.nextFloat() < 0.8f && (submerged || touchingLava)) {
    jumping = true;
  }
}
} // namespace net::minecraft::entity
