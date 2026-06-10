#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/GlassBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void GlassBlock::registerClass()
{
    namespace mat = material;
    Block::GLASS = (new GlassBlock(20, 49, mat::Material::GLASS, false))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setTranslationKey("glass");
}




static ::net::minecraft::registry::RegisterBlock<GlassBlock> autoReg(20);
} // namespace
} // namespace net::minecraft::block

