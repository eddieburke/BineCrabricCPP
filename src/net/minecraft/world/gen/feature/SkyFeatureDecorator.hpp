#pragma once
#include <cstdint>
#include <vector>

#include "net/minecraft/util/math/noise/OctavePerlinNoiseSampler.hpp"

namespace net::minecraft {
class BiomeSource;
class JavaRandom;
class World;

namespace world::gen::feature {
void decorateSkyChunk(World* world,
                      JavaRandom& random,
                      BiomeSource& biomeSource,
                      OctavePerlinNoiseSampler& forestNoise,
                      int chunkX,
                      int chunkZ,
                      std::vector<double>& decorateTemperatures);
}
}  // namespace net::minecraft
