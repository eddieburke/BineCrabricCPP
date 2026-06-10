#pragma once

#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft {
class World;
class JavaRandom;
}

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::entity {
class LivingEntity;
}

namespace net::minecraft::recipe { class CraftingRecipeManager; }
namespace net::minecraft::block {

class DispenserBlock : public BlockWithEntity {
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
public:
    static void registerClass();
    explicit DispenserBlock(int id) : BlockWithEntity(id, material::Material::STONE) { textureId = 45; }

    [[nodiscard]] int getTickRate() const override { return 4; }
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return 23; }

    [[nodiscard]] int getTexture(int side) const override
    {
        return Block::textureForSide(side, textureId, textureId + 17, textureId + 17, FACE_WEST, textureId + 1);
    }

    [[nodiscard]] int getTextureId(const BlockView* blockView, int x, int y, int z, int side) const override
    {
        const int facing = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : FACE_WEST;
        return Block::textureForSide(side, textureId, textureId + 17, textureId + 17, facing, textureId + 1);
    }

    void onPlaced(World* world, int x, int y, int z) override;
    void onPlaced(World* world, int x, int y, int z, PlayerEntity* placer) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    bool onUse(World* world, int x, int y, int z, PlayerEntity* player) override;
    void onBreak(World* world, int x, int y, int z) override;

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;

private:
    void updateDirection(World* world, int x, int y, int z);
    void dispense(World* world, int x, int y, int z, JavaRandom& random);
    JavaRandom random_;
};

} // namespace net::minecraft::block
