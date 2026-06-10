#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {

struct BedrockBlockRegistrar {
    static void registerClass()
    {
        namespace mat = material;
        Block::BEDROCK = (new Block(7, 17, mat::Material::STONE))->setUnbreakable()->setResistance(6000000.0f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("bedrock")->disableTrackingStatistics();
    }
};

static registry::RegisterCustom<BedrockBlockRegistrar> s_reg(registry::kBlockRegistrarBase + 7);

} // namespace net::minecraft::block
