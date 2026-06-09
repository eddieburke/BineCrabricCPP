#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void registerBricksBlock()
{
    namespace mat = material;
    Block::BRICKS = (new Block(45, 7, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("brick");
}

MINECRAFT_REGISTER_BLOCK(registerBricksBlock, 45);

} // namespace
} // namespace net::minecraft::block
