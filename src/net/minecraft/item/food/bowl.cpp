#include "net/minecraft/item/food/bowl.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/misc/stick.hpp"
namespace net::minecraft::item {
BowlItem::BowlItem() : Item(kRawId, RegistrationMode::Deferred) {}
void BowlItem::registerClass() {
  static BowlItem instance;
  instance.setTexturePosition(7, 4);
  instance.setTranslationKey("bowl");
  Item::registerInItemsArray(&instance);
}
void BowlItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(25), 4),
                                {std::string("# #"), std::string(" # "), '#', Block::PLANKS});
}
MC_REGISTER_ITEM(BowlItem)
} // namespace net::minecraft::item
