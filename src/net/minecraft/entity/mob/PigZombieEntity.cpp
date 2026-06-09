#include "net/minecraft/entity/mob/PigZombieEntity.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::mob {

PigZombieEntity::PigZombieEntity(World* world) : ZombieEntity(world)
{
    texture = "/mob/pigzombie.png";
    movementSpeed = 0.5f;
    attackDamage = 5;
    fireImmune = true;
}

void PigZombieEntity::writeNbt(NbtCompound& nbt) const
{
    LivingEntity::writeNbt(nbt);
    nbt.putShort("Anger", static_cast<std::int16_t>(anger));
}

void PigZombieEntity::readNbt(const NbtCompound& nbt)
{
    LivingEntity::readNbt(nbt);
    anger = nbt.getShort("Anger");
}

void PigZombieEntity::tick()
{
    movementSpeed = target != nullptr ? 0.95f : 0.5f;
    if (angrySoundDelay > 0 && --angrySoundDelay == 0 && world != nullptr) {
        world->playSound(this, "mob.zombiepig.zpigangry", getSoundVolume() * 2.0f, ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 1.8f);
    }
    ZombieEntity::tick();
}

bool PigZombieEntity::canSpawn() const
{
    if (world == nullptr || world->difficulty == 0) {
        return false;
    }
    return world->canSpawnEntity(boundingBox)
        && world->getEntityCollisions(const_cast<PigZombieEntity*>(this), boundingBox).empty()
        && !world->isBoxSubmergedInFluid(boundingBox);
}

Entity* PigZombieEntity::getTargetInRange()
{
    if (anger == 0) {
        return nullptr;
    }
    return ZombieEntity::getTargetInRange();
}

bool PigZombieEntity::damage(Entity* damageSource, int amount)
{
    if (dynamic_cast<player::PlayerEntity*>(damageSource) != nullptr && world != nullptr) {
        const std::vector<Entity*> nearby = world->getEntities(this, boundingBox.expand(32.0, 32.0, 32.0));
        for (Entity* entity : nearby) {
            if (auto* pigZombie = dynamic_cast<PigZombieEntity*>(entity); pigZombie != nullptr) {
                pigZombie->makeAngry(damageSource);
            }
        }
        makeAngry(damageSource);
    }
    return ZombieEntity::damage(damageSource, amount);
}

void PigZombieEntity::makeAngry(Entity* source)
{
    target = source;
    anger = 400 + random.nextInt(400);
    angrySoundDelay = random.nextInt(40);
}

int PigZombieEntity::getDroppedItemId() const
{
    return Item::COOKED_PORKCHOP != nullptr ? Item::COOKED_PORKCHOP->id : 320;
}

} // namespace net::minecraft::entity::mob
