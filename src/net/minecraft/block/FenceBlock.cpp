#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FenceBlock.hpp"

namespace net::minecraft::block {
namespace {

void FenceBlock::registerClass()
{
    Block::FENCE = (new FenceBlock(85, 4))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("fence")->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<FenceBlock> autoReg(85);
} // namespace
} // namespace net::minecraft::block

