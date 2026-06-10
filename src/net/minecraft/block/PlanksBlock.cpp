#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

struct PlanksBlockRegistrar {
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager)
    {
    recipeManager.addShapedRecipe(ItemStack(Block::PLANKS, 4), {std::string("#"), '#', Block::LOG});
    }

    static void registerClass()
    {
        namespace mat = material;
        Block::PLANKS = (new Block(5, 4, mat::Material::WOOD))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("wood")->ignoreMetaUpdates();
    }
};

struct PlanksBlockRecipeRegistrar {
    static void registerClass()
    {
        PlanksBlockRegistrar::registerRecipes(recipe::CraftingRecipeManager::getInstance());
    }
};

static registry::RegisterCustom<PlanksBlockRegistrar> s_reg(registry::kBlockRegistrarBase + 5);
static registry::RegisterCustom<PlanksBlockRecipeRegistrar> s_recipeReg(registry::kCraftingRecipeRegistrarBase + 5);

} // namespace net::minecraft::block
