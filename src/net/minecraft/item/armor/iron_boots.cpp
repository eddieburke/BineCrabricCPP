#include "net/minecraft/item/armor/iron_boots.hpp"

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
IronBootsItem::IronBootsItem() : ArmorItem(kRawId, 2, 2, 3) {
}

void IronBootsItem::registerClass() {
    static IronBootsItem instance;
    instance.setTexturePosition(2, 3);
    instance.setTranslationKey("bootsIron");
    Item::registerInItemsArray(&instance);
}

void IronBootsItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(53)),
                                  {std::string("X X"), std::string("X X"), 'X', Item::byRawId(9)});
}
MC_REGISTER_ITEM(IronBootsItem)
}  // namespace net::minecraft::item
