#include "net/minecraft/item/misc/gunpowder.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
GunpowderItem::GunpowderItem() : Item(kRawId, RegistrationMode::Deferred) {
}
void GunpowderItem::registerClass() {
 static GunpowderItem instance;
 instance.setTexturePosition(8, 2);
 instance.setTranslationKey("sulphur");
 Item::registerInItemsArray(&instance);
}
void GunpowderItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 (void)recipeManager;
}
MC_REGISTER_ITEM(GunpowderItem)
} // namespace net::minecraft::item
