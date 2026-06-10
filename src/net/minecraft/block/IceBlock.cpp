#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/IceBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/LightType.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

IceBlock::IceBlock(int blockId, int textureId) : TranslucentBlock(blockId, textureId, material::Material::ICE, false)
{
    slipperiness = 0.98f;
    setTickRandomly(true);
}

bool IceBlock::isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const
{
    return TranslucentBlock::isSideVisible(blockView, x, y, z, 1 - side);
}

void IceBlock::afterBreak(
    World* world,
    net::minecraft::PlayerEntity* player,
    int x,
    int y,
    int z,
    int meta)
{
    Block::afterBreak(world, player, x, y, z, meta);
    if (world == nullptr || Block::FLOWING_WATER == nullptr) {
        return;
    }
    const material::Material& below = world->getMaterial(x, y - 1, z);
    if (below.blocksMovement() || below.isFluid()) {
        world->setBlock(x, y, z, Block::FLOWING_WATER->id);
    }
}

void IceBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr || Block::WATER == nullptr) {
        return;
    }
    if (world->getBrightness(LightType::Block, x, y, z)
        > 11 - Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(id)]) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, Block::WATER->id);
    }
}
void IceBlock::registerClass()
{
    Block::ICE = (new IceBlock(79, 67))->setHardness(0.5f)->setOpacity(3)->setSoundGroup(&vanillaGlassSound())->setTranslationKey("ice");
}




namespace {static ::net::minecraft::registry::RegisterBlock<IceBlock> autoReg(79);} // namespace
} // namespace net::minecraft::block

