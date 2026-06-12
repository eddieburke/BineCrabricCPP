#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/DetectorRailBlock.hpp"

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

DetectorRailBlock::DetectorRailBlock(int id, int textureId) : RailBlock(id, textureId, true)
{
    setTickRandomly(true);
}

bool DetectorRailBlock::canTransferPowerInDirection(
    World* world, int x, int y, int z, int direction) const
{
    return world != nullptr && (world->getBlockMeta(x, y, z) & 8) != 0 && direction == 1;
}

void DetectorRailBlock::updatePoweredStatus(World* world, int x, int y, int z, int meta)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const bool powered = (meta & 8) != 0;
    bool hasMinecart = false;
    constexpr double inset = 0.125;
    const Box box {
        static_cast<double>(x) + inset,
        static_cast<double>(y),
        static_cast<double>(z) + inset,
        static_cast<double>(x + 1) - inset,
        static_cast<double>(y) + 0.25,
        static_cast<double>(z + 1) - inset,
    };
    for (net::minecraft::Entity* entity : world->getEntities(nullptr, box)) {
        if (dynamic_cast<net::minecraft::entity::vehicle::MinecartEntity*>(entity) != nullptr) {
            hasMinecart = true;
            break;
        }
    }
    if (hasMinecart && !powered) {
        world->setBlockMeta(x, y, z, meta | 8);
        world->notifyNeighbors(x, y, z, id);
        world->notifyNeighbors(x, y - 1, z, id);
        world->setBlocksDirty(x, y, z, x, y, z);
    } else if (!hasMinecart && powered) {
        world->setBlockMeta(x, y, z, meta & 7);
        world->notifyNeighbors(x, y, z, id);
        world->notifyNeighbors(x, y - 1, z, id);
        world->setBlocksDirty(x, y, z, x, y, z);
    }
    if (hasMinecart) {
        world->scheduleBlockUpdate(x, y, z, id, getTickRate());
    }
}

void DetectorRailBlock::onEntityCollision(World* world, int x, int y, int z, net::minecraft::Entity* /*entity*/)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    if ((meta & 8) != 0) {
        return;
    }
    updatePoweredStatus(world, x, y, z, meta);
}

void DetectorRailBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    if ((meta & 8) == 0) {
        return;
    }
    updatePoweredStatus(world, x, y, z, meta);
}
void DetectorRailBlock::registerClass()
{
    Block::DETECTOR_RAIL = (new DetectorRailBlock(kBlockId, 195))->setHardness(0.7f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("detectorRail")->ignoreMetaUpdates();
}
void DetectorRailBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::DETECTOR_RAIL, 6),
        {std::string("X X"), std::string("X#X"), std::string("XRX"), 'X', Item::byRawId(9), 'R', Item::byRawId(75), '#', Block::STONE_PRESSURE_PLATE});
}





namespace {static ::net::minecraft::registry::RegisterBlock<DetectorRailBlock> autoReg;} // namespace
} // namespace net::minecraft::block

