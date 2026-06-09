#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/MushroomPlantBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerMushroomPlantBlocks()
{
    Block::BROWN_MUSHROOM = (new MushroomPlantBlock(39, 29))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setLuminance(0.125f)->setTranslationKey("mushroom");
    Block::RED_MUSHROOM = (new MushroomPlantBlock(40, 28))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("mushroom");
}

MINECRAFT_REGISTER_BLOCK(registerMushroomPlantBlocks, 39);

} // namespace
} // namespace net::minecraft::block

