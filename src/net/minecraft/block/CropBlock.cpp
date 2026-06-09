#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CropBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerCropBlock()
{
    Block::WHEAT = (new CropBlock(59, 88))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("crops")->disableTrackingStatistics()->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerCropBlock, 59);

} // namespace
} // namespace net::minecraft::block

