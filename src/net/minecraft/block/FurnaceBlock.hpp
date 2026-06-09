#pragma once

#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft {
class World;
class JavaRandom;
}

namespace net::minecraft::block {

class FurnaceBlock : public BlockWithEntity {
public:
    FurnaceBlock(int id, bool lit) : BlockWithEntity(id, material::Material::STONE)
    {
        this->lit = lit;
        textureId = 45;
    }

    bool lit = false;
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return 61; }

    [[nodiscard]] int getTexture(int side) const override
    {
        if (side == 1 || side == 0) {
            return textureId + 17;
        }
        if (side == 3) {
            return textureId - 1;
        }
        return textureId;
    }

    [[nodiscard]] int getTextureId(const BlockView* blockView, int x, int y, int z, int side) const override
    {
        if (side == 1 || side == 0) {
            return textureId + 17;
        }
        int meta = blockView ? blockView->getBlockMeta(x, y, z) : 3;
        if (side != meta) {
            return textureId;
        }
        return lit ? textureId + 16 : textureId - 1;
    }

    void onPlaced(World* world, int x, int y, int z) override;
    void onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* placer) override;
    void onBreak(World* world, int x, int y, int z) override;
    bool onUse(World* world, int x, int y, int z, PlayerEntity* player) override;
    static void updateLitState(bool lit, World* world, int x, int y, int z);
    void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;

private:
    void updateDirection(World* world, int x, int y, int z);
    JavaRandom random_;
};

} // namespace net::minecraft::block
