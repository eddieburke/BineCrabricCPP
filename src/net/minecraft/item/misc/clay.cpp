#include "net/minecraft/item/misc/clay.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
ClayItem::ClayItem() : Item(kRawId, RegistrationMode::Deferred) {
}
void ClayItem::registerClass() {
  static ClayItem instance;
  instance.setTexturePosition(9, 3);
  instance.setTranslationKey("clay");
  Item::registerInItemsArray(&instance);
}
void ClayItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  (void)recipeManager;
}
MC_REGISTER_ITEM(ClayItem)
} // namespace net::minecraft::item
