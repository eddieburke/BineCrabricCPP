#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
IronIngotItem::IronIngotItem() : Item(kRawId, RegistrationMode::Deferred) {
}
void IronIngotItem::registerClass() {
  static IronIngotItem instance;
  instance.setTexturePosition(7, 1);
  instance.setTranslationKey("ingotIron");
  Item::registerInItemsArray(&instance);
}
void IronIngotItem::registerSmeltingRecipes() {
  if(Block::IRON_ORE != nullptr) {
    recipe::SmeltingRecipeManager::instance().addRecipe(Block::IRON_ORE->id, ItemStack(Item::byRawId(9)));
  }
}
void IronIngotItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Block::IRON_BLOCK),
      {std::string("###"), std::string("###"), std::string("###"), '#', ItemStack(Item::byRawId(9), 9)});
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(9), 9), {std::string("#"), '#', Block::IRON_BLOCK});
}
MC_REGISTER_ITEM(IronIngotItem)
} // namespace net::minecraft::item
