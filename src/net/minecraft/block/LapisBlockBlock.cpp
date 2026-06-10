#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void LapisBlockBlock::registerClass()
{
    namespace mat = material;
    Block::LAPIS_BLOCK = (new Block(22, 144, mat::Material::STONE))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("blockLapis");
}




static ::net::minecraft::registry::RegisterBlock<LapisBlockBlock> autoReg(22);
} // namespace
} // namespace net::minecraft::block
