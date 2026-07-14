#include "net/minecraft/item/transport/furnace_minecart.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
FurnaceMinecartItem::FurnaceMinecartItem() : MinecartItem(kRawId, 2) {
}
void FurnaceMinecartItem::registerClass() {
  static FurnaceMinecartItem instance;
  instance.setTexturePosition(7, 10);
  instance.setTranslationKey("minecartFurnace");
  Item::registerInItemsArray(&instance);
}
void FurnaceMinecartItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(87)),
                                {std::string("A"), std::string("B"), 'A', Block::FURNACE, 'B', Item::byRawId(72)});
}
MC_REGISTER_ITEM(FurnaceMinecartItem)
} // namespace net::minecraft::item
