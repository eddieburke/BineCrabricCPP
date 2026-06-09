#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CakeBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerCakeBlock()
{
    Block::CAKE = (new CakeBlock(92, 121))->setHardness(0.5f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cake")->disableTrackingStatistics()->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerCakeBlock, 92);

} // namespace
} // namespace net::minecraft::block

