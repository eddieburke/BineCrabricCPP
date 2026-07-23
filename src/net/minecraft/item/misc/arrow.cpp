#include "net/minecraft/item/misc/arrow.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::item {
ArrowItem::ArrowItem() : Item(kRawId, RegistrationMode::Deferred) {
}
void ArrowItem::registerClass() {
 static ArrowItem instance;
 instance.setTexturePosition(5, 2);
 instance.setTranslationKey("arrow");
 Item::registerInItemsArray(&instance);
}
void ArrowItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(ItemStack(Item::byRawId(6), 4),
                               {std::string("X"),
                                std::string("#"),
                                std::string("Y"),
                                'Y',
                                Item::byRawId(32),
                                'X',
                                Item::byRawId(62),
                                '#',
                                Item::byRawId(24)});
}
MC_REGISTER_ITEM(ArrowItem)
} // namespace net::minecraft::item
