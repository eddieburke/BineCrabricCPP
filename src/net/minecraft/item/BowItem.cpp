#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/BowItem.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

#include <memory>

namespace net::minecraft::item {

BowItem::BowItem(int rawId) : Item(rawId)
{
    setMaxCount(1);
}

ItemStack* BowItem::use(ItemStack* stack, World* world, PlayerEntity* user)
{
    if (world == nullptr || user == nullptr) {
        return stack;
    }
    if (user->inventory.remove(Item::ARROW != nullptr ? Item::ARROW->id : 262)) {
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

namespace {

void BowItem::registerClass()
{
    static BowItem BOW(5);
    BOW.setTexturePosition(5, 1)->setTranslationKey("bow");
    Item::BOW = &BOW;
}




static ::net::minecraft::registry::RegisterItem<BowItem> autoReg(5);
} // namespace

} // namespace net::minecraft::item
