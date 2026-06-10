#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void MossyCobblestoneBlock::registerClass()
{
    namespace mat = material;
    Block::MOSSY_COBBLESTONE = (new Block(48, 36, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("stoneMoss");
}




static ::net::minecraft::registry::RegisterBlock<MossyCobblestoneBlock> autoReg(48);
} // namespace
} // namespace net::minecraft::block
