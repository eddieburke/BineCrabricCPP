#include "net/minecraft/item/transport/chest_minecart.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
ChestMinecartItem::ChestMinecartItem() : MinecartItem(kRawId, 1) {
}
void ChestMinecartItem::registerClass() {
  static ChestMinecartItem instance;
  instance.setTexturePosition(7, 9);
  instance.setTranslationKey("minecartChest");
  Item::registerInItemsArray(&instance);
}
void ChestMinecartItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(86)),
                                {std::string("A"), std::string("B"), 'A', Block::CHEST, 'B', Item::byRawId(72)});
}
MC_REGISTER_ITEM(ChestMinecartItem)
} // namespace net::minecraft::item
