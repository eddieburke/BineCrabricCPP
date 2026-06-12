#pragma once

#include "net/minecraft/block/Block.hpp"

#include <array>
#include <string_view>

namespace net::minecraft::recipe { class CraftingRecipeManager; }
namespace net::minecraft::block {

class SlabBlock : public Block {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kBlockId = 43;
static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
public:
    static void registerClass();
    static void registerBlockItems();
    static constexpr std::array<std::string_view, 4> names {"stone", "sand", "wood", "cobble"};

    bool doubleSlab = false;

    SlabBlock(int id, bool doubleSlab);

    [[nodiscard]] int getTexture(int side, int meta) const override;
    [[nodiscard]] bool isOpaque() const override { return doubleSlab; }
    [[nodiscard]] bool isFullCube() const override { return doubleSlab; }
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override
    {
        return Block::SLAB != nullptr ? Block::SLAB->id : 44;
    }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& random) const override;
    [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override;

    void onPlaced(World* world, int x, int y, int z) override;

protected:
    [[nodiscard]] int getDroppedItemMeta(int blockMeta) const override;
};

} // namespace net::minecraft::block
