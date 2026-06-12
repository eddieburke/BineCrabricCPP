#include "net/minecraft/item/misc/leather.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

LeatherItem::LeatherItem() : Item(kRawId, RegistrationMode::Deferred) {}

void LeatherItem::registerClass()
{
    static LeatherItem instance;
    instance.setTexturePosition(7, 6);
    instance.setTranslationKey("leather");
    Item::registerInItemsArray(&instance);

}

void LeatherItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<LeatherItem> s_itemReg;
} // namespace net::minecraft::item
