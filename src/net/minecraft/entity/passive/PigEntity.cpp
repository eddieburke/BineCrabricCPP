#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/EntityRegistrar.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/entity/mob/PigZombieEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::passive {

PigEntity::PigEntity(World* world) : AnimalEntity(world)
{
    initDataTracker();
    texture = "/mob/pig.png";
    setBoundingBoxSpacing(0.9f, 0.9f);
}

void PigEntity::writeNbt(NbtCompound& nbt) const
{
    LivingEntity::writeNbt(nbt);
    nbt.putBoolean("Saddle", isSaddled());
}

void PigEntity::readNbt(const NbtCompound& nbt)
{
    LivingEntity::readNbt(nbt);
    setSaddled(nbt.getBoolean("Saddle"));
}

bool PigEntity::interact(player::PlayerEntity* player)
{
    if (isSaddled() && world != nullptr && !world->isRemote() && (passenger == nullptr || passenger == player)) {
        player->setVehicle(this);
        return true;
    }
    return false;
}

void PigEntity::onStruckByLightning(Entity* /*lightning*/)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    auto* pigZombie = new mob::PigZombieEntity(world);
    pigZombie->setPositionAndAnglesKeepPrevAngles(x, y, z, yaw, pitch);
    world->spawnEntity(pigZombie);
    markDead();
}

void PigEntity::onLanding(float fallDistance)
{
    AnimalEntity::onLanding(fallDistance);
    if (fallDistance > 5.0f) {
        if (auto* player = dynamic_cast<player::PlayerEntity*>(passenger)) {
            player->incrementStat(achievement::Achievements::FLY_PIG.statId());
        }
    }
}

int PigEntity::getDroppedItemId() const
{
    if (fireTicks > 0) {
        return Item::COOKED_PORKCHOP != nullptr ? Item::COOKED_PORKCHOP->id : 320;
    }
    return Item::RAW_PORKCHOP != nullptr ? Item::RAW_PORKCHOP->id : 319;
}


void PigEntity::registerClass()
{
    ::net::minecraft::entity::detail::registerVanillaEntity<PigEntity>("Pig", 90);
}

static ::net::minecraft::registry::RegisterEntity<PigEntity> autoReg(90);

} // namespace net::minecraft::entity::passive
