#include "net/minecraft/item/misc/clay.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

ClayItem::ClayItem() : Item(81, RegistrationMode::Deferred) {}

void ClayItem::registerClass()
{
    static ClayItem instance;
    instance.setTexturePosition(9, 3);
    instance.setTranslationKey("clay");
    Item::registerInItemsArray(&instance);

}

void ClayItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<ClayItem> s_itemReg(81);
} // namespace net::minecraft::item
