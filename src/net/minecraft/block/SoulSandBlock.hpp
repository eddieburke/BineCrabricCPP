#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"

namespace net::minecraft::block {

class SoulSandBlock : public Block {
public:
    SoulSandBlock(int id, int textureId) : Block(id, textureId, material::Material::SAND) {}

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int x, int y, int z) const override
    {
        constexpr float inset = 0.125f;
        return net::minecraft::Box {
            static_cast<double>(x),
            static_cast<double>(y),
            static_cast<double>(z),
            static_cast<double>(x + 1),
            static_cast<double>(y + 1) - static_cast<double>(inset),
            static_cast<double>(z + 1),
        };
    }

    void onEntityCollision(World* /*world*/, int /*x*/, int /*y*/, int /*z*/, net::minecraft::Entity* entity) override
    {
        if (entity != nullptr) {
            entity->velocityX *= 0.4;
            entity->velocityZ *= 0.4;
        }
    }
};

} // namespace net::minecraft::block
