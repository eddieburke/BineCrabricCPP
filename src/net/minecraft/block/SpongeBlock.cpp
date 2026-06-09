#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SpongeBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerSpongeBlock()
{
    Block::SPONGE = (new SpongeBlock(19))->setHardness(0.6f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("sponge");
}

MINECRAFT_REGISTER_BLOCK(registerSpongeBlock, 19);

} // namespace
} // namespace net::minecraft::block

