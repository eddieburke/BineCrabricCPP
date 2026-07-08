#include "net/minecraft/item/misc/flint.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
FlintItem::FlintItem() : Item(kRawId, RegistrationMode::Deferred) {
}

void FlintItem::registerClass() {
    static FlintItem instance;
    instance.setTexturePosition(6, 0);
    instance.setTranslationKey("flint");
    Item::registerInItemsArray(&instance);
}

void FlintItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    (void) recipeManager;
}
MC_REGISTER_ITEM(FlintItem)
}  // namespace net::minecraft::item
