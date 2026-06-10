#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/CobwebBlock.hpp"

namespace net::minecraft::block {
namespace {

void CobwebBlock::registerClass()
{
    Block::COBWEB = (new CobwebBlock(30, 11))->setOpacity(1)->setHardness(4.0f)->setTranslationKey("web");
}




static ::net::minecraft::registry::RegisterBlock<CobwebBlock> autoReg(30);
} // namespace
} // namespace net::minecraft::block

