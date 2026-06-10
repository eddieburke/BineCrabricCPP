#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/EntityRegistrar.hpp"
#include "net/minecraft/entity/passive/ChickenEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::passive {

ChickenEntity::ChickenEntity(World* world) : AnimalEntity(world)
{
    texture = "/mob/chicken.png";
    setBoundingBoxSpacing(0.3f, 0.4f);
    health = 4;
    eggLayTime = random.nextInt(6000) + 6000;
}

void ChickenEntity::tickMovement()
{
    AnimalEntity::tickMovement();
    prevFlapProgress = flapProgress;
    prevMaxWingDeviation = maxWingDeviation;
    maxWingDeviation = static_cast<float>(static_cast<double>(maxWingDeviation) + static_cast<double>(onGround ? -1 : 4) * 0.3);
    if (maxWingDeviation < 0.0f) {
        maxWingDeviation = 0.0f;
    }
    if (maxWingDeviation > 1.0f) {
        maxWingDeviation = 1.0f;
    }
    if (!onGround && flapSpeed < 1.0f) {
        flapSpeed = 1.0f;
    }
    flapSpeed = static_cast<float>(static_cast<double>(flapSpeed) * 0.9);
    if (!onGround && velocityY < 0.0) {
        velocityY *= 0.6;
    }
    flapProgress += flapSpeed * 2.0f;
    if (world != nullptr && !world->isRemote() && --eggLayTime <= 0) {
        world->playSound(this, "mob.chickenplop", 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
        if (Item::EGG != nullptr) {
            dropItem(Item::EGG->id, 1);
        }
        eggLayTime = random.nextInt(6000) + 6000;
    }
}

int ChickenEntity::getDroppedItemId() const
{
    return Item::FEATHER != nullptr ? Item::FEATHER->id : 288;
}


void ChickenEntity::registerClass()
{
    ::net::minecraft::entity::detail::registerVanillaEntity<ChickenEntity>("Chicken", 93);
}

static ::net::minecraft::registry::RegisterEntity<ChickenEntity> autoReg(93);

} // namespace net::minecraft::entity::passive
