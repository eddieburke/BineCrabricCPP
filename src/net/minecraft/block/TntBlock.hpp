#pragma once
#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}

namespace net::minecraft::block {
class TntBlock : public Block {
   public:
    static constexpr int kBlockId = 46;
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

   public:
    static void registerClass();
    TntBlock(int id, int textureId);
    [[nodiscard]] int getTexture(int side) const override;

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override {
        return 0;
    }

    void onPlaced(World* world, int x, int y, int z) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void onMetadataChange(World* world, int x, int y, int z, int meta) override;
    void onDestroyedByExplosion(World* world, int x, int y, int z) override;
    void onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
};
}  // namespace net::minecraft::block
