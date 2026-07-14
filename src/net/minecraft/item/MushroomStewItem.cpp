#include "net/minecraft/item/MushroomStewItem.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/food/bowl.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::item {
MushroomStewItem::MushroomStewItem(int rawId, int healthRestored) : FoodItem(rawId, healthRestored, false) {
  setMaxCount(1);
}
ItemStack* MushroomStewItem::use(ItemStack* stack, World* world, PlayerEntity* user) {
  FoodItem::use(stack, world, user);
  return new ItemStack(Item::byRawId(25));
}
void MushroomStewItem::registerClass() {
  static MushroomStewItem MUSHROOM_STEW(26, 10);
  MUSHROOM_STEW.setTexturePosition(8, 4)->setTranslationKey("mushroomStew");
  Item::registerInItemsArray(&MUSHROOM_STEW);
}
void MushroomStewItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(26)),
                                {std::string("Y"),
                                 std::string("X"),
                                 std::string("#"),
                                 'X',
                                 Block::BROWN_MUSHROOM,
                                 'Y',
                                 Block::RED_MUSHROOM,
                                 '#',
                                 Item::byRawId(25)});
  recipeManager.addShapedRecipe(ItemStack(Item::byRawId(26)),
                                {std::string("Y"),
                                 std::string("X"),
                                 std::string("#"),
                                 'X',
                                 Block::RED_MUSHROOM,
                                 'Y',
                                 Block::BROWN_MUSHROOM,
                                 '#',
                                 Item::byRawId(25)});
}
MC_REGISTER_ITEM(MushroomStewItem)
} // namespace net::minecraft::item
