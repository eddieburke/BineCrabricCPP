#include "net/minecraft/item/tool/wooden_shovel.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
WoodenShovelItem::WoodenShovelItem() : ShovelItem(kRawId, ToolMaterial::Wood) {
}

void WoodenShovelItem::registerClass() {
    static WoodenShovelItem instance;
    instance.setTexturePosition(0, 5);
    instance.setTranslationKey("shovelWood");
    Item::registerInItemsArray(&instance);
}

void WoodenShovelItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(
        ItemStack(Item::byRawId(13)),
        {std::string("X"), std::string("#"), std::string("#"), '#', Item::byRawId(24), 'X', Block::PLANKS});
}
MC_REGISTER_ITEM(WoodenShovelItem)
}  // namespace net::minecraft::item
