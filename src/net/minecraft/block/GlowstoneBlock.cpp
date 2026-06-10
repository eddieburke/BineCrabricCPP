#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/GlowstoneBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void GlowstoneBlock::registerClass()
{
    namespace mat = material;
    Block::GLOWSTONE = (new GlowstoneBlock(89, 105, mat::Material::STONE))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setLuminance(1.0f)->setTranslationKey("lightgem");
}




static ::net::minecraft::registry::RegisterBlock<GlowstoneBlock> autoReg(89);
} // namespace
} // namespace net::minecraft::block

