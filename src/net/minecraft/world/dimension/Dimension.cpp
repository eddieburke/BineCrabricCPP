#include "net/minecraft/world/dimension/Dimension.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/chunk/NetherChunkGenerator.hpp"
#include "net/minecraft/world/gen/chunk/OverworldGeneratorChunkSource.hpp"

namespace net::minecraft {

void Dimension::setWorld(World* worldIn)
{
    world = worldIn;
    initBiomeSource();
    initBrightnessTable();
}

void Dimension::initBrightnessTable()
{
    constexpr float factor = 0.05f;
    for (int i = 0; i <= 15; ++i) {
        float value = 1.0f - static_cast<float>(i) / 15.0f;
        lightLevelToLuminance[static_cast<std::size_t>(i)] = (1.0f - value) / (value * 3.0f + 1.0f) * (1.0f - factor) + factor;
    }
}

float Dimension::luminanceForLightLevel(int lightLevel)
{
    if (lightLevel < 0) {
        lightLevel = 0;
    } else if (lightLevel > 15) {
        lightLevel = 15;
    }
    constexpr float factor = 0.05f;
    const float value = 1.0f - static_cast<float>(lightLevel) / 15.0f;
    return (1.0f - value) / (value * 3.0f + 1.0f) * (1.0f - factor) + factor;
}

void Dimension::initBiomeSource()
{
    if (world != nullptr) {
        biomeSource = std::make_unique<BiomeSource>(world->getSeed());
    }
}

std::unique_ptr<ChunkSource> Dimension::createChunkGenerator()
{
    if (world == nullptr) {
        return nullptr;
    }
    return std::make_unique<OverworldGeneratorChunkSource>(world, world->seed());
}

std::unique_ptr<ChunkSource> Dimension::createChunkGeneratorFromSeed(std::uint64_t seed)
{
    if (world == nullptr) {
        return nullptr;
    }
    return std::make_unique<OverworldGeneratorChunkSource>(world, seed, /*useLocalBiomeSource=*/true);
}

bool Dimension::isValidSpawnPoint(int x, int z) const
{
    if (world == nullptr) {
        return false;
    }
    const int spawnId = world->getSpawnBlockId(x, z);
    return Block::SAND != nullptr && spawnId == Block::SAND->id;
}

std::unique_ptr<Dimension> Dimension::fromId(int dimensionId)
{
    return registry::DimensionRegistry::instance().create(dimensionId);
}

} // namespace net::minecraft
