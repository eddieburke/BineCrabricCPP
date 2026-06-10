#include "net/minecraft/item/food/apple.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

AppleItem::AppleItem() : FoodItem(4, 4, false) {}

void AppleItem::registerClass()
{
    static AppleItem instance;
    instance.setTexturePosition(10, 0);
    instance.setTranslationKey("apple");
    Item::registerInItemsArray(&instance);

}

void AppleItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<AppleItem> s_itemReg(4);
} // namespace net::minecraft::item
