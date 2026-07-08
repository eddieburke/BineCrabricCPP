#include "net/minecraft/item/food/golden_apple.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
GoldenAppleItem::GoldenAppleItem() : FoodItem(kRawId, 42, false) {
}

void GoldenAppleItem::registerClass() {
    static GoldenAppleItem instance;
    instance.setTexturePosition(11, 0);
    instance.setTranslationKey("appleGold");
    Item::registerInItemsArray(&instance);
}

void GoldenAppleItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(
        ItemStack(Item::byRawId(66)),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Block::GOLD_BLOCK, 'X', Item::byRawId(4)});
}
MC_REGISTER_ITEM(GoldenAppleItem)
}  // namespace net::minecraft::item
