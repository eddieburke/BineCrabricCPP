#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SaplingBlock.hpp"
#include "net/minecraft/item/SaplingBlockItem.hpp"

namespace net::minecraft::block {
void SaplingBlock::registerClass()
{
    Block::SAPLING = (new SaplingBlock(6, 15))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("sapling")->ignoreMetaUpdates();
}

void SaplingBlock::registerBlockItems()
{
    (new item::SaplingBlockItem(6 - 256))->setTranslationKey("sapling");
}




namespace {static ::net::minecraft::registry::RegisterBlock<SaplingBlock> autoReg(6);} // namespace
} // namespace net::minecraft::block

