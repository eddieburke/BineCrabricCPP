#include "net/minecraft/item/tool/golden_axe.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/item/misc/diamond.hpp"
#include "net/minecraft/item/misc/gold_ingot.hpp"

namespace net::minecraft::item {

GoldenAxeItem::GoldenAxeItem() : AxeItem(kRawId, ToolMaterial::Gold) {}

void GoldenAxeItem::registerClass()
{
    static GoldenAxeItem instance;
    instance.setTexturePosition(4, 7);
    instance.setTranslationKey("hatchetGold");
    Item::registerInItemsArray(&instance);

}

void GoldenAxeItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(30)),
        {std::string("XX"), std::string("X#"), std::string(" #"), '#', Item::byRawId(24), 'X', Item::byRawId(10)});
}

MC_REGISTER_ITEM(GoldenAxeItem)
} // namespace net::minecraft::item
