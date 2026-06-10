#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/TallPlantBlock.hpp"

namespace net::minecraft::block {
void TallPlantBlock::registerClass()
{
    Block::GRASS = (new TallPlantBlock(31, 39))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("tallgrass");
}




namespace {static ::net::minecraft::registry::RegisterBlock<TallPlantBlock> autoReg(31);} // namespace
} // namespace net::minecraft::block

