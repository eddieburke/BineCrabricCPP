#include "net/minecraft/item/misc/string.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
namespace net::minecraft::item {
StringItem::StringItem() : Item(kRawId, RegistrationMode::Deferred) {}
void StringItem::registerClass() {
  static StringItem instance;
  instance.setTexturePosition(8, 0);
  instance.setTranslationKey("string");
  Item::registerInItemsArray(&instance);
}
void StringItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  (void)recipeManager;
}
MC_REGISTER_ITEM(StringItem)
} // namespace net::minecraft::item
