#pragma once
#include "net/minecraft/block/Block.hpp"

namespace net::minecraft {
class BlockView;
class World;
}  // namespace net::minecraft

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}

namespace net::minecraft::block {
class TorchBlock : public Block {
   public:
    static constexpr int kBlockId = 50;
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

   public:
    static void registerClass();
    using Block::canPlaceAt;
    TorchBlock(int id, int textureId);

    [[nodiscard]] bool isOpaque() const override {
        return false;
    }

    [[nodiscard]] bool isFullCube() const override {
        return false;
    }

    [[nodiscard]] int getRenderType() const override {
        return 2;
    }

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/,
                                                                       int /*x*/,
                                                                       int /*y*/,
                                                                       int /*z*/) const override {
        return std::nullopt;
    }

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    void onPlaced(World* world, int x, int y, int z) override;
    void onPlaced(World* world, int x, int y, int z, int direction) override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    [[nodiscard]] std::optional<net::minecraft::HitResult> raycast(
        World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const;
    void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;

   private:
    [[nodiscard]] bool canPlaceOn(World* world, int x, int y, int z) const;
    bool breakIfCannotPlaceAt(World* world, int x, int y, int z);
};
}  // namespace net::minecraft::block
