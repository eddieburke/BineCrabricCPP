#include "net/minecraft/item/misc/bone.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
BoneItem::BoneItem() : Item(kRawId, RegistrationMode::Deferred) {
}

void BoneItem::registerClass() {
    static BoneItem instance;
    instance.setTexturePosition(12, 1);
    instance.setHandheld();
    instance.setTranslationKey("bone");
    Item::registerInItemsArray(&instance);
}

void BoneItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    (void) recipeManager;
}
MC_REGISTER_ITEM(BoneItem)
}  // namespace net::minecraft::item
