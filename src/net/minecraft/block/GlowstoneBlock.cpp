#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/GlowstoneBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void registerGlowstoneBlock()
{
    namespace mat = material;
    Block::GLOWSTONE = (new GlowstoneBlock(89, 105, mat::Material::STONE))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setLuminance(1.0f)->setTranslationKey("lightgem");
}

MINECRAFT_REGISTER_BLOCK(registerGlowstoneBlock, 89);

} // namespace
} // namespace net::minecraft::block

