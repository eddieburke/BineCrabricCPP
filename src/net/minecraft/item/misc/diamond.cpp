#include "net/minecraft/item/misc/diamond.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"

namespace net::minecraft::item {

DiamondItem::DiamondItem() : Item(8, RegistrationMode::Deferred) {}

void DiamondItem::registerClass()
{
    static DiamondItem instance;
    instance.setTexturePosition(7, 3);
    instance.setTranslationKey("emerald");
    Item::registerInItemsArray(&instance);
}

void DiamondItem::registerSmeltingRecipes()
{
    if (Block::DIAMOND_ORE != nullptr) {
        recipe::SmeltingRecipeManager::instance().addRecipe(Block::DIAMOND_ORE->id, ItemStack(Item::byRawId(8)));
    }
}

void DiamondItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::DIAMOND_BLOCK),
        {std::string("###"), std::string("###"), std::string("###"), '#', ItemStack(Item::byRawId(8), 9)});
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(8), 9),
        {std::string("#"), '#', Block::DIAMOND_BLOCK});
}

static registry::RegisterItem<DiamondItem> s_itemReg(8);
} // namespace net::minecraft::item
