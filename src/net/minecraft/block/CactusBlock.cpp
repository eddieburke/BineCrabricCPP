#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CactusBlock.hpp"

namespace net::minecraft::block {
void CactusBlock::registerClass()
{
    Block::CACTUS = (new CactusBlock(81, 70))->setHardness(0.4f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cactus");
}




namespace {static ::net::minecraft::registry::RegisterBlock<CactusBlock> autoReg(81);} // namespace
} // namespace net::minecraft::block

