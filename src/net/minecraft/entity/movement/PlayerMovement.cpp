#include "net/minecraft/entity/movement/PlayerMovement.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::movement {
namespace {
float groundSlipperiness(const LivingEntity& entity) {
    float slipperiness = 0.91f;
    if (!entity.onGround || entity.world == nullptr) {
        return slipperiness;
    }
    slipperiness = 0.54600006f;
    const int blockId = entity.world->getBlockId(
        MathHelper::floor(entity.x), MathHelper::floor(entity.boundingBox.minY) - 1, MathHelper::floor(entity.z));
    if (blockId > 0 && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr) {
        slipperiness = Block::BLOCKS[static_cast<std::size_t>(blockId)]->slipperiness * 0.91f;
    }
    return slipperiness;
}

void applyFluidWallClimb(LivingEntity& entity, double prevY) {
    if (!entity.horizontalCollision) {
        return;
    }
    if (!entity.getEntitiesInside(entity.velocityX, entity.velocityY + 0.6 - entity.y + prevY, entity.velocityZ)) {
        return;
    }
    entity.velocityY = 0.3;
}

void travelWater(LivingEntity& entity, float sideways, float forward, float speedMultiplier) {
    const double prevY = entity.y;
    entity.moveNonSolid(sideways, forward, 0.02f * speedMultiplier);
    entity.move(entity.velocityX, entity.velocityY, entity.velocityZ);
    entity.velocityX *= 0.8;
    entity.velocityY *= 0.8;
    entity.velocityZ *= 0.8;
    entity.velocityY -= 0.02;
    applyFluidWallClimb(entity, prevY);
}

void travelLava(LivingEntity& entity, float sideways, float forward, float speedMultiplier) {
    const double prevY = entity.y;
    entity.moveNonSolid(sideways, forward, 0.02f * speedMultiplier);
    entity.move(entity.velocityX, entity.velocityY, entity.velocityZ);
    entity.velocityX *= 0.5;
    entity.velocityY *= 0.5;
    entity.velocityZ *= 0.5;
    entity.velocityY -= 0.02;
    applyFluidWallClimb(entity, prevY);
}

void applyLadderConstraints(LivingEntity& entity) {
    if (!entity.isOnLadder()) {
        return;
    }
    constexpr float ladderCap = 0.15f;
    if (entity.velocityX < static_cast<double>(-ladderCap)) {
        entity.velocityX = -ladderCap;
    }
    if (entity.velocityX > static_cast<double>(ladderCap)) {
        entity.velocityX = ladderCap;
    }
    if (entity.velocityZ < static_cast<double>(-ladderCap)) {
        entity.velocityZ = -ladderCap;
    }
    if (entity.velocityZ > static_cast<double>(ladderCap)) {
        entity.velocityZ = ladderCap;
    }
    entity.fallDistance = 0.0f;
    if (entity.velocityY < -0.15) {
        entity.velocityY = -0.15;
    }
    if (entity.isSneaking() && entity.velocityY < 0.0) {
        entity.velocityY = 0.0;
    }
}

void travelGround(LivingEntity& entity, float sideways, float forward, float speedMultiplier) {
    float slipperiness = groundSlipperiness(entity);
    const float acceleration =
        entity.onGround ? 0.1f * (0.16277136f / (slipperiness * slipperiness * slipperiness)) : 0.02f;
    entity.moveNonSolid(sideways, forward, acceleration * speedMultiplier);
    slipperiness = groundSlipperiness(entity);
    applyLadderConstraints(entity);
    entity.move(entity.velocityX, entity.velocityY, entity.velocityZ);
    if (entity.horizontalCollision && entity.isOnLadder()) {
        entity.velocityY = 0.2;
    }
    entity.velocityY -= 0.08;
    entity.velocityY *= 0.98;
    entity.velocityX *= static_cast<double>(slipperiness);
    entity.velocityZ *= static_cast<double>(slipperiness);
}

void travelFlyingAir(LivingEntity& entity, float sideways, float forward, float speedMultiplier) {
    float slipperiness = groundSlipperiness(entity);
    const float acceleration = 0.16277136f / (slipperiness * slipperiness * slipperiness);
    entity.moveNonSolid(sideways, forward, (entity.onGround ? 0.1f * acceleration : 0.02f) * speedMultiplier);
    slipperiness = groundSlipperiness(entity);
    entity.move(entity.velocityX, entity.velocityY, entity.velocityZ);
    entity.velocityX *= static_cast<double>(slipperiness);
    entity.velocityY *= static_cast<double>(slipperiness);
    entity.velocityZ *= static_cast<double>(slipperiness);
}
}  // namespace

bool PlayerMovement::canStepUp(bool onGround, double intendedDy, double actualDy) {
    return onGround || (intendedDy != actualDy && intendedDy < 0.0);
}

void PlayerMovement::applyJumpInput(LivingEntity& entity) {
    if (!entity.jumping) {
        return;
    }
    if (entity.isSubmergedInWater() || entity.isTouchingLava()) {
        entity.velocityY += 0.04;
    } else if (entity.onGround) {
        entity.jump();
    }
}

void PlayerMovement::updateWalkAnimation(LivingEntity& entity) {
    entity.lastWalkAnimationSpeed = entity.walkAnimationSpeed;
    const double dx = entity.x - entity.prevX;
    const double dz = entity.z - entity.prevZ;
    float speed = MathHelper::sqrt(static_cast<float>(dx * dx + dz * dz)) * 4.0f;
    if (speed > 1.0f) {
        speed = 1.0f;
    }
    entity.walkAnimationSpeed += (speed - entity.walkAnimationSpeed) * 0.4f;
    entity.walkAnimationProgress += entity.walkAnimationSpeed;
}

void PlayerMovement::travel(LivingEntity& entity, float sideways, float forward, float speedMultiplier) {
    if (entity.world == nullptr) {
        return;
    }
    const float speedScale = speedMultiplier > 0.0f ? speedMultiplier : 1.0f;
    if (entity.isSubmergedInWater()) {
        travelWater(entity, sideways, forward, speedScale);
    } else if (entity.isTouchingLava()) {
        travelLava(entity, sideways, forward, speedScale);
    } else {
        travelGround(entity, sideways, forward, speedScale);
    }
    updateWalkAnimation(entity);
}

void PlayerMovement::travelFlying(LivingEntity& entity, float sideways, float forward, float speedMultiplier) {
    if (entity.world == nullptr) {
        return;
    }
    const float speedScale = speedMultiplier > 0.0f ? speedMultiplier : 1.0f;
    if (entity.isSubmergedInWater()) {
        travelWater(entity, sideways, forward, speedScale);
    } else if (entity.isTouchingLava()) {
        travelLava(entity, sideways, forward, speedScale);
    } else {
        travelFlyingAir(entity, sideways, forward, speedScale);
    }
    updateWalkAnimation(entity);
}
}  // namespace net::minecraft::entity::movement
