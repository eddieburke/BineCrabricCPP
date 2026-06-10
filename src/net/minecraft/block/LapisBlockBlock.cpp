#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {

struct LapisBlockBlockRegistrar {
    static void registerClass()
    {
        namespace mat = material;
        Block::LAPIS_BLOCK = (new Block(22, 144, mat::Material::STONE))->setHardness(3.0f)->setResistance(5.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("blockLapis");
    }
};

static registry::RegisterCustom<LapisBlockBlockRegistrar> s_reg(registry::kBlockRegistrarBase + 22);

} // namespace net::minecraft::block
