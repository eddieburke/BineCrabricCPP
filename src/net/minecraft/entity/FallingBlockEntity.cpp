#include "net/minecraft/entity/FallingBlockEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity {

FallingBlockEntity::FallingBlockEntity(World* world)
    : Entity(world)
{
}

FallingBlockEntity::FallingBlockEntity(World* world, double x, double y, double z, int blockIdIn)
    : Entity(world)
{
    blockId = blockIdIn;
    blocksSameBlockSpawning = true;
    setBoundingBoxSpacing(0.98f, 0.98f);
    standingEyeHeight = height / 2.0f;
    setPosition(x, y, z);
    velocityX = 0.0;
    velocityY = 0.0;
    velocityZ = 0.0;
    prevX = x;
    prevY = y;
    prevZ = z;
}

void FallingBlockEntity::tick()
{
    if (blockId == 0) {
        markDead();
        return;
    }

    prevX = x;
    prevY = y;
    prevZ = z;
    ++timeFalling;
    velocityY -= 0.04;
    move(velocityX, velocityY, velocityZ);
    velocityX *= 0.98;
    velocityY *= 0.98;
    velocityZ *= 0.98;

    const int blockX = MathHelper::floor(x);
    const int blockY = MathHelper::floor(y);
    const int blockZ = MathHelper::floor(z);
    if (world != nullptr && world->getBlockId(blockX, blockY, blockZ) == blockId) {
        world->setBlock(blockX, blockY, blockZ, 0);
    }

    if (onGround) {
        velocityX *= 0.7;
        velocityZ *= 0.7;
        velocityY *= -0.5;
        markDead();
        if (world == nullptr || world->isRemote()) {
            return;
        }
        const bool placed = world->canPlace(blockId, blockX, blockY, blockZ, true, 1)
            && !block::FallingBlock::canFallThrough(world, blockX, blockY - 1, blockZ)
            && world->setBlock(blockX, blockY, blockZ, blockId);
        if (!placed) {
            dropItem(blockId, 1);
        }
    } else if (timeFalling > 100 && world != nullptr && !world->isRemote()) {
        dropItem(blockId, 1);
        markDead();
    }
}

} // namespace net::minecraft::entity
