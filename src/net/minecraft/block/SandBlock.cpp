#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SandBlock.hpp"

namespace net::minecraft::block {
void SandBlock::registerClass()
{
    Block::SAND = (new SandBlock(12, 18))->setHardness(0.5f)->setSoundGroup(&vanillaSandSound())->setTranslationKey("sand");
}




namespace {static ::net::minecraft::registry::RegisterBlock<SandBlock> autoReg(12);} // namespace
} // namespace net::minecraft::block

