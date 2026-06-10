#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SnowBlock.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/world/LightType.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

SnowBlock::SnowBlock(int id, int textureId) : Block(id, textureId, material::Material::SNOW_BLOCK)
{
    setTickRandomly(true);
}

int SnowBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const
{
    return Item::SNOWBALL != nullptr ? Item::SNOWBALL->id : 332;
}

void SnowBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr) {
        return;
    }
    if (world->getBrightness(LightType::Block, x, y, z) > 11) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
}
namespace {

void SnowBlock::registerClass()
{
    Block::SNOW_BLOCK = (new SnowBlock(80, 66))->setHardness(0.2f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("snow");
}




static ::net::minecraft::registry::RegisterBlock<SnowBlock> autoReg(80);
} // namespace
} // namespace net::minecraft::block

