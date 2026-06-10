#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/ClayBlock.hpp"

namespace net::minecraft::block {
namespace {

void ClayBlock::registerClass()
{
    Block::CLAY = (new ClayBlock(82, 72))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("clay");
}




static ::net::minecraft::registry::RegisterBlock<ClayBlock> autoReg(82);
} // namespace
} // namespace net::minecraft::block

