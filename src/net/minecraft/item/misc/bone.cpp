#include "net/minecraft/item/misc/bone.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/item/misc/stick.hpp"

namespace net::minecraft::item {

BoneItem::BoneItem() : Item(96, RegistrationMode::Deferred) {}

void BoneItem::registerClass()
{
    static BoneItem instance;
    instance.setTexturePosition(12, 1);
    instance.setHandheld();
    instance.setTranslationKey("bone");
    Item::registerInItemsArray(&instance);

}

void BoneItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    (void)recipeManager;
}

static registry::RegisterItem<BoneItem> s_itemReg(96);
} // namespace net::minecraft::item
