#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SnowyBlock.hpp"

#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/SnowballItem.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/world/LightType.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

SnowyBlock::SnowyBlock(int id, int textureId) : Block(id, textureId, material::Material::SNOW_LAYER)
{
    setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
    setTickRandomly(true);
}

std::optional<net::minecraft::Box> SnowyBlock::getCollisionShape(
    World* world, int x, int y, int z) const
{
    const int layers = world != nullptr ? (world->getBlockMeta(x, y, z) & 7) : 0;
    if (layers < 3) {
        return std::nullopt;
    }
    return net::minecraft::Box {
        static_cast<double>(x) + minX,
        static_cast<double>(y) + minY,
        static_cast<double>(z) + minZ,
        static_cast<double>(x) + maxX,
        static_cast<double>(y) + 0.5,
        static_cast<double>(z) + maxZ,
    };
}

bool SnowyBlock::canPlaceAt(World* world, int x, int y, int z) const
{
    if (world == nullptr) {
        return false;
    }
    const int belowId = world->getBlockId(x, y - 1, z);
    if (belowId == 0 || belowId < 0 || belowId >= static_cast<int>(Block::BLOCKS.size())) {
        return false;
    }
    Block* below = Block::BLOCKS[static_cast<std::size_t>(belowId)];
    if (below == nullptr || !below->isOpaque()) {
        return false;
    }
    return world->getMaterial(x, y - 1, z).blocksMovement();
}

void SnowyBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/)
{
    breakIfCannotPlaceAt(world, x, y, z);
}

bool SnowyBlock::breakIfCannotPlaceAt(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return world != nullptr;
    }
    if (!canPlaceAt(world, x, y, z)) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
        return false;
    }
    return true;
}

void SnowyBlock::afterBreak(
    World* world,
    net::minecraft::PlayerEntity* player,
    int x,
    int y,
    int z,
    int meta)
{
    if (world == nullptr || world->isRemote() || player == nullptr) {
        Block::afterBreak(world, player, x, y, z, meta);
        return;
    }
    const int snowballId = Item::byRawId(76) != nullptr ? Item::byRawId(76)->id : 332;
    constexpr float spread = 0.7f;
    const double offsetX = static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
    const double offsetY = static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
    const double offsetZ = static_cast<double>(world->random().nextFloat() * spread) + static_cast<double>(1.0f - spread) * 0.5;
    auto* itemEntity = new net::minecraft::entity::ItemEntity(world, static_cast<double>(x) + offsetX, static_cast<double>(y) + offsetY,
        static_cast<double>(z) + offsetZ, ItemStack(snowballId, 1, 0));
    itemEntity->pickupDelay = 10;
    world->spawnEntity(itemEntity);
    world->setBlock(x, y, z, 0);
    player->increaseStat(stat::Stats::mineBlockStatId(id), 1);
}

int SnowyBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const
{
    return Item::byRawId(76) != nullptr ? Item::byRawId(76)->id : 332;
}

void SnowyBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr) {
        return;
    }
    if (world->getBrightness(LightType::Block, x, y, z) > 11) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
}

bool SnowyBlock::isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const
{
    if (side == 1) {
        return true;
    }
    return Block::isSideVisible(blockView, x, y, z, side);
}
void SnowyBlock::registerClass()
{
    Block::SNOW = (new SnowyBlock(78, 66))->setHardness(0.1f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("snow");
}




namespace {static ::net::minecraft::registry::RegisterBlock<SnowyBlock> autoReg(78);} // namespace
} // namespace net::minecraft::block

