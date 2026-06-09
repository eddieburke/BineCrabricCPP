#include "net/minecraft/world/dimension/Dimension.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/dimension/NetherDimension.hpp"
#include "net/minecraft/world/dimension/OverworldDimension.hpp"
#include "net/minecraft/world/dimension/SkylandsDimension.hpp"
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
    if (dimensionId == -1) {
        return std::make_unique<NetherDimension>();
    }
    if (dimensionId == 0) {
        return std::make_unique<OverworldDimension>();
    }
    if (dimensionId == 1) {
        return std::make_unique<SkylandsDimension>();
    }
    return nullptr;
}

} // namespace net::minecraft
