#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SpawnerBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerSpawnerBlock()
{
    Block::SPAWNER = (new SpawnerBlock(52, 65))->setHardness(5.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("mobSpawner")->disableTrackingStatistics();
}

MINECRAFT_REGISTER_BLOCK(registerSpawnerBlock, 52);

} // namespace
} // namespace net::minecraft::block

