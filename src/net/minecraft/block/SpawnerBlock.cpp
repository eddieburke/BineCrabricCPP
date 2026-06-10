#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SpawnerBlock.hpp"

namespace net::minecraft::block {
namespace {

void SpawnerBlock::registerClass()
{
    Block::SPAWNER = (new SpawnerBlock(52, 65))->setHardness(5.0f)->setSoundGroup(&vanillaMetalSound())->setTranslationKey("mobSpawner")->disableTrackingStatistics();
}




static ::net::minecraft::registry::RegisterBlock<SpawnerBlock> autoReg(52);
} // namespace
} // namespace net::minecraft::block

