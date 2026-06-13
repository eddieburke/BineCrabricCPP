#include "net/minecraft/world/biome/Biomes.hpp"

#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::world::biome {

struct BiomeBootstrap {
    static void registerClass() { Biomes::init(); }
};

static registry::RegisterPhase<BiomeBootstrap> s_reg(mod::LifecyclePhase::BiomeRegistration, 0);

} // namespace net::minecraft::world::biome
