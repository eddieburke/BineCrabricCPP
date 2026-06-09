#include "net/minecraft/entity/ItemEntity.hpp"

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity {

void ItemEntity::tick()
{
    Entity::tick();
    if (pickupDelay > 0) {
        --pickupDelay;
    }
    prevX = x;
    prevY = y;
    prevZ = z;
    velocityY -= 0.04;
    if (world != nullptr
        && &world->getMaterial(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z))
            == &block::material::Material::LAVA) {
        velocityY = 0.2;
        velocityX = static_cast<double>((random.nextFloat() - random.nextFloat()) * 0.2f);
        velocityZ = static_cast<double>((random.nextFloat() - random.nextFloat()) * 0.2f);
        world->playSound(this, "random.fizz", 0.4f, 2.0f + random.nextFloat() * 0.4f);
    }
    pushOutOfBlock(x, (boundingBox.minY + boundingBox.maxY) * 0.5, z);
    move(velocityX, velocityY, velocityZ);
    float drag = 0.98f;
    if (onGround) {
        drag = 0.58800006f;
        if (world != nullptr) {
            const int belowId = world->getBlockId(
                MathHelper::floor(x), MathHelper::floor(boundingBox.minY) - 1, MathHelper::floor(z));
            if (belowId > 0 && belowId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(belowId)] != nullptr) {
                drag = Block::BLOCKS[static_cast<std::size_t>(belowId)]->slipperiness * 0.98f;
            }
        }
    }
    velocityX *= static_cast<double>(drag);
    velocityY *= 0.98;
    velocityZ *= static_cast<double>(drag);
    if (onGround) {
        velocityY *= -0.5;
    }
    ++itemTicks;
    ++itemAge;
    if (itemAge >= 6000) {
        markDead();
    }
}

void ItemEntity::onPlayerInteraction(player::PlayerEntity* player)
{
    if (world == nullptr || world->isRemote() || player == nullptr || pickupDelay != 0 || stack.empty()) {
        return;
    }

    ItemStack pickupStack = stack;
    if (!player->inventory.addStack(pickupStack)) {
        return;
    }
    if (Block::LOG != nullptr && stack.itemId == Block::LOG->id) {
        player->increaseStat(achievement::Achievements::MINE_WOOD.statId(), 1);
    }
    if (Item::LEATHER != nullptr && stack.itemId == Item::LEATHER->id) {
        player->increaseStat(achievement::Achievements::KILL_COW.statId(), 1);
    }
    stack = std::move(pickupStack);
    world->playSound(
        this,
        "random.pop",
        0.2f,
        ((random.nextFloat() - random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
    world->notifyEntityPickup(this, player);
    if (stack.count <= 0) {
        markDead();
    }
}

bool ItemEntity::checkWaterCollisions()
{
    if (world == nullptr) {
        return false;
    }
    return world->updateMovementInFluid(boundingBox, block::material::Material::WATER, this);
}

bool ItemEntity::damage(Entity* /*damageSource*/, int amount)
{
    scheduleVelocityUpdate();
    health_ -= amount;
    if (health_ <= 0) {
        markDead();
    }
    return false;
}

} // namespace net::minecraft::entity
