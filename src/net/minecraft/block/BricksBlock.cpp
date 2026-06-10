#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

struct BricksBlockRegistrar {
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager)
    {
    recipeManager.addShapedRecipe(ItemStack(Block::BRICKS),
        {std::string("##"), std::string("##"), '#', Item::byRawId(80)});
    }

    static void registerClass()
    {
        namespace mat = material;
        Block::BRICKS = (new Block(45, 7, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("brick");
    }
};

struct BricksBlockRecipeRegistrar {
    static void registerClass()
    {
        BricksBlockRegistrar::registerRecipes(recipe::CraftingRecipeManager::getInstance());
    }
};

static registry::RegisterCustom<BricksBlockRegistrar> s_reg(registry::kBlockRegistrarBase + 45);
static registry::RegisterCustom<BricksBlockRecipeRegistrar> s_recipeReg(registry::kCraftingRecipeRegistrarBase + 45);

} // namespace net::minecraft::block
