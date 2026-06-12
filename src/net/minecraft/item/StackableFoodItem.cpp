#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/StackableFoodItem.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

namespace net::minecraft::item {
void StackableFoodItem::registerClass()
{
    static StackableFoodItem COOKIE(101, 1, false, 8);
    COOKIE.setTexturePosition(12, 5)->setTranslationKey("cookie");
    Item::registerInItemsArray(&COOKIE);
}

void StackableFoodItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(101), 8),
        {std::string("#X#"), '#', Item::byRawId(40), 'X', ItemStack(Item::byRawId(95), 1, 3)});
}




namespace {static ::net::minecraft::registry::RegisterItem<StackableFoodItem> autoReg;} // namespace
} // namespace net::minecraft::item
