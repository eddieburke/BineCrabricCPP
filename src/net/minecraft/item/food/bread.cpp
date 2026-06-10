#include "net/minecraft/item/food/bread.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/item/food/wheat.hpp"

namespace net::minecraft::item {

BreadItem::BreadItem() : FoodItem(41, 5, false) {}

void BreadItem::registerClass()
{
    static BreadItem instance;
    instance.setTexturePosition(9, 2);
    instance.setTranslationKey("bread");
    Item::registerInItemsArray(&instance);

}

void BreadItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(41)),
        {std::string("###"), '#', Item::byRawId(40)});
}

static registry::RegisterItem<BreadItem> s_itemReg(41);
} // namespace net::minecraft::item
