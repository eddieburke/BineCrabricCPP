#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/SaplingBlock.hpp"

namespace net::minecraft::block {
namespace {

void registerSaplingBlock()
{
    Block::SAPLING = (new SaplingBlock(6, 15))->setHardness(0.0f)->setSoundGroup(&vanillaDirtSound())->setTranslationKey("sapling")->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerSaplingBlock, 6);

} // namespace
} // namespace net::minecraft::block

