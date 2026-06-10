#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/RedstoneOreBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/RedstoneItem.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

RedstoneOreBlock::RedstoneOreBlock(int id, int textureId, bool litIn) : Block(id, textureId, material::Material::STONE)
{
    lit = litIn;
    if (lit) {
        setTickRandomly(true);
    }
}

int RedstoneOreBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const
{
    return Item::byRawId(75) != nullptr ? Item::byRawId(75)->id : 331;
}

int RedstoneOreBlock::getDroppedItemCount(JavaRandom& random) const
{
    return 4 + random.nextInt(2);
}

void RedstoneOreBlock::light(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    spawnParticles(world, x, y, z);
    if (Block::REDSTONE_ORE != nullptr && id == Block::REDSTONE_ORE->id && Block::LIT_REDSTONE_ORE != nullptr) {
        world->setBlock(x, y, z, Block::LIT_REDSTONE_ORE->id);
    }
}

void RedstoneOreBlock::onBlockBreakStart(
    World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    light(world, x, y, z);
    Block::onBlockBreakStart(world, x, y, z, player);
}

void RedstoneOreBlock::onSteppedOn(World* world, int x, int y, int z, net::minecraft::Entity* entity)
{
    light(world, x, y, z);
    Block::onSteppedOn(world, x, y, z, entity);
}

bool RedstoneOreBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    light(world, x, y, z);
    return Block::onUse(world, x, y, z, player);
}

void RedstoneOreBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr || Block::LIT_REDSTONE_ORE == nullptr || Block::REDSTONE_ORE == nullptr) {
        return;
    }
    if (id == Block::LIT_REDSTONE_ORE->id) {
        world->setBlock(x, y, z, Block::REDSTONE_ORE->id);
    }
}

void RedstoneOreBlock::randomDisplayTick(
    World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (lit && world != nullptr) {
        spawnParticles(world, x, y, z);
    }
}

void RedstoneOreBlock::spawnParticles(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    JavaRandom& random = world->random();
    constexpr double inset = 0.0625;
    for (int i = 0; i < 6; ++i) {
        double px = static_cast<double>(x) + random.nextFloat();
        double py = static_cast<double>(y) + random.nextFloat();
        double pz = static_cast<double>(z) + random.nextFloat();
        if (i == 0 && !world->isBlockOpaqueCube(x, y + 1, z)) {
            py = static_cast<double>(y + 1) + inset;
        }
        if (i == 1 && !world->isBlockOpaqueCube(x, y - 1, z)) {
            py = static_cast<double>(y) - inset;
        }
        if (i == 2 && !world->isBlockOpaqueCube(x, y, z + 1)) {
            pz = static_cast<double>(z + 1) + inset;
        }
        if (i == 3 && !world->isBlockOpaqueCube(x, y, z - 1)) {
            pz = static_cast<double>(z) - inset;
        }
        if (i == 4 && !world->isBlockOpaqueCube(x + 1, y, z)) {
            px = static_cast<double>(x + 1) + inset;
        }
        if (i == 5 && !world->isBlockOpaqueCube(x - 1, y, z)) {
            px = static_cast<double>(x) - inset;
        }
        if (px < static_cast<double>(x) || px > static_cast<double>(x + 1) || py < 0.0
            || py > static_cast<double>(y + 1) || pz < static_cast<double>(z) || pz > static_cast<double>(z + 1)) {
            world->addParticle("reddust", px, py, pz, 0.0, 0.0, 0.0);
        }
    }
}
void RedstoneOreBlock::registerClass()
{
    Block::REDSTONE_ORE = (new RedstoneOreBlock(73, 51, false))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreRedstone")->ignoreMetaUpdates();
    Block::LIT_REDSTONE_ORE = (new RedstoneOreBlock(74, 51, true))->setLuminance(0.625f)->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreRedstone")->ignoreMetaUpdates();
}




namespace {static ::net::minecraft::registry::RegisterBlock<RedstoneOreBlock> autoReg(74);} // namespace
} // namespace net::minecraft::block

