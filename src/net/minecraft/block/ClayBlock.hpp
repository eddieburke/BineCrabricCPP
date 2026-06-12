#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/clay.hpp"

namespace net::minecraft::recipe { class CraftingRecipeManager; }
namespace net::minecraft::block {

class ClayBlock : public Block {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kBlockId = 82;
static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
public:
    static void registerClass();
    ClayBlock(int id, int textureId) : Block(id, textureId, material::Material::CLAY) {}

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override
    {
        return Item::byRawId(81) != nullptr ? Item::byRawId(81)->id : 337;
    }

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 4; }
};

} // namespace net::minecraft::block