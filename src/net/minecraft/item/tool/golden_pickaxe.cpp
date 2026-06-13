#include "net/minecraft/item/tool/golden_pickaxe.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

GoldenPickaxeItem::GoldenPickaxeItem() : PickaxeItem(kRawId, ToolMaterial::Gold) {}

void GoldenPickaxeItem::registerClass()
{
    static GoldenPickaxeItem instance;
    instance.setTexturePosition(4, 6);
    instance.setTranslationKey("pickaxeGold");
    Item::registerInItemsArray(&instance);

}

void GoldenPickaxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(29)),
        {std::string("XXX"), std::string(" # "), std::string(" # "), '#', Item::byRawId(24), 'X', Item::byRawId(10)});
}

MC_REGISTER_ITEM(GoldenPickaxeItem)
} // namespace net::minecraft::item
