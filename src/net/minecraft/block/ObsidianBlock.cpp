#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/ObsidianBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerObsidianBlock()
{
    Block::OBSIDIAN = (new ObsidianBlock(49, 37))->setHardness(10.0f)->setResistance(2000.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("obsidian");
}

MINECRAFT_REGISTER_BLOCK(registerObsidianBlock, 49);

} // namespace
} // namespace net::minecraft::block

