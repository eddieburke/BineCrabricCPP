#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/FlintAndSteel.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

FlintAndSteel::FlintAndSteel(int rawId) : Item(rawId)
{
    setMaxCount(1);
    setMaxDamage(64);
}

bool FlintAndSteel::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side)
{
    if (world == nullptr || stack == nullptr) {
        return false;
    }
    detail::offsetPlacementPos(world, x, y, z, side);
    if (world->getBlockId(x, y, z) == 0 && Block::FIRE != nullptr) {
        world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
            "fire.ignite", 1.0f, random.nextFloat() * 0.4f + 0.8f);
        world->setBlock(x, y, z, Block::FIRE->id);
    }
    stack->applyDamage(1);
    return true;
}

namespace {

void FlintAndSteel::registerClass()
{
    static FlintAndSteel FLINT_AND_STEEL(3);
    FLINT_AND_STEEL.setTexturePosition(5, 0)->setTranslationKey("flintAndSteel");
    Item::FLINT_AND_STEEL = &FLINT_AND_STEEL;
}




static ::net::minecraft::registry::RegisterItem<FlintAndSteel> autoReg(3);
} // namespace

} // namespace net::minecraft::item
