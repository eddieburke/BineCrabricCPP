#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SandBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerSandBlock()
{
    Block::SAND = (new SandBlock(12, 18))->setHardness(0.5f)->setSoundGroup(&vanillaSandSound())->setTranslationKey("sand");
}

MINECRAFT_REGISTER_BLOCK(registerSandBlock, 12);

} // namespace
} // namespace net::minecraft::block

