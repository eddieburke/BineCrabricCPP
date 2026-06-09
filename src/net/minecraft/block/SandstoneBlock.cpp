#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SandstoneBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerSandstoneBlock()
{
    Block::SANDSTONE = (new SandstoneBlock(24))->setSoundGroup(&vanillaStoneSound())->setHardness(0.8f)->setTranslationKey("sandStone");
}

MINECRAFT_REGISTER_BLOCK(registerSandstoneBlock, 24);

} // namespace
} // namespace net::minecraft::block

