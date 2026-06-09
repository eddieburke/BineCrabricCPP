#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SugarCaneBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerSugarCaneBlock()
{
    Block::SUGAR_CANE = (new SugarCaneBlock(83, 73))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("reeds")->disableTrackingStatistics();
}

MINECRAFT_REGISTER_BLOCK(registerSugarCaneBlock, 83);

} // namespace
} // namespace net::minecraft::block

