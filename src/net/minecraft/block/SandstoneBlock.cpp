#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SandstoneBlock.hpp"

namespace net::minecraft::block {
namespace {

void SandstoneBlock::registerClass()
{
    Block::SANDSTONE = (new SandstoneBlock(24))->setSoundGroup(&vanillaStoneSound())->setHardness(0.8f)->setTranslationKey("sandStone");
}




static ::net::minecraft::registry::RegisterBlock<SandstoneBlock> autoReg(24);
} // namespace
} // namespace net::minecraft::block

