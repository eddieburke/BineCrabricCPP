#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/BookshelfBlock.hpp"

namespace net::minecraft::block {
namespace {

void BookshelfBlock::registerClass()
{
    Block::BOOKSHELF = (new BookshelfBlock(47, 35))->setHardness(1.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("bookshelf");
}




static ::net::minecraft::registry::RegisterBlock<BookshelfBlock> autoReg(47);
} // namespace
} // namespace net::minecraft::block

