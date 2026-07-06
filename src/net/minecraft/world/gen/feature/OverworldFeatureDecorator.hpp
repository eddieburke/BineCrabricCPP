#pragma once
#include "net/minecraft/util/math/noise/OctavePerlinNoiseSampler.hpp"
#include <cstdint>
#include <vector>
namespace net::minecraft {
class BiomeSource;
class JavaRandom;
class World;
namespace world::gen::feature {
void decorateOverworldChunk(World* world, JavaRandom& random, BiomeSource& biomeSource,
                            OctavePerlinNoiseSampler& forestNoise, int chunkX, int chunkZ,
                            std::vector<double>& decorateTemperatures);
}
} // namespace net::minecraft
