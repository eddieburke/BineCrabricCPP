#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/BookshelfBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerBookshelfBlock()
{
    Block::BOOKSHELF = (new BookshelfBlock(47, 35))->setHardness(1.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("bookshelf");
}

MINECRAFT_REGISTER_BLOCK(registerBookshelfBlock, 47);

} // namespace
} // namespace net::minecraft::block

