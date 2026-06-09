#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void registerBedrockBlock()
{
    namespace mat = material;
    Block::BEDROCK = (new Block(7, 17, mat::Material::STONE))->setUnbreakable()->setResistance(6000000.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("bedrock")->disableTrackingStatistics();
}

MINECRAFT_REGISTER_BLOCK(registerBedrockBlock, 7);

} // namespace
} // namespace net::minecraft::block
