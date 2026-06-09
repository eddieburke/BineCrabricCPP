#include "net/minecraft/block/BlockWithEntity.hpp"

#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

void BlockWithEntity::onPlaced(World* world, int x, int y, int z)
{
    Block::onPlaced(world, x, y, z);
    if (world == nullptr) {
        return;
    }
    auto entity = createBlockEntity();
    if (entity == nullptr) {
        return;
    }
    entity->x = x;
    entity->y = y;
    entity->z = z;
    entity->world = world;
    world->setBlockEntity(x, y, z, std::move(entity));
}

void BlockWithEntity::onBreak(World* world, int x, int y, int z)
{
    Block::onBreak(world, x, y, z);
    if (world != nullptr) {
        world->removeBlockEntity(x, y, z);
    }
}

} // namespace net::minecraft::block
