#pragma once

// Entity-capable world port — extends IBlockWorld for block headers that spawn
// entities (e.g. CropBlock dropStacks). Most block headers use IBlockWorld only.

#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::entity {
class Entity;
}

namespace net::minecraft {

using Entity = entity::Entity;

class IEntityWorld : public IBlockWorld {
public:
    ~IEntityWorld() override = default;

    virtual bool spawnEntity(Entity* entity) = 0;
};

} // namespace net::minecraft
