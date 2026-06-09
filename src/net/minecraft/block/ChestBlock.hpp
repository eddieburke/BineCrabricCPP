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

class ChestBlock : public BlockWithEntity {
public:
    using Block::canPlaceAt;
    explicit ChestBlock(int id) : BlockWithEntity(id, material::Material::WOOD) { textureId = 26; }

    [[nodiscard]] int getTextureId(const BlockView* blockView, int x, int y, int z, int side) const override;
    [[nodiscard]] int getTexture(int side) const override
    {
        return Block::textureForSide(side, textureId, textureId - 1, textureId - 1, FACE_WEST, textureId + 1);
    }

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z, int side) const override;
    bool onUse(World* world, int x, int y, int z, ::net::minecraft::PlayerEntity* player) override;
    void onBreak(World* world, int x, int y, int z) override;

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;

private:
    [[nodiscard]] bool hasNeighbor(World* world, int x, int y, int z) const;
    JavaRandom random_;
};

} // namespace net::minecraft::block
