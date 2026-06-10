#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonConstants.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"

namespace net::minecraft {
class World;
}

namespace net::minecraft::recipe { class CraftingRecipeManager; }
namespace net::minecraft::block {

class PistonBlock : public Block {
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
public:
    static void registerClass();
    static void registerBlockItems();
    bool sticky = false;

    PistonBlock(int id, int textureId, bool stickyIn);

    [[nodiscard]] int getRenderType() const override { return 16; }
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getTopTexture() const { return sticky ? PistonConstants::TEXTURE_STICKY_TOP : PistonConstants::TEXTURE_TOP; }
    [[nodiscard]] int getTexture(int side, int meta) const override;

    void onPlaced(World* world, int x, int y, int z) override;
    void onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* placer) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void onBlockAction(World* world, int x, int y, int z, int data1, int data2) override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;
    void addIntersectingBoundingBox(
        World* world, int x, int y, int z, const net::minecraft::Box& box, std::vector<Box>& boxes) const override;

    static int getFacing(int meta) { return meta & 7; }
    static bool isExtended(int meta) { return (meta & 8) != 0; }

private:
    bool deaf_ = false;
    void checkExtended(World* world, int x, int y, int z);
    [[nodiscard]] bool shouldExtend(World* world, int x, int y, int z, int facing) const;
    [[nodiscard]] bool push(World* world, int x, int y, int z, int dir);
    static int getFacingForPlacement(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player);
    static bool canMoveBlock(int blockId, World* world, int x, int y, int z, bool allowBreaking);
    static bool canExtend(World* world, int x, int y, int z, int dir);
};

} // namespace net::minecraft::block
