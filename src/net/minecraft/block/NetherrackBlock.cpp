#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/NetherrackBlock.hpp"

namespace net::minecraft::block {
namespace {

void NetherrackBlock::registerClass()
{
    Block::NETHERRACK = (new NetherrackBlock(87, 103))->setHardness(0.4f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("hellrock");
}




static ::net::minecraft::registry::RegisterBlock<NetherrackBlock> autoReg(87);
} // namespace
} // namespace net::minecraft::block

