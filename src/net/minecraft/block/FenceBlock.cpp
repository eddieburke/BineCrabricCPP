#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FenceBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerFenceBlock()
{
    Block::FENCE = (new FenceBlock(85, 4))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("fence")->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerFenceBlock, 85);

} // namespace
} // namespace net::minecraft::block

