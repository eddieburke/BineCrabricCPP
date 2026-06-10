#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SoulSandBlock.hpp"

namespace net::minecraft::block {
void SoulSandBlock::registerClass()
{
    Block::SOUL_SAND = (new SoulSandBlock(88, 104))->setHardness(0.5f)->setSoundGroup(&vanillaSandSound())->setTranslationKey("hellsand");
}




namespace {static ::net::minecraft::registry::RegisterBlock<SoulSandBlock> autoReg(88);} // namespace
} // namespace net::minecraft::block

