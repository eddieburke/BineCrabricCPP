#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CactusBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerCactusBlock()
{
    Block::CACTUS = (new CactusBlock(81, 70))->setHardness(0.4f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cactus");
}

MINECRAFT_REGISTER_BLOCK(registerCactusBlock, 81);

} // namespace
} // namespace net::minecraft::block

