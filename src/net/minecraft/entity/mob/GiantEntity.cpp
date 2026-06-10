#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/EntityRegistrar.hpp"
#include "net/minecraft/entity/mob/GiantEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

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


void GiantEntity::registerClass()
{
    ::net::minecraft::entity::detail::registerVanillaEntity<GiantEntity>("Giant", 53);
}

static ::net::minecraft::registry::RegisterEntity<GiantEntity> autoReg(53);

} // namespace net::minecraft::entity::mob
