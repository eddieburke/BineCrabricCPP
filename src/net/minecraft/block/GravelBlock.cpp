#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/GravelBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerGravelBlock()
{
    Block::GRAVEL = (new GravelBlock(13, 19))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("gravel");
}

MINECRAFT_REGISTER_BLOCK(registerGravelBlock, 13);

} // namespace
} // namespace net::minecraft::block

