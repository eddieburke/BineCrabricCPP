#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CakeBlock.hpp"

namespace net::minecraft::block {
namespace {

void CakeBlock::registerClass()
{
    Block::CAKE = (new CakeBlock(92, 121))->setHardness(0.5f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cake")->disableTrackingStatistics()->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<CakeBlock> autoReg(92);
} // namespace
} // namespace net::minecraft::block

