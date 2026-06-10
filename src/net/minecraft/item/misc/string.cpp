#include "net/minecraft/item/misc/string.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

StringItem::StringItem() : Item(31, RegistrationMode::Deferred) {}

void StringItem::registerClass()
{
    static StringItem instance;
    instance.setTexturePosition(8, 0);
    instance.setTranslationKey("string");
    Item::registerInItemsArray(&instance);

}

void StringItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<StringItem> s_itemReg(31);
} // namespace net::minecraft::item
