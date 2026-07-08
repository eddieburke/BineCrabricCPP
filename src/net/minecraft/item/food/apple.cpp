#include "net/minecraft/item/food/apple.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
AppleItem::AppleItem() : FoodItem(kRawId, 4, false) {
}

void AppleItem::registerClass() {
    static AppleItem instance;
    instance.setTexturePosition(10, 0);
    instance.setTranslationKey("apple");
    Item::registerInItemsArray(&instance);
}

void AppleItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    (void) recipeManager;
}
MC_REGISTER_ITEM(AppleItem)
}  // namespace net::minecraft::item
