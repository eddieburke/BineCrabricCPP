// Entity::move — faithful port of Entity.move (beta 1.7.3). Lives in its own TU to break cycles.

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/movement/PlayerMovement.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/LiquidBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/sound/BlockSoundGroup.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <algorithm>
#include <cmath>

namespace net::minecraft::entity {

bool Entity::isInFluid(block::material::Material& material) const
{
    if (world == nullptr) {
        return false;
    }
    const double eyeY = y + static_cast<double>(getEyeHeight());
    const int blockX = MathHelper::floor(x);
    const int blockY = MathHelper::floor(static_cast<double>(MathHelper::floor(eyeY)));
    const int blockZ = MathHelper::floor(z);
    const int blockId = world->getBlockId(blockX, blockY, blockZ);
    if (blockId == 0) {
        return false;
    }
    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if (block == nullptr || &block->material != &material) {
        return false;
    }
    const float fluidHeight =
        block::LiquidBlock::getFluidHeightFromMeta(world->getBlockMeta(blockX, blockY, blockZ)) - 0.11111111f;
    const float surfaceY = static_cast<float>(blockY + 1) - fluidHeight;
    return eyeY < static_cast<double>(surfaceY);
}

void Entity::move(double dx, double dy, double dz)
{
    if (world == nullptr) {
        return;
    }

    if (noClip) {
        boundingBox = boundingBox.translate(dx, dy, dz);
        x = (boundingBox.minX + boundingBox.maxX) * 0.5;
        y = boundingBox.minY + static_cast<double>(standingEyeHeight) - static_cast<double>(cameraOffset);
        z = (boundingBox.minZ + boundingBox.maxZ) * 0.5;
        return;
    }

    cameraOffset *= 0.4f;
    const double startX = x;
    const double startZ = z;

    if (slowed) {
        slowed = false;
        dx *= 0.25;
        dy *= 0.05;
        dz *= 0.25;
        velocityX = 0.0;
        velocityY = 0.0;
        velocityZ = 0.0;
    }

    double intendedDx = dx;
    const double intendedDy = dy;
    double intendedDz = dz;
    const Box preCollisionBox = boundingBox;

    const auto stopMovement = [](double& moveX, double& moveY, double& moveZ) {
        moveZ = 0.0;
        moveY = 0.0;
        moveX = 0.0;
    };

    const auto resolveY = [&](double& moveX, double& moveY, double& moveZ, const std::vector<Box>& collisions) {
        for (const Box& box : collisions) {
            moveY = box.getYOffset(boundingBox, moveY);
        }
        boundingBox = boundingBox.translate(0.0, moveY, 0.0);
        if (!keepVelocityOnCollision && intendedDy != moveY) {
            stopMovement(moveX, moveY, moveZ);
        }
    };

    const auto resolveX = [&](double& moveX, double& moveY, double& moveZ, const std::vector<Box>& collisions) {
        for (const Box& box : collisions) {
            moveX = box.getXOffset(boundingBox, moveX);
        }
        boundingBox = boundingBox.translate(moveX, 0.0, 0.0);
        if (!keepVelocityOnCollision && intendedDx != moveX) {
            stopMovement(moveX, moveY, moveZ);
        }
    };

    const auto resolveZ = [&](double& moveX, double& moveY, double& moveZ, const std::vector<Box>& collisions) {
        for (const Box& box : collisions) {
            moveZ = box.getZOffset(boundingBox, moveZ);
        }
        boundingBox = boundingBox.translate(0.0, 0.0, moveZ);
        if (!keepVelocityOnCollision && intendedDz != moveZ) {
            stopMovement(moveX, moveY, moveZ);
        }
    };

    const bool sneaking = onGround && isSneaking();
    if (sneaking) {
        constexpr double step = 0.05;
        const auto clampSneakMovement = [](double movement) {
            if (movement < step && movement >= -step) {
                return 0.0;
            }
            return movement + (movement > 0.0 ? -step : step);
        };
        while (dx != 0.0 && world->getEntityCollisions(this, boundingBox.offset(dx, -1.0, 0.0)).empty()) {
            dx = clampSneakMovement(dx);
            intendedDx = dx;
        }
        while (dz != 0.0 && world->getEntityCollisions(this, boundingBox.offset(0.0, -1.0, dz)).empty()) {
            dz = clampSneakMovement(dz);
            intendedDz = dz;
        }
    }

    std::vector<Box> collisions = world->getEntityCollisions(this, boundingBox.stretch(dx, dy, dz));

    resolveY(dx, dy, dz, collisions);
    const bool canStep = movement::PlayerMovement::canStepUp(onGround, intendedDy, dy);
    resolveX(dx, dy, dz, collisions);
    resolveZ(dx, dy, dz, collisions);

    if (stepHeight > 0.0f && canStep && (sneaking || cameraOffset < 0.05f) &&
        (intendedDx != dx || intendedDz != dz)) {
        const double savedDx = dx;
        const double savedDy = dy;
        const double savedDz = dz;
        dx = intendedDx;
        dy = static_cast<double>(stepHeight);
        dz = intendedDz;
        const Box savedBox = boundingBox;
        boundingBox = preCollisionBox;
        collisions = world->getEntityCollisions(this, boundingBox.stretch(dx, dy, dz));
        resolveY(dx, dy, dz, collisions);
        resolveX(dx, dy, dz, collisions);
        resolveZ(dx, dy, dz, collisions);
        if (!keepVelocityOnCollision && intendedDy != dy) {
            stopMovement(dx, dy, dz);
        } else {
            dy = -static_cast<double>(stepHeight);
            for (const Box& box : collisions) {
                dy = box.getYOffset(boundingBox, dy);
            }
            boundingBox = boundingBox.translate(0.0, dy, 0.0);
        }
        if (savedDx * savedDx + savedDz * savedDz >= dx * dx + dz * dz) {
            dx = savedDx;
            dy = savedDy;
            dz = savedDz;
            boundingBox = savedBox;
        } else {
            const double subY = boundingBox.minY - std::floor(boundingBox.minY);
            if (subY > 0.0) {
                cameraOffset = static_cast<float>(static_cast<double>(cameraOffset) + (subY + 0.01));
            }
        }
    }

    x = (boundingBox.minX + boundingBox.maxX) * 0.5;
    y = boundingBox.minY + static_cast<double>(standingEyeHeight) - static_cast<double>(cameraOffset);
    z = (boundingBox.minZ + boundingBox.maxZ) * 0.5;

    horizontalCollision = intendedDx != dx || intendedDz != dz;
    verticalCollision = intendedDy != dy;
    onGround = intendedDy != dy && intendedDy < 0.0;
    hasCollided = horizontalCollision || verticalCollision;

    fall(dy, onGround);

    if (intendedDx != dx) {
        velocityX = 0.0;
    }
    if (intendedDy != dy) {
        velocityY = 0.0;
    }
    if (intendedDz != dz) {
        velocityZ = 0.0;
    }

    const double movedX = x - startX;
    const double movedZ = z - startZ;
    if (bypassesSteppingEffects() && !sneaking && vehicle == nullptr) {
        horizontalSpeed = static_cast<float>(
            static_cast<double>(horizontalSpeed) + MathHelper::sqrt(static_cast<float>(movedX * movedX + movedZ * movedZ)) * 0.6f);
        const int blockX = MathHelper::floor(x);
        int blockY = MathHelper::floor(y - 0.2 - static_cast<double>(standingEyeHeight));
        const int blockZ = MathHelper::floor(z);
        int blockId = world->getBlockId(blockX, blockY, blockZ);
        if (Block::FENCE != nullptr && world->getBlockId(blockX, blockY - 1, blockZ) == Block::FENCE->id) {
            blockId = world->getBlockId(blockX, blockY - 1, blockZ);
        }
        if (horizontalSpeed > static_cast<float>(nextStepSoundDistance) && blockId > 0 && blockId < Block::BLOCK_COUNT) {
            Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
            if (block != nullptr && block->soundGroup != nullptr) {
                ++nextStepSoundDistance;
                BlockSoundGroup* soundGroup = block->soundGroup;
                if (Block::SNOW != nullptr && world->getBlockId(blockX, blockY + 1, blockZ) == Block::SNOW->id
                    && Block::SNOW->soundGroup != nullptr) {
                    soundGroup = Block::SNOW->soundGroup;
                    world->playSound(
                        this,
                        soundGroup->getSound(),
                        soundGroup->getVolume() * 0.15f,
                        soundGroup->getPitch());
                } else if (!block->material.isFluid()) {
                    world->playSound(
                        this,
                        soundGroup->getSound(),
                        soundGroup->getVolume() * 0.15f,
                        soundGroup->getPitch());
                }
                block->onSteppedOn(world, blockX, blockY, blockZ, this);
            }
        }
    }

    const int collisionMinX = MathHelper::floor(boundingBox.minX + 0.001);
    const int collisionMinY = MathHelper::floor(boundingBox.minY + 0.001);
    const int collisionMinZ = MathHelper::floor(boundingBox.minZ + 0.001);
    const int collisionMaxX = MathHelper::floor(boundingBox.maxX - 0.001);
    const int collisionMaxY = MathHelper::floor(boundingBox.maxY - 0.001);
    const int collisionMaxZ = MathHelper::floor(boundingBox.maxZ - 0.001);
    const int neighborMinX = collisionMinX - 1;
    const int neighborMinY = collisionMinY - 1;
    const int neighborMinZ = collisionMinZ - 1;
    const int neighborMaxX = collisionMaxX + 1;
    const int neighborMaxY = collisionMaxY + 1;
    const int neighborMaxZ = collisionMaxZ + 1;
    if (world->isRegionLoaded(
            neighborMinX, neighborMinY, neighborMinZ, neighborMaxX, neighborMaxY, neighborMaxZ)) {
        for (int blockX = neighborMinX; blockX <= neighborMaxX; ++blockX) {
            for (int blockY = neighborMinY; blockY <= neighborMaxY; ++blockY) {
                for (int blockZ = neighborMinZ; blockZ <= neighborMaxZ; ++blockZ) {
                    const int blockId = world->getBlockId(blockX, blockY, blockZ);
                    if (blockId <= 0 || blockId >= Block::BLOCK_COUNT) {
                        continue;
                    }
                    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
                    if (block == nullptr) {
                        continue;
                    }
                    const std::optional<Box> collisionShape = block->getCollisionShape(world, blockX, blockY, blockZ);
                    if (collisionShape.has_value()) {
                        if (!collisionShape->intersects(boundingBox)) {
                            continue;
                        }
                    } else if (blockX < collisionMinX || blockX > collisionMaxX || blockY < collisionMinY
                        || blockY > collisionMaxY || blockZ < collisionMinZ || blockZ > collisionMaxZ) {
                        continue;
                    }
                    block->onEntityCollision(world, blockX, blockY, blockZ, this);
                }
            }
        }
    }

    const bool wasWet = isWet();
    if (world->isFireOrLavaInBox(boundingBox.contract(0.001))) {
        damage(nullptr, 1);
        if (!wasWet) {
            ++fireTicks;
            if (fireTicks == 0) {
                fireTicks = 300;
            }
        }
    } else if (fireTicks <= 0) {
        fireTicks = -fireImmunityTicks;
    }
    if (wasWet && fireTicks > 0) {
        world->playSound(
            this,
            "random.fizz",
            0.7f,
            1.6f + (random.nextFloat() - random.nextFloat()) * 0.4f);
        fireTicks = -fireImmunityTicks;
    }
}

} // namespace net::minecraft::entity
