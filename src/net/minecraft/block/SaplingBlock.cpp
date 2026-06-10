#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SaplingBlock.hpp"

namespace net::minecraft::block {
namespace {

void SaplingBlock::registerClass()
{
    Block::SAPLING = (new SaplingBlock(6, 15))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("sapling")->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<SaplingBlock> autoReg(6);
} // namespace
} // namespace net::minecraft::block

