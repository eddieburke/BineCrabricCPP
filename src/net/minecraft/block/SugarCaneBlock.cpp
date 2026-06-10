#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SugarCaneBlock.hpp"

namespace net::minecraft::block {
void SugarCaneBlock::registerClass()
{
    Block::SUGAR_CANE = (new SugarCaneBlock(83, 73))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("reeds")->disableTrackingStatistics();
}




namespace {static ::net::minecraft::registry::RegisterBlock<SugarCaneBlock> autoReg(83);} // namespace
} // namespace net::minecraft::block

