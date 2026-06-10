#include "net/minecraft/world/dimension/NetherDimension.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/BiomeDefinition.hpp"
#include "net/minecraft/world/biome/Biomes.hpp"
#include "net/minecraft/world/biome/source/FixedBiomeSource.hpp"
#include "net/minecraft/world/gen/chunk/NetherChunkGenerator.hpp"

namespace net::minecraft {

void NetherDimension::initBiomeSource()
{
    const BiomeDefinition& hell = Biomes::hell();
    const BiomeInfo hellInfo {hell.id, hell.name, hell.topBlockId, hell.soilBlockId};
    biomeSource = std::make_unique<FixedBiomeSource>(hellInfo, 1.0, 0.0);
    isNether = true;
    evaporatesWater = true;
    hasCeiling = true;
    id = -1;
}

void NetherDimension::initBrightnessTable()
{
    constexpr float factor = 0.1f;
    for (int i = 0; i <= 15; ++i) {
        const float value = 1.0f - static_cast<float>(i) / 15.0f;
        lightLevelToLuminance[static_cast<std::size_t>(i)] =
            (1.0f - value) / (value * 3.0f + 1.0f) * (1.0f - factor) + factor;
    }
}

std::unique_ptr<ChunkSource> NetherDimension::createChunkGenerator()
{
    if (world == nullptr) {
        return nullptr;
    }
    return std::make_unique<NetherChunkGenerator>(world, world->seed());
}

std::unique_ptr<ChunkSource> NetherDimension::createChunkGeneratorFromSeed(std::uint64_t seed)
{
    if (world == nullptr) {
        return nullptr;
    }
    return std::make_unique<NetherChunkGenerator>(world, seed);
}

bool NetherDimension::isValidSpawnPoint(int x, int z) const
{
    if (world == nullptr) {
        return false;
    }
    const int blockId = world->getSpawnBlockId(x, z);
    if (blockId == Block::BEDROCK->id) {
        return false;
    }
    if (blockId == 0) {
        return false;
    }
    if (blockId < 0 || blockId >= static_cast<int>(Block::BLOCKS_OPAQUE.size())) {
        return false;
    }
    return Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)];
}

float NetherDimension::getTimeOfDay(long long time, float tickDelta) const
{
    (void)time;
    (void)tickDelta;
    return 0.5f;
}

Vec3d NetherDimension::getFogColor(float timeOfDay, float tickDelta) const
{
    (void)timeOfDay;
    (void)tickDelta;
    return {0.2, 0.03, 0.03};
}

} // namespace net::minecraft
