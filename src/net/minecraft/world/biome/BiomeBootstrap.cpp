#include "net/minecraft/world/biome/Biomes.hpp"

#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::world::biome {

struct BiomeBootstrap {
    static void registerClass() { Biomes::init(); }
};

static registry::RegisterCustom<BiomeBootstrap> s_reg(registry::kBiomeRegistrarPriority);

} // namespace net::minecraft::world::biome
