#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::block {

class PumpkinBlock : public Block {
public:
    using Block::canPlaceAt;
    bool lit = false;

    PumpkinBlock(int id, int textureId, bool litIn);

    [[nodiscard]] int getTexture(int side, int meta) const override;
    [[nodiscard]] int getTexture(int side) const override;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    void onPlaced(
        World* world, int x, int y, int z, net::minecraft::entity::player::PlayerEntity* placer) override;
};

} // namespace net::minecraft::block
