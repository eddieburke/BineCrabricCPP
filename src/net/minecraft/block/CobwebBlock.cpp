#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CobwebBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerCobwebBlock()
{
    Block::COBWEB = (new CobwebBlock(30, 11))->setOpacity(1)->setHardness(4.0f)->setTranslationKey("web");
}

MINECRAFT_REGISTER_BLOCK(registerCobwebBlock, 30);

} // namespace
} // namespace net::minecraft::block

