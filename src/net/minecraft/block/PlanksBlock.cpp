#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void PlanksBlock::registerClass()
{
    namespace mat = material;
    Block::PLANKS = (new Block(5, 4, mat::Material::WOOD))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("wood")->ignoreMetaUpdates();
}




static ::net::minecraft::registry::RegisterBlock<PlanksBlock> autoReg(5);
} // namespace
} // namespace net::minecraft::block
