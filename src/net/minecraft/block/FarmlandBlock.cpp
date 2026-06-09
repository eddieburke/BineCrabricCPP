#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FarmlandBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerFarmlandBlock()
{
    Block::FARMLAND = (new FarmlandBlock(60))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("farmland");
}

MINECRAFT_REGISTER_BLOCK(registerFarmlandBlock, 60);

} // namespace
} // namespace net::minecraft::block

