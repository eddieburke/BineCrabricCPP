#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/DirtBlock.hpp"


namespace net::minecraft::block {
namespace {

void registerDirtBlock()
{
    Block::DIRT = (new DirtBlock(3, 2))
        ->setHardness(0.5f)
        ->setSoundGroup(&vanillaGravelSound())
        ->setTranslationKey("dirt");
}

MINECRAFT_REGISTER_BLOCK(registerDirtBlock, 3);

} // namespace
} // namespace net::minecraft::block
