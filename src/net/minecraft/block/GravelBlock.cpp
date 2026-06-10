#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/GravelBlock.hpp"

namespace net::minecraft::block {
namespace {

void GravelBlock::registerClass()
{
    Block::GRAVEL = (new GravelBlock(13, 19))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("gravel");
}




static ::net::minecraft::registry::RegisterBlock<GravelBlock> autoReg(13);
} // namespace
} // namespace net::minecraft::block

