#include "net/minecraft/item/armor/diamond_boots.hpp"

#include <string>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/leather.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
DiamondBootsItem::DiamondBootsItem() : ArmorItem(kRawId, 3, 3, 3) {
}

void DiamondBootsItem::registerClass() {
    static DiamondBootsItem instance;
    instance.setTexturePosition(3, 3);
    instance.setTranslationKey("bootsDiamond");
    Item::registerInItemsArray(&instance);
}

void DiamondBootsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(57)),
                                  {std::string("X X"), std::string("X X"), 'X', Item::byRawId(8)});
}
MC_REGISTER_ITEM(DiamondBootsItem)
}  // namespace net::minecraft::item
