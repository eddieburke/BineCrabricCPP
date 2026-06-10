#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CropBlock.hpp"

namespace net::minecraft::block {
namespace {

void CropBlock::registerClass()
{
    Block::WHEAT = (new CropBlock(59, 88))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("crops")->disableTrackingStatistics()->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<CropBlock> autoReg(59);
} // namespace
} // namespace net::minecraft::block

