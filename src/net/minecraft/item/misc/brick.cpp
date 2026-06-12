#include "net/minecraft/item/misc/brick.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::item {

BrickItem::BrickItem() : Item(kRawId, RegistrationMode::Deferred) {}

void BrickItem::registerClass()
{
    static BrickItem instance;
    instance.setTexturePosition(6, 1);
    instance.setTranslationKey("brick");
    Item::registerInItemsArray(&instance);
}

void BrickItem::registerSmeltingRecipes()
{
    Item* clay = Item::byRawId(81);
    Item* brick = Item::byRawId(80);
    if (clay != nullptr && brick != nullptr) {
        recipe::SmeltingRecipeManager::instance().addRecipe(clay->id, ItemStack(brick));
    }
}

void BrickItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<BrickItem> s_itemReg;
} // namespace net::minecraft::item
