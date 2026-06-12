#include "net/minecraft/item/food/raw_porkchop.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

RawPorkchopItem::RawPorkchopItem() : FoodItem(kRawId, 3, true) {}

void RawPorkchopItem::registerClass()
{
    static RawPorkchopItem instance;
    instance.setTexturePosition(7, 5);
    instance.setTranslationKey("porkchopRaw");
    Item::registerInItemsArray(&instance);

}

void RawPorkchopItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<RawPorkchopItem> s_itemReg;
} // namespace net::minecraft::item
