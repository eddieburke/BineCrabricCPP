#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SpongeBlock.hpp"

namespace net::minecraft::block {
namespace {

void SpongeBlock::registerClass()
{
    Block::SPONGE = (new SpongeBlock(19))->setHardness(0.6f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("sponge");
}




static ::net::minecraft::registry::RegisterBlock<SpongeBlock> autoReg(19);
} // namespace
} // namespace net::minecraft::block

