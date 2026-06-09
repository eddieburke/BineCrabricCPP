#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void registerLapisBlock()
{
    namespace mat = material;
    Block::LAPIS_BLOCK = (new Block(22, 144, mat::Material::STONE))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("blockLapis");
}

MINECRAFT_REGISTER_BLOCK(registerLapisBlock, 22);

} // namespace
} // namespace net::minecraft::block
