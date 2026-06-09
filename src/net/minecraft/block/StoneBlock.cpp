#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/StoneBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerStoneBlock()
{
    Block::STONE = (new StoneBlock(1, 1))->setHardness(1.5f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stone");
}

MINECRAFT_REGISTER_BLOCK(registerStoneBlock, 1);

} // namespace
} // namespace net::minecraft::block

