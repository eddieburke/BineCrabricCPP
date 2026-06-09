#include "net/minecraft/entity/mob/CreeperEntity.hpp"

#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::mob {

void CreeperEntity::writeNbt(NbtCompound& nbt) const
{
    LivingEntity::writeNbt(nbt);
    if (dataTracker.getByte(17) == 1) {
        nbt.putBoolean("powered", true);
    }
}

void CreeperEntity::readNbt(const NbtCompound& nbt)
{
    LivingEntity::readNbt(nbt);
    dataTracker.set(17, static_cast<std::int8_t>(nbt.getBoolean("powered") ? 1 : 0));
}

void CreeperEntity::tick()
{
    lastFuseTime = fuseTime;
    if (world != nullptr && world->isRemote()) {
        const int fuseSpeed = getFuseSpeed();
        if (fuseSpeed > 0 && fuseTime == 0) {
            world->playSound(this, "random.fuse", 1.0f, 0.5f);
        }
        fuseTime += fuseSpeed;
        if (fuseTime < 0) {
            fuseTime = 0;
        }
        if (fuseTime >= 30) {
            fuseTime = 30;
        }
    }
    MonsterEntity::tick();
    if (target == nullptr && fuseTime > 0) {
        setFuseSpeed(-1);
        --fuseTime;
        if (fuseTime < 0) {
            fuseTime = 0;
        }
    }
}

void CreeperEntity::onKilledBy(Entity* adversary)
{
    MonsterEntity::onKilledBy(adversary);
    if (dynamic_cast<SkeletonEntity*>(adversary) != nullptr) {
        const int recordBase = Item::RECORD_THIRTEEN != nullptr ? Item::RECORD_THIRTEEN->id : 2256;
        dropItem(recordBase + random.nextInt(2), 1);
    }
}

void CreeperEntity::onStruckByLightning(Entity* lightning)
{
    MonsterEntity::onStruckByLightning(lightning);
    dataTracker.set(17, static_cast<std::int8_t>(1));
}

int CreeperEntity::getDroppedItemId() const
{
    return Item::GUNPOWDER != nullptr ? Item::GUNPOWDER->id : 289;
}

void CreeperEntity::resetAttack(Entity* /*other*/, float /*distance*/)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    if (fuseTime > 0) {
        setFuseSpeed(-1);
        --fuseTime;
        if (fuseTime < 0) {
            fuseTime = 0;
        }
    }
}

void CreeperEntity::attack(Entity* /*other*/, float distance)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const int fuseSpeed = getFuseSpeed();
    if ((fuseSpeed <= 0 && distance < 3.0f) || (fuseSpeed > 0 && distance < 7.0f)) {
        if (fuseTime == 0) {
            world->playSound(this, "random.fuse", 1.0f, 0.5f);
        }
        setFuseSpeed(1);
        ++fuseTime;
        if (fuseTime >= 30) {
            if (isCharged()) {
                world->createExplosion(this, x, y, z, 6.0f);
            } else {
                world->createExplosion(this, x, y, z, 3.0f);
            }
            markDead();
        }
        movementBlocked = true;
    } else {
        setFuseSpeed(-1);
        --fuseTime;
        if (fuseTime < 0) {
            fuseTime = 0;
        }
    }
}

} // namespace net::minecraft::entity::mob
