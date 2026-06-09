#include "net/minecraft/entity/mob/SlimeEntity.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

#include <cmath>

namespace net::minecraft::entity::mob {

SlimeEntity::SlimeEntity(World* world) : LivingEntity(world)
{
    initDataTracker();
    texture = "/mob/slime.png";
    standingEyeHeight = 0.0f;
    const int initialSize = 1 << random.nextInt(3);
    ticksUntilJump = random.nextInt(20) + 10;
    setSize(initialSize);
}

void SlimeEntity::setSize(int size)
{
    dataTracker.set(16, static_cast<std::int8_t>(size));
    setBoundingBoxSpacing(0.6f * static_cast<float>(size), 0.6f * static_cast<float>(size));
    health = size * size;
    setPosition(x, y, z);
}

int SlimeEntity::getSize() const
{
    return dataTracker.getByte(16);
}

void SlimeEntity::writeNbt(NbtCompound& nbt) const
{
    LivingEntity::writeNbt(nbt);
    nbt.putInt("Size", getSize() - 1);
}

void SlimeEntity::readNbt(const NbtCompound& nbt)
{
    LivingEntity::readNbt(nbt);
    setSize(nbt.getInt("Size") + 1);
}

void SlimeEntity::tick()
{
    lastStretch = stretch;
    const bool wasOnGround = onGround;
    LivingEntity::tick();
    if (onGround && !wasOnGround) {
        const int size = getSize();
        if (world != nullptr) {
            for (int i = 0; i < size * 8; ++i) {
                const float angle = random.nextFloat() * static_cast<float>(kPiF) * 2.0f;
                const float scale = random.nextFloat() * 0.5f + 0.5f;
                const float offsetX = MathHelper::sin(angle) * static_cast<float>(size) * 0.5f * scale;
                const float offsetZ = MathHelper::cos(angle) * static_cast<float>(size) * 0.5f * scale;
                world->addParticle("slime", x + static_cast<double>(offsetX), boundingBox.minY, z + static_cast<double>(offsetZ), 0.0, 0.0, 0.0);
            }
            if (size > 2) {
                world->playSound(this, "mob.slime", getSoundVolume(), ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) / 0.8f);
            }
        }
        stretch = -0.5f;
    }
    stretch *= 0.6f;
}

void SlimeEntity::tickLiving()
{
    tryDespawn();
    PlayerEntity* player = world != nullptr ? world->getClosestPlayer(this, 16.0) : nullptr;
    if (player != nullptr) {
        lookAt(player, 10.0f, 20.0f);
    }
    if (onGround && --ticksUntilJump <= 0) {
        ticksUntilJump = random.nextInt(20) + 10;
        if (player != nullptr) {
            ticksUntilJump /= 3;
        }
        jumping = true;
        if (getSize() > 1 && world != nullptr) {
            world->playSound(this, "mob.slime", getSoundVolume(), ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 0.8f);
        }
        stretch = 1.0f;
        sidewaysSpeed = 1.0f - random.nextFloat() * 2.0f;
        forwardSpeed = static_cast<float>(getSize());
    } else {
        jumping = false;
        if (onGround) {
            forwardSpeed = 0.0f;
            sidewaysSpeed = 0.0f;
        }
    }
}

void SlimeEntity::markDead()
{
    const int size = getSize();
    if (world != nullptr && !world->isRemote() && size > 1 && health == 0) {
        for (int i = 0; i < 4; ++i) {
            const float offsetX = (static_cast<float>(i % 2) - 0.5f) * static_cast<float>(size) / 4.0f;
            const float offsetZ = (static_cast<float>(i / 2) - 0.5f) * static_cast<float>(size) / 4.0f;
            auto* child = new SlimeEntity(world);
            child->setSize(size / 2);
            child->setPositionAndAnglesKeepPrevAngles(x + static_cast<double>(offsetX), y + 0.5, z + static_cast<double>(offsetZ), random.nextFloat() * 360.0f, 0.0f);
            world->spawnEntity(child);
        }
    }
    LivingEntity::markDead();
}

void SlimeEntity::onPlayerInteraction(player::PlayerEntity* player)
{
    const int size = getSize();
    if (size > 1 && player != nullptr && canSee(player) && getDistance(*player) < 0.6f * static_cast<float>(size)) {
        if (player->damage(this, size) && world != nullptr) {
            world->playSound(this, "mob.slimeattack", 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
        }
    }
}

bool SlimeEntity::canSpawn() const
{
    if (world == nullptr) {
        return false;
    }
    const Chunk& chunk = world->getChunkFromPos(MathHelper::floor(x), MathHelper::floor(z));
    return (getSize() == 1 || world->difficulty > 0)
        && random.nextInt(10) == 0
        && chunk.getSlimeRandom(987234911LL).nextInt(10) == 0
        && y < 16.0;
}

int SlimeEntity::getDroppedItemId() const
{
    if (getSize() == 1) {
        return Item::SLIMEBALL != nullptr ? Item::SLIMEBALL->id : 341;
    }
    return 0;
}

} // namespace net::minecraft::entity::mob
