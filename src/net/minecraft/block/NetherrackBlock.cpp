#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/NetherrackBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerNetherrackBlock()
{
    Block::NETHERRACK = (new NetherrackBlock(87, 103))->setHardness(0.4f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("hellrock");
}

MINECRAFT_REGISTER_BLOCK(registerNetherrackBlock, 87);

} // namespace
} // namespace net::minecraft::block

