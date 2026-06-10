#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/PlantBlock.hpp"

namespace net::minecraft::block {
void PlantBlock::registerClass()
{
    Block::DANDELION = (new PlantBlock(37, 13))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("flower");
    Block::ROSE = (new PlantBlock(38, 12))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("rose");
}




namespace {static ::net::minecraft::registry::RegisterBlock<PlantBlock> autoReg(37);} // namespace
} // namespace net::minecraft::block

