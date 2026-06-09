#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/Biomes.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

#include <memory>

namespace net::minecraft {

// Delegates to the full biome objects in Biomes, matching Java per-biome overrides.
inline std::unique_ptr<Feature> getRandomTreeFeature(BiomeId biome, JavaRandom& random)
{
    return Biomes::byId(biome).getRandomTreeFeature(random);
}

} // namespace net::minecraft
