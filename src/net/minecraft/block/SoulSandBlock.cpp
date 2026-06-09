#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SoulSandBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerSoulSandBlock()
{
    Block::SOUL_SAND = (new SoulSandBlock(88, 104))->setHardness(0.5f)->setSoundGroup(&vanillaSandSound())->setTranslationKey("hellsand");
}

MINECRAFT_REGISTER_BLOCK(registerSoulSandBlock, 88);

} // namespace
} // namespace net::minecraft::block

