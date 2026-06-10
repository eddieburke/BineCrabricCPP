#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void BricksBlock::registerClass()
{
    namespace mat = material;
    Block::BRICKS = (new Block(45, 7, mat::Material::STONE))->setHardness(2.0f)->setResistance(10.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("brick");
}




static ::net::minecraft::registry::RegisterBlock<BricksBlock> autoReg(45);
} // namespace
} // namespace net::minecraft::block
