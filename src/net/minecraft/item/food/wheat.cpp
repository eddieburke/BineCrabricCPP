#include "net/minecraft/item/food/wheat.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

WheatItem::WheatItem() : Item(kRawId, RegistrationMode::Deferred) {}

void WheatItem::registerClass()
{
    static WheatItem instance;
    instance.setTexturePosition(9, 1);
    instance.setTranslationKey("wheat");
    Item::registerInItemsArray(&instance);

}

void WheatItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<WheatItem> s_itemReg;
} // namespace net::minecraft::item
