#include "net/minecraft/item/misc/feather.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
namespace net::minecraft::item {
FeatherItem::FeatherItem() : Item(kRawId, RegistrationMode::Deferred) {}
void FeatherItem::registerClass() {
  static FeatherItem instance;
  instance.setTexturePosition(8, 1);
  instance.setTranslationKey("feather");
  Item::registerInItemsArray(&instance);
}
void FeatherItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  (void)recipeManager;
}
MC_REGISTER_ITEM(FeatherItem)
} // namespace net::minecraft::item
