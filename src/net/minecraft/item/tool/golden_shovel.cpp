#include "net/minecraft/item/tool/golden_shovel.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
GoldenShovelItem::GoldenShovelItem() : ShovelItem(kRawId, ToolMaterial::Gold) {
}

void GoldenShovelItem::registerClass() {
    static GoldenShovelItem instance;
    instance.setTexturePosition(4, 5);
    instance.setTranslationKey("shovelGold");
    Item::registerInItemsArray(&instance);
}

void GoldenShovelItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(
        ItemStack(Item::byRawId(28)),
        {std::string("X"), std::string("#"), std::string("#"), '#', Item::byRawId(24), 'X', Item::byRawId(10)});
}
MC_REGISTER_ITEM(GoldenShovelItem)
}  // namespace net::minecraft::item
