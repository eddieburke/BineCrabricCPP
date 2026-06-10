#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/WoolBlock.hpp"

namespace net::minecraft::block {
namespace {

void WoolBlock::registerClass()
{
    Block::WOOL = (new WoolBlock())->setHardness(0.8f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cloth")->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<WoolBlock> autoReg(35);
} // namespace
} // namespace net::minecraft::block

