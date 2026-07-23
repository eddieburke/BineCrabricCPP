#include "net/minecraft/item/SecondaryBlockItem.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
void SecondaryBlockItem::registerClass() {
 static SecondaryBlockItem CAKE(98, Block::CAKE);
 CAKE.setMaxCount(1)->setTexturePosition(13, 1)->setTranslationKey("cake");
 Item::registerInItemsArray(&CAKE);
 static SecondaryBlockItem REPEATER(100, Block::REPEATER);
 REPEATER.setTexturePosition(6, 5)->setTranslationKey("diode");
 Item::registerInItemsArray(&REPEATER);
}
void SecondaryBlockItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(ItemStack(Item::byRawId(98)),
                               {std::string("AAA"),
                                std::string("BEB"),
                                std::string("CCC"),
                                'A',
                                Item::byRawId(79),
                                'B',
                                Item::byRawId(97),
                                'C',
                                Item::byRawId(40),
                                'E',
                                Item::byRawId(88)});
 recipeManager.addShapedRecipe(ItemStack(Item::byRawId(100)),
                               {std::string("#X#"),
                                std::string("III"),
                                '#',
                                Block::LIT_REDSTONE_TORCH,
                                'X',
                                Item::byRawId(75),
                                'I',
                                Block::STONE});
}
MC_REGISTER_ITEM(SecondaryBlockItem)
} // namespace net::minecraft::item
