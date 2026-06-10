#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/DirtBlock.hpp"


namespace net::minecraft::block {
void DirtBlock::registerClass()
{
    Block::DIRT = (new DirtBlock(3, 2))
        ->setHardness(0.5f)
        ->setSoundGroup(&vanillaGravelSound())
        ->setTranslationKey("dirt");
}




namespace {static ::net::minecraft::registry::RegisterBlock<DirtBlock> autoReg(3);} // namespace
} // namespace net::minecraft::block
