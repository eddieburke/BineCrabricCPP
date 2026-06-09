#include "net/minecraft/entity/mob/GiantEntity.hpp"

namespace net::minecraft::entity::mob {

GiantEntity::GiantEntity(World* world) : MonsterEntity(world)
{
    texture = "/mob/zombie.png";
    movementSpeed = 0.5f;
    attackDamage = 50;
    health *= 10;
    standingEyeHeight *= 6.0f;
    setBoundingBoxSpacing(width * 6.0f, height * 6.0f);
}

float GiantEntity::getPathfindingFavor(int x, int y, int z) const
{
    if (world == nullptr) {
        return 0.0f;
    }
    return world->getLightBrightness(x, y, z) - 0.5f;
}

} // namespace net::minecraft::entity::mob
