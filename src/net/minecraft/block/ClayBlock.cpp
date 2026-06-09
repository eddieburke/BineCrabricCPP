#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/ClayBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerClayBlock()
{
    Block::CLAY = (new ClayBlock(82, 72))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("clay");
}

MINECRAFT_REGISTER_BLOCK(registerClayBlock, 82);

} // namespace
} // namespace net::minecraft::block

