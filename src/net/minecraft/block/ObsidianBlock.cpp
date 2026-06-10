#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/ObsidianBlock.hpp"

namespace net::minecraft::block {
void ObsidianBlock::registerClass()
{
    Block::OBSIDIAN = (new ObsidianBlock(49, 37))->setHardness(10.0f)->setResistance(2000.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("obsidian");
}




namespace {static ::net::minecraft::registry::RegisterBlock<ObsidianBlock> autoReg(49);} // namespace
} // namespace net::minecraft::block

