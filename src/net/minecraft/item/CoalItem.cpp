#include "net/minecraft/item/CoalItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::item {
void CoalItem::registerClass() {
    static CoalItem COAL(7);
    COAL.setTexturePosition(7, 0)->setTranslationKey("coal")->setFuelTime(1600);
}

void CoalItem::registerSmeltingRecipes() {
    if (Block::LOG != nullptr) {
        recipe::SmeltingRecipeManager::instance().addRecipe(Block::LOG->id, ItemStack(Item::byRawId(7), 1, 1));
    }
}
MC_REGISTER_ITEM(CoalItem)
}  // namespace net::minecraft::item
