#include "net/minecraft/entity/mob/SpiderEntity.hpp"

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::mob {
SpiderEntity::SpiderEntity(World* world) : MonsterEntity(world) {
    texture = "/mob/spider.png";
    setBoundingBoxSpacing(1.4f, 0.9f);
    movementSpeed = 0.8f;
}

Entity* SpiderEntity::getTargetInRange() {
    if (world == nullptr) {
        return nullptr;
    }
    const float brightness = getBrightnessAtEyes(1.0f);
    if (brightness < 0.5f) {
        return world->getClosestPlayer(this, 16.0);
    }
    return nullptr;
}

void SpiderEntity::attack(Entity* other, float distance) {
    const float brightness = getBrightnessAtEyes(1.0f);
    if (brightness > 0.5f && random.nextInt(100) == 0) {
        target = nullptr;
        return;
    }
    if (distance > 2.0f && distance < 6.0f && random.nextInt(10) == 0) {
        if (onGround && other != nullptr) {
            const double deltaX = other->x - x;
            const double deltaZ = other->z - z;
            const float flatDistance = MathHelper::sqrt(static_cast<float>(deltaX * deltaX + deltaZ * deltaZ));
            if (flatDistance > 1.0e-4f) {
                velocityX = deltaX / static_cast<double>(flatDistance) * 0.5 * 0.8 + velocityX * 0.2;
                velocityZ = deltaZ / static_cast<double>(flatDistance) * 0.5 * 0.8 + velocityZ * 0.2;
                velocityY = 0.4;
            }
        }
    } else {
        MonsterEntity::attack(other, distance);
    }
}

int SpiderEntity::getDroppedItemId() const {
    return Item::byRawId(31) != nullptr ? Item::byRawId(31)->id : 287;
}
MC_REGISTER_ENTITY(SpiderEntity)
}  // namespace net::minecraft::entity::mob
