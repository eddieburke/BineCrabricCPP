#include "net/minecraft/item/misc/slimeball.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

SlimeballItem::SlimeballItem() : Item(85, RegistrationMode::Deferred) {}

void SlimeballItem::registerClass()
{
    static SlimeballItem instance;
    instance.setTexturePosition(14, 1);
    instance.setTranslationKey("slimeball");
    Item::registerInItemsArray(&instance);

}

void SlimeballItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<SlimeballItem> s_itemReg(85);
} // namespace net::minecraft::item
