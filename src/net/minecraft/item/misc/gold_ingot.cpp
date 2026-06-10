#include "net/minecraft/item/misc/gold_ingot.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"

namespace net::minecraft::item {

GoldIngotItem::GoldIngotItem() : Item(10, RegistrationMode::Deferred) {}

void GoldIngotItem::registerClass()
{
    static GoldIngotItem instance;
    instance.setTexturePosition(7, 2);
    instance.setTranslationKey("ingotGold");
    Item::registerInItemsArray(&instance);
}

void GoldIngotItem::registerSmeltingRecipes()
{
    if (Block::GOLD_ORE != nullptr) {
        recipe::SmeltingRecipeManager::instance().addRecipe(Block::GOLD_ORE->id, ItemStack(Item::byRawId(10)));
    }
}

void GoldIngotItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::GOLD_BLOCK),
        {std::string("###"), std::string("###"), std::string("###"), '#', ItemStack(Item::byRawId(10), 9)});
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(10), 9),
        {std::string("#"), '#', Block::GOLD_BLOCK});
}

static registry::RegisterItem<GoldIngotItem> s_itemReg(10);
} // namespace net::minecraft::item
