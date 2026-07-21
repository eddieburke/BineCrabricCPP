#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
StickItem::StickItem() : Item(kRawId, RegistrationMode::Deferred) {
}
void StickItem::registerClass() {
 static StickItem instance;
 instance.setTexturePosition(5, 3);
 instance.setHandheld();
 instance.setTranslationKey("stick");
 instance.setFuelTime(100);
 Item::registerInItemsArray(&instance);
}
void StickItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(ItemStack(Item::byRawId(24), 4),
                               {std::string("#"), std::string("#"), '#', Block::PLANKS});
}
MC_REGISTER_ITEM(StickItem)
} // namespace net::minecraft::item
