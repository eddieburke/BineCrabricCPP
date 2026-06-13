#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/mob/ZombieEntity.hpp"



#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::mob {

ZombieEntity::ZombieEntity(World* world) : MonsterEntity(world)
{
    texture = "/mob/zombie.png";
    movementSpeed = 0.5f;
    attackDamage = 5;
}

void ZombieEntity::tickMovement()
{
    if (world != nullptr && world->canMonsterSpawn()) {
        const float brightness = getBrightnessAtEyes(1.0f);
        if (brightness > 0.5f
            && world->hasSkyLight(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z))
            && random.nextFloat() * 30.0f < (brightness - 0.4f) * 2.0f) {
            fireTicks = 300;
        }
    }
    MonsterEntity::tickMovement();
}

int ZombieEntity::getDroppedItemId() const
{
    return Item::byRawId(32) != nullptr ? Item::byRawId(32)->id : 288;
}



MC_REGISTER_ENTITY(ZombieEntity)
} // namespace net::minecraft::entity::mob
