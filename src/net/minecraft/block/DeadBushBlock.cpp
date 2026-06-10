#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/DeadBushBlock.hpp"

namespace net::minecraft::block {
void DeadBushBlock::registerClass()
{
    Block::DEAD_BUSH = (new DeadBushBlock(32, 55))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("deadbush");
}




namespace {static ::net::minecraft::registry::RegisterBlock<DeadBushBlock> autoReg(32);} // namespace
} // namespace net::minecraft::block

