#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void registerCobblestoneBlock()
{
    namespace mat = material;
    Block::COBBLESTONE = (new Block(4, 16, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stonebrick");
}

MINECRAFT_REGISTER_BLOCK(registerCobblestoneBlock, 4);

} // namespace
} // namespace net::minecraft::block
