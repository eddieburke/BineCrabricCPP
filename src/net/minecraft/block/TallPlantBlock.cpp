#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/TallPlantBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerTallPlantBlock()
{
    Block::GRASS = (new TallPlantBlock(31, 39))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("tallgrass");
}

MINECRAFT_REGISTER_BLOCK(registerTallPlantBlock, 31);

} // namespace
} // namespace net::minecraft::block

