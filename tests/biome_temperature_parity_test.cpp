// biome_temperature_parity_test.cpp -- Java keeps sky temperature separate from biome climate.
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

#include <cmath>
#include <cstdio>

namespace {

int failures = 0;

void check(bool ok, const char* message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

void checkClose(double got, double expected, const char* message)
{
    if (std::abs(got - expected) > 1.0e-12) {
        std::fprintf(stderr, "FAIL: %s got=%.17g expected=%.17g\n", message, got, expected);
        ++failures;
    }
}

void probe(std::uint64_t seed, int x, int z)
{
    net::minecraft::World world("temperature-parity", seed);
    net::minecraft::BiomeSource reference(seed);

    const double worldTemperature = world.getTemperature(x, z);
    const double javaTemperature = reference.getTemperature(x, z);
    const double biomeClimateTemperature = reference.sampleClimate(x, z).temperature;

    std::printf("seed=%llu x=%d z=%d worldTemperature=%.17g javaTemperature=%.17g biomeClimateTemperature=%.17g\n",
        static_cast<unsigned long long>(seed), x, z, worldTemperature, javaTemperature, biomeClimateTemperature);

    checkClose(worldTemperature, javaTemperature, "World::getTemperature must match BiomeSource::getTemperature");
    check(std::abs(javaTemperature - biomeClimateTemperature) > 1.0e-6,
        "raw Java temperature should stay distinct from transformed biome climate");
}

} // namespace

int main()
{
    net::minecraft::block::initializeBlocks();
    net::minecraft::registry::Registry::bootstrap();

    probe(12345ULL, 8, 8);
    probe(0ULL, -64, 96);

    if (failures == 0) {
        std::printf("biome_temperature_parity_test: passed\n");
        return 0;
    }
    std::fprintf(stderr, "biome_temperature_parity_test: %d failure(s)\n", failures);
    return 1;
}
