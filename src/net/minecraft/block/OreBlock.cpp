#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/OreBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerOreBlocks()
{
    Block::GOLD_ORE = (new OreBlock(14, 32))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreGold");
    Block::IRON_ORE = (new OreBlock(15, 33))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreIron");
    Block::COAL_ORE = (new OreBlock(16, 34))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreCoal");
    Block::LAPIS_ORE = (new OreBlock(21, 160))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreLapis");
    Block::DIAMOND_ORE = (new OreBlock(56, 50))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("oreDiamond");
}

MINECRAFT_REGISTER_BLOCK(registerOreBlocks, 14);

} // namespace
} // namespace net::minecraft::block

