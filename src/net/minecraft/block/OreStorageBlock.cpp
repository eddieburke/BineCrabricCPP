#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/OreStorageBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerOreStorageBlocks()
{
    Block::GOLD_BLOCK = (new OreStorageBlock(41, 23))->setHardness(3.0f)->setResistance(10.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("blockGold");
    Block::IRON_BLOCK = (new OreStorageBlock(42, 22))->setHardness(5.0f)->setResistance(10.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("blockIron");
    Block::DIAMOND_BLOCK = (new OreStorageBlock(57, 24))->setHardness(5.0f)->setResistance(10.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("blockDiamond");
}

MINECRAFT_REGISTER_BLOCK(registerOreStorageBlocks, 41);

} // namespace
} // namespace net::minecraft::block

