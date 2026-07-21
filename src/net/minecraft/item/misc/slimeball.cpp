#include "net/minecraft/item/misc/slimeball.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
SlimeballItem::SlimeballItem() : Item(kRawId, RegistrationMode::Deferred) {
}
void SlimeballItem::registerClass() {
 static SlimeballItem instance;
 instance.setTexturePosition(14, 1);
 instance.setTranslationKey("slimeball");
 Item::registerInItemsArray(&instance);
}
void SlimeballItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 (void)recipeManager;
}
MC_REGISTER_ITEM(SlimeballItem)
} // namespace net::minecraft::item
