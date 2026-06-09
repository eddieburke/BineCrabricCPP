#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/WoolBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerWoolBlock()
{
    Block::WOOL = (new WoolBlock())->setHardness(0.8f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cloth")->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerWoolBlock, 35);

} // namespace
} // namespace net::minecraft::block

