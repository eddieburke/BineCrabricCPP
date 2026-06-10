#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/BowItem.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/misc/arrow.hpp"
#include "net/minecraft/item/misc/stick.hpp"
#include "net/minecraft/item/misc/string.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/world/World.hpp"

#include <memory>

namespace net::minecraft::item {

BowItem::BowItem() : Item(5, RegistrationMode::Deferred)
{
    setMaxCount(1);
}

ItemStack* BowItem::use(ItemStack* stack, World* world, PlayerEntity* user)
{
    if (world == nullptr || user == nullptr) {
        return stack;
    }
    if (user->inventory.remove(Item::byRawId(6) != nullptr ? Item::byRawId(6)->id : 262)) {
        world->playSound(user, "random.bow", 1.0f, 1.0f / (random.nextFloat() * 0.4f + 0.8f));
        if (!world->isRemote()) {
            auto projectile = std::make_unique<entity::projectile::ArrowEntity>(world, user);
            if (world->spawnEntity(projectile.get())) {
                projectile.release();
            }
        }
    }
    return stack;
}

void BowItem::registerClass()
{
    static BowItem instance;
    instance.setTexturePosition(5, 1)->setTranslationKey("bow");
    Item::registerInItemsArray(&instance);
}

void BowItem::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Item::byRawId(5)),
        {std::string(" #X"), std::string("# X"), std::string(" #X"), '#', Item::byRawId(24), 'X', Item::byRawId(31)});
}

namespace { static ::net::minecraft::registry::RegisterItem<BowItem> autoReg(5); }

} // namespace net::minecraft::item
