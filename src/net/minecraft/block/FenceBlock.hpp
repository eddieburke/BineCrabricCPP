#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class FenceBlock : public Block {
public:
    static void registerClass();
    using Block::canPlaceAt;
    FenceBlock(int id, int textureId) : Block(id, textureId, material::Material::WOOD) {}

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const
    {
        if (world == nullptr) {
            return false;
        }
        if (world->getBlockId(x, y - 1, z) == id) {
            return true;
        }
        if (!world->getMaterial(x, y - 1, z).isSolid()) {
            return false;
        }
        return Block::canPlaceAt(world, x, y, z);
    }

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 11; }
    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int x, int y, int z) const override
    {
        return net::minecraft::Box {
            static_cast<double>(x),
            static_cast<double>(y),
            static_cast<double>(z),
            static_cast<double>(x + 1),
            static_cast<double>(y) + 1.5,
            static_cast<double>(z + 1),
        };
    }
};

} // namespace net::minecraft::block
