#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/DeadBushBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerDeadBushBlock()
{
    Block::DEAD_BUSH = (new DeadBushBlock(32, 55))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("deadbush");
}

MINECRAFT_REGISTER_BLOCK(registerDeadBushBlock, 32);

} // namespace
} // namespace net::minecraft::block

