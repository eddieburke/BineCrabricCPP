#include "net/minecraft/item/food/raw_fish.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

RawFishItem::RawFishItem() : FoodItem(kRawId, 2, false) {}

void RawFishItem::registerClass()
{
    static RawFishItem instance;
    instance.setTexturePosition(9, 5);
    instance.setTranslationKey("fishRaw");
    Item::registerInItemsArray(&instance);

}

void RawFishItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<RawFishItem> s_itemReg;
} // namespace net::minecraft::item
