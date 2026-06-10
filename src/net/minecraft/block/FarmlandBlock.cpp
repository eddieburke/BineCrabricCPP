#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FarmlandBlock.hpp"

namespace net::minecraft::block {
namespace {

void FarmlandBlock::registerClass()
{
    Block::FARMLAND = (new FarmlandBlock(60))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("farmland");
}




static ::net::minecraft::registry::RegisterBlock<FarmlandBlock> autoReg(60);
} // namespace
} // namespace net::minecraft::block

