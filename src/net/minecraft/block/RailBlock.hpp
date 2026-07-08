#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}

namespace net::minecraft::block {
class RailBlock : public Block {
   public:
    static constexpr int kBlockId = 27;
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

   public:
    static void registerClass();
    bool alwaysStraight = false;
    RailBlock(int id, int textureId, bool alwaysStraight);

    [[nodiscard]] bool isAlwaysStraight() const {
        return alwaysStraight;
    }

    [[nodiscard]] static bool isRail(World* world, int x, int y, int z);
    [[nodiscard]] static bool isRail(int blockId);

    [[nodiscard]] bool isOpaque() const override {
        return false;
    }

    [[nodiscard]] bool isFullCube() const override {
        return false;
    }

    [[nodiscard]] int getRenderType() const override {
        return 9;
    }

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override {
        return 1;
    }

    [[nodiscard]] int getPistonBehavior() const {
        return 0;
    }

    [[nodiscard]] int getTexture(int side, int meta) const override;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const override;

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/,
                                                                       int /*x*/,
                                                                       int /*y*/,
                                                                       int /*z*/) const override {
        return std::nullopt;
    }

    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;
    void onPlaced(World* world, int x, int y, int z) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    [[nodiscard]] std::optional<net::minecraft::HitResult> raycast(
        World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const override;

   private:
    void updateShape(World* world, int x, int y, int z, bool force);
    bool isPoweredByConnectedRails(World* world, int x, int y, int z, int meta, bool towardsNegative, int depth);
    bool isPoweredByRail(World* world, int x, int y, int z, bool towardsNegative, int depth, int shape);
};
}  // namespace net::minecraft::block
