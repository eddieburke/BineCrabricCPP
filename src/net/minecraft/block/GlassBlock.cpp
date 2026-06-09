#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/GlassBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {
namespace {

void registerGlassBlock()
{
    namespace mat = material;
    Block::GLASS = (new GlassBlock(20, 49, mat::Material::GLASS, false))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setTranslationKey("glass");
}

MINECRAFT_REGISTER_BLOCK(registerGlassBlock, 20);

} // namespace
} // namespace net::minecraft::block

